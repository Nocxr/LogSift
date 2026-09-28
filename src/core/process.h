#pragma once

#include <string>
#include <vector>

struct Config;

std::string ShellQuote(const std::string& s);
std::string Base64Encode(const std::vector<unsigned char>& data);
bool IsEndpointUnavailableError(const std::string& message);
std::string ReadPipe(const std::string& command);
std::string FormatDiagnosticText(const std::string& text, const Config& cfg);
std::string DedupeLines(const std::string& text);
