#pragma once

#include <string>

// Requests keep credentials and JSON in memory; no shell or temporary payload files.
std::string HttpGet(const std::string& url, const std::string& apiKey, long timeoutSeconds);
std::string HttpPostJson(const std::string& url, const std::string& apiKey,
                         const std::string& body, long timeoutSeconds);
