// Activity log formatting and bounded retention.
// Included by main.cpp; keep this module focused on this responsibility.

std::string ActivityClockTime() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S");
    return out.str();
}

void AppendActivityLog(std::string& log, const char* level, const std::string& message) {
    log += "[" + ActivityClockTime() + "] [" + level + "] " + message + "\n";

    // Keep the in-app activity log useful without allowing it to grow forever.
    constexpr size_t kMaxActivityLogBytes = 256 * 1024;
    if (log.size() > kMaxActivityLogBytes) {
        const size_t trimTarget = log.size() - (kMaxActivityLogBytes * 3 / 4);
        const size_t nextLine = log.find('\n', trimTarget);
        if (nextLine != std::string::npos)
            log.erase(0, nextLine + 1);
    }
}

