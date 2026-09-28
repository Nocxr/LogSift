// Structured fast path, chunking, result finalization, cancellation.
// Included by main.cpp; keep this module focused on this responsibility.

bool LooksLikeGenericLog(const std::string& text) {
    if (text.size() < 160) return false;
    std::istringstream in(text); std::string line;
    int nonEmpty=0, timestampish=0, signal=0, structured=0;
    while (std::getline(in,line) && nonEmpty<300) {
        if (line.find_first_not_of(" \t\r")==std::string::npos) continue;
        ++nonEmpty;
        if ((line.size()>=19 && std::isdigit((unsigned char)line[0]) && line[4]=='-' && line[7]=='-') ||
            (!line.empty() && line[0]=='[' && line.find(']')<40)) ++timestampish;
        if (line.find('=')!=std::string::npos || line.find(" | ")!=std::string::npos) ++structured;
        std::string lower=line;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char ch){return (char)std::tolower(ch);});
        if (lower.find("error")!=std::string::npos || lower.find("fail")!=std::string::npos ||
            lower.find("alert")!=std::string::npos || lower.find("warn")!=std::string::npos ||
            lower.find("timeout")!=std::string::npos || lower.find("unable")!=std::string::npos ||
            lower.find("degraded")!=std::string::npos || lower.find("exception")!=std::string::npos ||
            lower.find("critical")!=std::string::npos || lower.find("retry")!=std::string::npos) ++signal;
    }
    return nonEmpty>=5 && signal>=1 && (timestampish>=3 || structured>=3 || nonEmpty>=12);
}

std::string GenericLogCandidates(const std::string& text) {
    std::istringstream in(text); std::ostringstream out; std::string line;
    while (std::getline(in,line)) {
        std::string lower=line;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char ch){return (char)std::tolower(ch);});
        if (lower.find("error")!=std::string::npos || lower.find("fail")!=std::string::npos ||
            lower.find("alert")!=std::string::npos || lower.find("warn")!=std::string::npos ||
            lower.find("timeout")!=std::string::npos || lower.find("unable")!=std::string::npos ||
            lower.find("degraded")!=std::string::npos || lower.find("exception")!=std::string::npos ||
            lower.find("critical")!=std::string::npos || lower.find("retry")!=std::string::npos) {
            out<<line<<'\n';
        }
    }
    return out.str();
}

bool LooksLikeStructuredBuildDiagnostics(const std::string& filtered) {
    int strong = 0;
    std::istringstream in(filtered);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(": error ") != std::string::npos ||
            line.find("fatal error") != std::string::npos ||
            line.find("LNK") != std::string::npos ||
            line.find("undefined reference") != std::string::npos ||
            line.find("unresolved external") != std::string::npos) {
            ++strong;
        }
    }
    return strong > 0;
}

std::string FastStructuredResult(const std::string& filtered) {
    return DedupeLines(filtered);
}

std::string ExtractVerbatimModelDiagnostics(
    const std::string& modelText,
    const std::string& modelInput,
    const std::string& originalInput) {

    auto restoreFullSourceLine = [](const std::string& source, const std::string& selected) {
        std::istringstream in(source);
        std::string line;
        std::string best;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line == selected) return line;
            if (!selected.empty() && line.find(selected) != std::string::npos) {
                // Models occasionally return only the beginning of a requested
                // verbatim line. Prefer the shortest containing source line so
                // the result is restored instead of displaying a chopped prefix.
                if (best.empty() || line.size() < best.size()) best = line;
            }
        }
        return best;
    };

    std::istringstream filteredOut(modelText);
    std::ostringstream clean;
    std::string outLine;
    int kept = 0;
    while (std::getline(filteredOut, outLine) && kept < 12) {
        if (!outLine.empty() && outLine.back() == '\r') outLine.pop_back();
        if (outLine == "NO_DIAGNOSTICS") {
            clean << outLine << '\n';
            break;
        }
        const bool commentary =
            outLine.find("**") != std::string::npos ||
            outLine.find("Analyzing") != std::string::npos ||
            outLine.find("Warnings:") != std::string::npos ||
            outLine.find("Several") != std::string::npos ||
            outLine.find("Blocks of") != std::string::npos;
        if (commentary || outLine.empty()) continue;

        std::string restored = restoreFullSourceLine(modelInput, outLine);
        if (restored.empty())
            restored = restoreFullSourceLine(originalInput, outLine);

        if (!restored.empty()) {
            clean << restored << '\n';
            ++kept;
        }
    }
    return DedupeLines(clean.str());
}

