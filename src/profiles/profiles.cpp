#include "profiles.h"
#include "../config/config_store.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <utility>

using json = nlohmann::json;

std::vector<LogProfile> gProfiles;
std::filesystem::path gProfilesDir;

bool ContainsAny(const std::string& line, const std::vector<std::string>& needles) {
    std::string lowerLine=line;
    std::transform(lowerLine.begin(),lowerLine.end(),lowerLine.begin(),[](unsigned char ch){return (char)std::tolower(ch);});
    for (const auto& s : needles) {
        if (s.empty()) continue;
        std::string lower=s;
        std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char ch){return (char)std::tolower(ch);});
        if (lowerLine.find(lower)!=std::string::npos) return true;
    }
    return false;
}
void LoadProfiles(const char* argv0) {
    gProfiles.clear();
    SeedUserProfiles(argv0);
    gProfilesDir = UserDataDir() / "profiles";
    std::error_code ec;
    if (!std::filesystem::is_directory(gProfilesDir, ec)) return;
    for (const auto& entry : std::filesystem::directory_iterator(gProfilesDir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        try {
            std::ifstream in(entry.path());
            json j; in >> j;
            LogProfile p;
            p.id=j.value("id",entry.path().stem().string()); p.name=j.value("name",p.id);
            p.detect=j.value("detect",std::vector<std::string>{});
            p.highPriority=j.value("high_priority",std::vector<std::string>{});
            p.warnings=j.value("warnings",std::vector<std::string>{});
            p.questionable=j.value("questionable",std::vector<std::string>{});
            p.noise=j.value("noise",std::vector<std::string>{});
            gProfiles.push_back(std::move(p));
        } catch (const std::exception& e) {
            std::fprintf(stderr, "logsift: invalid profile %s: %s\n",
                         entry.path().string().c_str(), e.what());
        }
    }
    std::sort(gProfiles.begin(),gProfiles.end(),[](const LogProfile& a,const LogProfile& b){return a.name<b.name;});
}
const LogProfile* FindProfile(const std::string& id) {
    for (const auto& p:gProfiles) if (p.id==id) return &p;
    return nullptr;
}
std::vector<const LogProfile*> DetectProfiles(const std::string& text, const Config& cfg) {
    if (cfg.profileId == "generic") {
        if (const auto* generic = FindProfile("generic")) return {generic};
        return {};
    }

    if (cfg.profileId != "auto") {
        if (const auto* forced = FindProfile(cfg.profileId)) return {forced};
        // A stale/unknown saved profile must not disable automatic detection.
    }

    struct Match {
        int score = 0;
        const LogProfile* profile = nullptr;
    };
    std::vector<Match> matches;
    for (const auto& p : gProfiles) {
        if (p.id == "generic") continue;
        int score = 0;
        for (const auto& s : p.detect) {
            if (ContainsAny(text, std::vector<std::string>{s})) ++score;
        }
        if (score > 0) matches.push_back({score, &p});
    }

    std::stable_sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.profile->name < b.profile->name;
    });

    std::vector<const LogProfile*> result;
    result.reserve(matches.size());
    for (const Match& match : matches) result.push_back(match.profile);

    if (result.empty()) {
        if (const auto* generic = FindProfile("generic")) result.push_back(generic);
    }
    return result;
}

const LogProfile* DetectProfile(const std::string& text, const Config& cfg) {
    const auto matches = DetectProfiles(text, cfg);
    return matches.empty() ? nullptr : matches.front();
}
