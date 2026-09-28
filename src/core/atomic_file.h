#pragma once

#include <filesystem>
#include <string>

// Replaces a text file after a complete same-directory write.
bool WriteFileAtomically(const std::filesystem::path& target,
                         const std::string& content);