std::vector<std::string> DiagnosticEntries(const std::string& text);

std::vector<std::string> ChunkModelInput(const std::string& text, size_t maxBytes = 12000) {
    std::vector<std::string> chunks;
    if (text.empty()) return chunks;

    std::istringstream in(text);
    std::string line;
    std::string current;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t needed = line.size() + 1;
        if (!current.empty() && current.size() + needed > maxBytes) {
            chunks.push_back(std::move(current));
            current.clear();
        }
        if (needed > maxBytes) {
            size_t offset = 0;
            while (offset < line.size()) {
                const size_t take = std::min(maxBytes - 1, line.size() - offset);
                chunks.push_back(line.substr(offset, take) + "\n");
                offset += take;
            }
            continue;
        }
        current += line;
        current += '\n';
    }
    if (!current.empty()) chunks.push_back(std::move(current));
    return chunks;
}

SiftResult SendModelChunk(
    const Config& cfg,
    const std::string& modelInput,
    const std::string& originalInput,
    const std::string& prompt) {

    json body = {
        {"model", cfg.model},
        {"messages", json::array({
            {{"role", "system"}, {"content", prompt}},
            {{"role", "user"}, {"content",
                std::string("Select the highest-value diagnostics from these candidate log lines. Output at most 12 lines, verbatim.\n\n") +
                modelInput +
                ((cfg.model.find("qwen3") != std::string::npos || cfg.model.find("Qwen3") != std::string::npos)
                    ? "\n/no_think"
                    : "")}}
        })},
        {"temperature", 0},
        {"max_tokens", 4096}
    };

    const auto temp = std::filesystem::temp_directory_path() /
        ("logsift-" + std::to_string(SDL_GetTicks()) + "-" +
         std::to_string(std::hash<std::string>{}(modelInput)) + ".json");
    {
        std::ofstream out(temp, std::ios::binary);
        out << body.dump();
    }

    std::string cmd = "curl -sS --fail-with-body --max-time 120 -X POST " +
        ShellQuote(cfg.endpoint) + " -H " + ShellQuote("Content-Type: application/json");
    if (!cfg.apiKey.empty())
        cmd += " -H " + ShellQuote("Authorization: Bearer " + cfg.apiKey);
    cmd += " --data-binary @" + ShellQuote(temp.string()) + " 2>&1";

    std::string raw;
    try {
        raw = ReadPipe(cmd);
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        throw;
    }
    std::error_code ec;
    std::filesystem::remove(temp, ec);

    const json response = json::parse(raw);
    if (response.contains("error"))
        throw std::runtime_error(response["error"].dump(2));
    if (!response.contains("choices") || response["choices"].empty())
        throw std::runtime_error("Unexpected model response: " + raw);

    const auto& choice = response["choices"][0];
    const auto& message = choice["message"];
    const std::string content = message.value("content", "");
    std::string reasoning = message.value("reasoning_content", "");
    if (reasoning.empty()) reasoning = message.value("reasoning", "");

    SiftResult result;
    if (!content.empty()) {
        result.text = ExtractVerbatimModelDiagnostics(content, modelInput, originalInput);
        result.route = "LLM";
    }
    if (result.text.empty() && !reasoning.empty()) {
        result.text = ExtractVerbatimModelDiagnostics(reasoning, modelInput, originalInput);
        result.route = "LLM reasoning extract";
        if (!result.text.empty())
            result.note = "Model returned diagnostics through its reasoning channel.";
    }

    if (result.text.empty()) {
        std::istringstream localIn(modelInput);
        std::ostringstream localOut;
        std::string localLine;
        int kept = 0;
        while (std::getline(localIn, localLine) && kept < 12) {
            if (!localLine.empty()) {
                localOut << localLine << '\n';
                ++kept;
            }
        }
        result.text = kept ? DedupeLines(localOut.str()) : "NO_DIAGNOSTICS";
        result.route = "Model fallback";
        result.usedLocalFallback = true;
        const std::string finishReason = choice.value("finish_reason", "");
        if (!reasoning.empty() && finishReason == "length")
            result.note = "Model used its response budget without returning usable final diagnostics.";
        else if (!reasoning.empty())
            result.note = "Model returned reasoning but no usable verbatim diagnostics.";
        else if (!content.empty())
            result.note = "Model response did not contain verbatim diagnostic lines.";
        else
            result.note = "Model returned no diagnostic content.";
    }

    if (response.contains("usage")) {
        const auto& usage = response["usage"];
        result.promptTokens = usage.value("prompt_tokens", 0);
        result.completionTokens = usage.value("completion_tokens", 0);
    }
    if (response.contains("stats")) {
        const auto& responseStats = response["stats"];
        result.promptTokensPerSecond = responseStats.value("prompt_tokens_per_second", 0.0);
        result.completionTokensPerSecond = responseStats.value("tokens_per_second", 0.0);
    }
    return result;
}

