// Profile loading, detection, and deterministic prefiltering.
// Included by main.cpp; keep this module focused on this responsibility.

DiagnosticSplit SplitWithProfile(const std::string& text, const Config& cfg) {
    DiagnosticSplit r; const auto* p=DetectProfile(text,cfg); if(!p) return r;
    std::istringstream in(text); std::ostringstream good,questionable; std::unordered_set<std::string> seenGood,seenQ; std::string line;
    while(std::getline(in,line)) {
        if(!line.empty()&&line.back()=='\r') line.pop_back();
        if(ContainsAny(line,p->noise)) continue;
        if(ContainsAny(line,p->questionable)) {
            const std::string shown = FormatDiagnosticText(line, cfg);
            if(seenQ.insert(shown).second) questionable<<shown;
            continue;
        }
        const bool high=ContainsAny(line,p->highPriority), warn=ContainsAny(line,p->warnings);
        if((cfg.showErrors&&high)||(cfg.showWarnings&&warn)) {
            const std::string shown = FormatDiagnosticText(line, cfg);
            if(seenGood.insert(shown).second) good<<shown;
        }
    }
    r.included=good.str(); r.questionable=questionable.str(); return r;
}
std::string ProfileName(const std::string& text,const Config& cfg) {
    const auto* p=DetectProfile(text,cfg); return p?p->name:"Generic Log";
}

bool LooksLikeUnrealLog(const std::string& text) {
    return text.find("LogInit:") != std::string::npos ||
           text.find("LogModuleManager:") != std::string::npos ||
           text.find("LogPluginManager:") != std::string::npos ||
           text.find("LogWindows:") != std::string::npos;
}

bool LooksLikeGenericLog(const std::string& text);
std::string GenericLogCandidates(const std::string& text);

std::string PreFilterUnrealLog(const std::string& text, const Config& cfg) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    std::ostringstream out;
    std::unordered_set<std::string> seen;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& s = lines[i];
        const bool error =
            s.find(": Error:") != std::string::npos || s.find("Fatal error:") != std::string::npos ||
            s.find("Ensure condition failed") != std::string::npos || s.find("Assertion failed") != std::string::npos ||
            s.find("Unhandled Exception") != std::string::npos || s.find("LowLevelFatalError") != std::string::npos;
        const bool warning = s.find(": Warning:") != std::string::npos;
        const bool knownNoise =
            s.find("LogWindows: Failed to load") != std::string::npos ||
            s.find("Failed to SetupSDK") != std::string::npos ||
            s.find("WinPixGpuCapturer") != std::string::npos ||
            s.find("Wintab32") != std::string::npos ||
            s.find("failed to initialize") != std::string::npos ||
            s.find("does not exist") != std::string::npos ||
            s.find("Incompatible or missing module") != std::string::npos ||
            s.find("Required extension") != std::string::npos ||
            s.find("Could not enable all required OpenXR extensions") != std::string::npos ||
            s.find("not supported by this project's data") != std::string::npos ||
            s.find("Found VULKAN_SDK") != std::string::npos ||
            s.find("Adding HMD requested") != std::string::npos ||
            s.find("isn't part of the engine's core extension list") != std::string::npos;

        // Known-noise is a classification, not an additional include source. If it is
        // disabled, suppress matching lines even when the generic Warnings toggle is on.
        const bool important = (cfg.showErrors && error && (!knownNoise || cfg.showKnownNoise)) ||
                               (cfg.showWarnings && warning && (!knownNoise || cfg.showKnownNoise)) ||
                               (cfg.showKnownNoise && knownNoise);
        if (!important) continue;

        const size_t begin = cfg.showContext && i > 1 ? i - 1 : i;
        const size_t end = cfg.showContext ? std::min(lines.size(), i + 3) : i + 1;
        for (size_t j = begin; j < end; ++j) {
            const std::string shown = FormatDiagnosticText(lines[j], cfg);
            if (seen.insert(shown).second) out << shown;
        }
    }
    return out.str();
}

DiagnosticSplit SplitUnrealDiagnostics(const std::string& text, const Config& cfg) {
    DiagnosticSplit result;
    std::istringstream in(text);
    std::unordered_set<std::string> includedSeen, questionableSeen;
    std::ostringstream included, questionable;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const bool error =
            line.find(": Error:") != std::string::npos || line.find("Fatal error:") != std::string::npos ||
            line.find("Ensure condition failed") != std::string::npos || line.find("Assertion failed") != std::string::npos ||
            line.find("Unhandled Exception") != std::string::npos || line.find("LowLevelFatalError") != std::string::npos;
        const bool warning = line.find(": Warning:") != std::string::npos;
        const bool questionableLine =
            line.find("LogWindows: Failed to load") != std::string::npos ||
            line.find("Failed to SetupSDK") != std::string::npos ||
            line.find("WinPixGpuCapturer") != std::string::npos ||
            line.find("Wintab32") != std::string::npos ||
            line.find("failed to initialize") != std::string::npos ||
            line.find("does not exist") != std::string::npos ||
            line.find("Incompatible or missing module") != std::string::npos ||
            line.find("Required extension") != std::string::npos ||
            line.find("Could not enable all required OpenXR extensions") != std::string::npos ||
            line.find("not supported by this project's data") != std::string::npos ||
            line.find("Found VULKAN_SDK") != std::string::npos ||
            line.find("Adding HMD requested") != std::string::npos ||
            line.find("isn't part of the engine's core extension list") != std::string::npos;
        if (questionableLine) {
            const std::string shown = FormatDiagnosticText(line, cfg);
            if (questionableSeen.insert(shown).second) questionable << shown;
            continue;
        }
        if ((cfg.showErrors && error) || (cfg.showWarnings && warning)) {
            const std::string shown = FormatDiagnosticText(line, cfg);
            if (includedSeen.insert(shown).second) included << shown;
        }
    }
    result.included = included.str();
    result.questionable = questionable.str();
    return result;
}

