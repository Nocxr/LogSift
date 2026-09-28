// CLI argument parsing and CLI execution path.
// Included by main.cpp; keep this module focused on this responsibility.

    bool cliMode = false;
    bool cliJson = false;
    bool cliUseLlm = false;
    bool cliListProfiles = false;
    bool cliShowTimestamps = false;
    bool cliKnownNoise = false;
    bool cliNoGroup = false;
    bool backgroundMode = false;
    std::string cliFile;
    std::string cliImage;
    std::string cliProfile = "auto";
    std::string cliEndpoint;
    std::string cliModel;

    auto requireValue = [&](int& i, const std::string& option) -> std::string {
        if (i + 1 >= argc) {
            std::cerr << "logsift: " << option << " requires a value\n"
                      << "Try 'logsift --help'.\n";
            return {};
        }
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintCliHelp();
            return 0;
        }
        if (arg == "--version") {
            std::cout << "Log Sift " << LOGSIFT_VERSION << "\n";
            return 0;
        }
        if (arg == "--background") {
            backgroundMode = true;
            continue;
        }
        if (arg == "--cli") {
            cliMode = true;
            continue;
        }
        if (arg == "--json") {
            cliMode = true;
            cliJson = true;
            continue;
        }
        if (arg == "--llm") {
            cliMode = true;
            cliUseLlm = true;
            continue;
        }
        if (arg == "--list-profiles") {
            cliMode = true;
            cliListProfiles = true;
            continue;
        }
        if (arg == "--timestamps") {
            cliMode = true;
            cliShowTimestamps = true;
            continue;
        }
        if (arg == "--known-noise") {
            cliMode = true;
            cliKnownNoise = true;
            continue;
        }
        if (arg == "--no-group") {
            cliMode = true;
            cliNoGroup = true;
            continue;
        }
        if (arg == "--file") {
            cliMode = true;
            cliFile = requireValue(i, arg);
            if (cliFile.empty() && i >= argc - 1) return 2;
            continue;
        }
        if (arg == "--image") {
            cliMode = true;
            cliUseLlm = true;
            cliImage = requireValue(i, arg);
            if (cliImage.empty() && i >= argc - 1) return 2;
            continue;
        }
        if (arg == "--profile") {
            cliMode = true;
            cliProfile = requireValue(i, arg);
            if (cliProfile.empty() && i >= argc - 1) return 2;
            continue;
        }
        if (arg == "--endpoint") {
            cliMode = true;
            cliEndpoint = requireValue(i, arg);
            if (cliEndpoint.empty() && i >= argc - 1) return 2;
            continue;
        }
        if (arg == "--model") {
            cliMode = true;
            cliModel = requireValue(i, arg);
            if (cliModel.empty() && i >= argc - 1) return 2;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            if (arg == "-") {
                cliMode = true;
                if (!cliFile.empty()) {
                    std::cerr << "logsift: multiple input files specified\n";
                    return 2;
                }
                cliFile = "-";
                continue;
            }
            std::cerr << "logsift: unknown option '" << arg << "'\n"
                      << "Try 'logsift --help'.\n";
            return 2;
        }

        cliMode = true;
        if (!cliFile.empty()) {
            std::cerr << "logsift: multiple input files specified\n";
            return 2;
        }
        cliFile = arg;
    }

    LoadProfiles(argc > 0 ? argv[0] : nullptr);

    if (cliListProfiles) {
        std::cout << "auto\tAutomatic detection\n";
        for (const auto& profile : gProfiles)
            std::cout << profile.id << "\t" << profile.name << "\n";
        return 0;
    }

    if (cliMode) {
        if (!cliImage.empty() && !cliFile.empty()) {
            std::cerr << "logsift: --image cannot be combined with --file or a positional log file\n";
            return 2;
        }

        if (cliProfile != "auto" && !FindProfile(cliProfile)) {
            std::cerr << "logsift: unknown profile '" << cliProfile << "'\n"
                      << "Use --list-profiles to see installed profile IDs.\n";
            return 2;
        }

        Config cliCfg;
        LoadConfig(cliCfg);
        cliCfg.profileId = cliProfile;
        cliCfg.showTimestamps = cliShowTimestamps;
        cliCfg.showKnownNoise = cliKnownNoise;
        cliCfg.groupDiagnostics = !cliNoGroup;
        if (!cliEndpoint.empty()) cliCfg.endpoint = cliEndpoint;
        if (!cliModel.empty()) cliCfg.model = cliModel;

        std::string source;
        std::string result;
        std::string questionable;
        std::string route = "Local deterministic";
        std::string modelError;
        SiftResult modelResult;
        OcrPassStats cliOcr;

        if (!cliImage.empty()) {
            ClipboardImage image;
            std::string imageStatus;
            if (!LoadImageFile(cliImage.c_str(), image, imageStatus)) {
                std::cerr << "logsift: "
                          << (imageStatus.empty()
                              ? "unsupported image; use PNG, JPG, or JPEG"
                              : imageStatus)
                          << "\n";
                return 2;
            }

            try {
                modelResult = SendClipboardImage(
                    cliCfg, image, kDefaultPrompt, nullptr);
            } catch (const std::exception& e) {
                std::cerr << "logsift: OCR/model request failed: "
                          << e.what() << "\n";
                return 3;
            }

            if (modelResult.visionFailure) {
                std::cerr << "logsift: OCR/model request failed: "
                          << modelResult.note << "\n";
                return 3;
            }

            source = modelResult.sourceText;
            result = ApplyOutputPreferences(modelResult.text, cliCfg);
            route = modelResult.route;
            cliOcr = modelResult.ocr;

            if (!source.empty()) {
                const DiagnosticSplit split =
                    LooksLikeUnrealLog(source) &&
                    (cliCfg.profileId == "auto" ||
                     cliCfg.profileId == "unreal")
                        ? SplitUnrealDiagnostics(source, cliCfg)
                        : SplitWithProfile(source, cliCfg);
                questionable =
                    ApplyOutputPreferences(split.questionable, cliCfg);
            }
        } else {
            std::ostringstream ss;
            if (!cliFile.empty() && cliFile != "-") {
                std::ifstream in(cliFile, std::ios::binary);
                if (!in) {
                    std::cerr << "logsift: cannot open "
                              << cliFile << "\n";
                    return 2;
                }
                ss << in.rdbuf();
            } else {
                ss << std::cin.rdbuf();
            }
            source = ss.str();
            if (source.empty()) {
                std::cerr << "logsift: empty input\n";
                return 2;
            }

            const std::string filtered = PreFilter(source, cliCfg);
            const DiagnosticSplit split =
                LooksLikeUnrealLog(source) &&
                (cliCfg.profileId == "auto" ||
                 cliCfg.profileId == "unreal")
                    ? SplitUnrealDiagnostics(source, cliCfg)
                    : SplitWithProfile(source, cliCfg);
            questionable =
                ApplyOutputPreferences(split.questionable, cliCfg);
            const std::string candidate =
                !split.included.empty() ? split.included : filtered;

            if (cliUseLlm) {
                try {
                    modelResult = Send(
                        cliCfg, source, kDefaultPrompt, nullptr);
                    result =
                        ApplyOutputPreferences(modelResult.text, cliCfg);
                    route = modelResult.route;
                } catch (const std::exception& e) {
                    modelError = e.what();
                    result = LooksLikeStructuredBuildDiagnostics(candidate)
                        ? FastStructuredResult(candidate)
                        : candidate;
                    result = ApplyOutputPreferences(result, cliCfg);
                    route = "Local fallback";
                    if (!cliJson) {
                        std::cerr
                            << "logsift: model request failed; using local fallback: "
                            << modelError << "\n";
                    }
                }
            } else {
                const bool structured =
                    LooksLikeStructuredBuildDiagnostics(candidate);
                result = structured
                    ? FastStructuredResult(candidate)
                    : candidate;
                result = ApplyOutputPreferences(result, cliCfg);
                route = structured
                    ? "Deterministic fast path"
                    : "Local prefilter";
            }

            if (result.empty())
                result = ApplyOutputPreferences(candidate, cliCfg);
        }

        const std::string filteredForStats =
            source.empty() ? std::string{} : PreFilter(source, cliCfg);
        const auto [inputLines, inputWords] = HumanTextStats(source);
        const auto [filteredLines, filteredWords] =
            HumanTextStats(filteredForStats);
        const size_t diagnosticCount =
            DiagnosticEntries(result).size();
        const bool hasDiagnostics =
            !result.empty() &&
            result != "NO_DIAGNOSTICS" &&
            result != "NO_DIAGNOSTICS\n";

        if (cliJson) {
            json j;
            j["version"] = LOGSIFT_VERSION;
            j["source"] =
                !cliImage.empty() ? "image" :
                (!cliFile.empty() && cliFile != "-" ? "file" : "stdin");
            j["type"] =
                source.empty() ? "Unknown" : DetectLogType(source);
            j["profile"] =
                source.empty()
                    ? cliCfg.profileId
                    : ProfileName(source, cliCfg);
            j["route"] = route;
            j["input_bytes"] = source.size();
            j["input_lines"] = inputLines;
            j["input_words"] = inputWords;
            j["filtered_bytes"] = filteredForStats.size();
            j["filtered_lines"] = filteredLines;
            j["filtered_words"] = filteredWords;
            j["estimated_input_tokens"] =
                EstimateTokenCount(source);
            j["estimated_filtered_tokens"] =
                EstimateTokenCount(filteredForStats);
            j["diagnostic_count"] = diagnosticCount;
            j["diagnostics"] = result;
            j["questionable"] = questionable;

            if (cliUseLlm || !cliImage.empty()) {
                j["model"] = cliCfg.model;
                j["endpoint"] = cliCfg.endpoint;
                j["prompt_tokens"] = modelResult.promptTokens;
                j["completion_tokens"] =
                    modelResult.completionTokens;
                j["prompt_tokens_per_second"] =
                    modelResult.promptTokensPerSecond;
                j["completion_tokens_per_second"] =
                    modelResult.completionTokensPerSecond;
            }
            if (!modelError.empty())
                j["model_error"] = modelError;
            if (cliOcr.present) {
                j["ocr"] = {
                    {"mime_type", cliOcr.mimeType},
                    {"image_bytes", cliOcr.imageBytes},
                    {"output_bytes", cliOcr.outputBytes},
                    {"output_lines", cliOcr.outputLines},
                    {"output_words", cliOcr.outputWords},
                    {"prompt_tokens", cliOcr.promptTokens},
                    {"completion_tokens", cliOcr.completionTokens},
                    {"prompt_tokens_per_second",
                        cliOcr.promptTokensPerSecond},
                    {"completion_tokens_per_second",
                        cliOcr.completionTokensPerSecond},
                    {"seconds", cliOcr.seconds}
                };
            }
            std::cout << j.dump(2) << "\n";
        } else if (hasDiagnostics) {
            std::cout << result;
            if (!result.empty() && result.back() != '\n')
                std::cout << '\n';
        }

        if (!modelError.empty())
            return 3;
        return hasDiagnostics ? 0 : 1;
    }