std::string LimitDiagnosticLines(const std::string& text, int maxLines = 12) {
    std::istringstream in(DedupeLines(text));
    std::ostringstream out;
    std::string line;
    int kept = 0;
    while (std::getline(in, line) && kept < maxLines) {
        if (line.empty()) continue;
        out << line << '\n';
        ++kept;
    }
    return out.str();
}

std::string FinalizeModelText(std::string text, const std::string& input, const Config& cfg) {
    text = DedupeLines(text);
    if (text == "NO_DIAGNOSTICS\n" || text == "NO_DIAGNOSTICS")
        return "NO_DIAGNOSTICS";

    if (LooksLikeUnrealLog(input)) {
        std::istringstream dedupeIn(text);
        std::ostringstream dedupeOut;
        std::unordered_set<std::string> signatures;
        std::string line;
        while (std::getline(dedupeIn, line)) {
            std::string signature = line;
            const size_t logPos = signature.find("Log");
            if (logPos != std::string::npos) signature = signature.substr(logPos);
            while (!signature.empty() && std::isspace(static_cast<unsigned char>(signature.back())))
                signature.pop_back();
            if (signatures.insert(signature).second) dedupeOut << line << '\n';
        }
        text = DedupeLines(dedupeOut.str());

        std::istringstream formatIn(text);
        std::ostringstream formatOut;
        std::unordered_set<std::string> groups;
        while (std::getline(formatIn, line)) {
            std::string shown = line;
            const size_t category = shown.find("Log");
            if (!cfg.showTimestamps && category != std::string::npos)
                shown = shown.substr(category);

            if (cfg.groupDiagnostics) {
                std::string key = shown;
                const size_t warningPos = key.find(": Warning:");
                const size_t errorPos = key.find(": Error:");
                const size_t severityPos =
                    warningPos != std::string::npos ? warningPos : errorPos;
                if (severityPos != std::string::npos) {
                    const size_t msg =
                        severityPos + (warningPos != std::string::npos ? 10 : 8);
                    const size_t colon = key.find(':', msg);
                    const size_t equal = key.find('=', msg);
                    const size_t cut = std::min(
                        colon == std::string::npos ? key.size() : colon,
                        equal == std::string::npos ? key.size() : equal);
                    key = key.substr(0, cut);
                }
                if (!groups.insert(key).second) continue;
            }
            formatOut << shown << '\n';
        }
        text = DedupeLines(formatOut.str());
    }

    return LimitDiagnosticLines(text, 12);
}


void ThrowIfSiftCancelled(const std::shared_ptr<SiftProgress>& progress) {
    if (progress && progress->cancelled.load())
        throw std::runtime_error("Sift cancelled.");
}