std::string DetectLogType(const std::string& text) {
    if (LooksLikeUnrealLog(text)) return "Unreal";
    if (text.find("error C") != std::string::npos || text.find("LNK") != std::string::npos) return "MSVC / Linker";
    if (text.find("undefined reference") != std::string::npos || text.find("fatal error:") != std::string::npos) return "GCC / Clang";
    if (text.find("ninja: build stopped") != std::string::npos || text.find("CMake Error") != std::string::npos) return "CMake / Ninja";
    if (text.find("Unhandled Exception") != std::string::npos || text.find("Stack trace") != std::string::npos) return "Crash / Stack";
    if (LooksLikeGenericLog(text)) return "Generic log";
    return "General";
}

std::string PreFilter(const std::string& text, const Config& cfg) {
    if (LooksLikeUnrealLog(text)) {
        // Unreal logs must never fall through to the generic fallback. A clean Unreal
        // log legitimately produces an empty candidate set; returning the whole log
        // here can overflow the model context for a successful run.
        return PreFilterUnrealLog(text, cfg);
    }

    const DiagnosticSplit profiled = SplitWithProfile(text, cfg);
    if (!profiled.included.empty()) return DedupeLines(profiled.included);
    const std::string genericCandidates = LooksLikeGenericLog(text) ? GenericLogCandidates(text) : std::string{};
    if (!genericCandidates.empty()) return DedupeLines(genericCandidates);

    std::istringstream in(text);
    std::ostringstream errors, warnings;
    std::unordered_set<std::string> seenErrors, seenWarnings;
    std::string line;
    bool ignoredSection = false;
    bool previousWasKeptError = false;
    bool haveError = false;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        const bool sectionDivider = line.find("----------------") != std::string::npos;
        if (line.find("previous session") != std::string::npos ||
            line.find("runtime log from earlier") != std::string::npos ||
            line.find("generated source") != std::string::npos ||
            line.find("Application test output") != std::string::npos) {
            ignoredSection = true;
            previousWasKeptError = false;
            continue;
        }
        if (ignoredSection && sectionDivider &&
            (line.find("actual build result") != std::string::npos ||
             line.find("actual") != std::string::npos)) {
            ignoredSection = false;
            continue;
        }
        if (ignoredSection) continue;

        const bool debugOrTest =
            line.rfind("DEBUG:", 0) == 0 ||
            line.rfind("INFO:", 0) == 0 ||
            line.rfind("[TEST]", 0) == 0 ||
            line.find("Example diagnostic") != std::string::npos ||
            line.find("ExampleErrors") != std::string::npos;
        if (debugOrTest) {
            previousWasKeptError = false;
            continue;
        }

        const bool compilerError =
            line.find("): error C") != std::string::npos ||
            line.find(": error:") != std::string::npos ||
            line.find(": fatal error ") != std::string::npos ||
            line.find(" : fatal error LNK") != std::string::npos ||
            line.find(" : error LNK") != std::string::npos ||
            line.find(".obj : error LNK") != std::string::npos ||
            line.find("undefined reference") != std::string::npos ||
            line.find("unresolved external symbol") != std::string::npos;
        const bool buildFailure =
            line.rfind("FAILED:", 0) == 0 ||
            line.rfind("ninja: build stopped:", 0) == 0 ||
            line.rfind("Build command exited with code ", 0) == 0;
        const bool note = line.find("): note:") != std::string::npos ||
                          line.find(": note:") != std::string::npos;
        const bool warning = line.find("): warning C") != std::string::npos ||
                             line.find(": warning:") != std::string::npos ||
                             line.find(" : warning LNK") != std::string::npos;

        if (compilerError || buildFailure) {
            if (seenErrors.insert(line).second) errors << line << '\n';
            haveError = true;
            previousWasKeptError = compilerError;
        } else if (note && previousWasKeptError) {
            if (seenErrors.insert(line).second) errors << line << '\n';
        } else {
            previousWasKeptError = false;
            if (warning && seenWarnings.insert(line).second) warnings << line << '\n';
        }
    }

    if (haveError) return errors.str();
    const std::string warningText = warnings.str();
    if (!warningText.empty()) return warningText;
    return DedupeLines(text);
}

