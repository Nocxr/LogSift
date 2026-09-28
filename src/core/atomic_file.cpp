#include "atomic_file.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

bool WriteFileAtomically(const std::filesystem::path& target,
                         const std::string& content) {
    std::error_code ec;
    std::filesystem::create_directories(target.parent_path(), ec);
    if (ec) {
        std::fprintf(stderr, "logsift: could not create file directory: %s\n",
                     ec.message().c_str());
        return false;
    }

    // Write in the destination directory, then replace the old file. A failed
    // write leaves the previous settings intact.
    static std::atomic<unsigned long long> sequence{0};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path temporary = target;
    temporary += ".tmp-" + std::to_string(nonce) +
        "-" + std::to_string(sequence.fetch_add(1));
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out || !(out << content) || !(out.flush())) {
            std::fprintf(stderr, "logsift: could not write file\n");
            std::filesystem::remove(temporary, ec);
            return false;
        }
        out.close();
        if (!out) {
            std::fprintf(stderr, "logsift: could not close file\n");
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }
#ifndef _WIN32
    std::filesystem::permissions(temporary,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace, ec);
    if (ec) {
        std::fprintf(stderr, "logsift: could not set file permissions: %s\n",
                     ec.message().c_str());
        std::filesystem::remove(temporary, ec);
        return false;
    }
    std::filesystem::rename(temporary, target, ec);
#else
    if (!MoveFileExW(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
#endif
    if (ec) {
        std::fprintf(stderr, "logsift: could not replace file: %s\n",
                     ec.message().c_str());
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return true;
}
