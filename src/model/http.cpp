#include "http.h"

#include <curl/curl.h>

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {
size_t AppendResponse(char* bytes, size_t size, size_t count, void* context) {
    auto& response = *static_cast<std::string*>(context);
    const size_t length = size * count;
    // A model endpoint can return errors, but it should never be able to
    // exhaust the app's memory with an unbounded response.
    constexpr size_t kMaxResponseBytes = 32 * 1024 * 1024;
    if (length > kMaxResponseBytes - response.size()) return 0;
    response.append(bytes, length);
    return length;
}

std::string Request(const std::string& url, const std::string& apiKey,
                    const std::string* body, long timeoutSeconds) {
    static std::once_flag initFlag;
    static CURLcode initResult = CURLE_OK;
    std::call_once(initFlag, [] { initResult = curl_global_init(CURL_GLOBAL_DEFAULT); });
    if (initResult != CURLE_OK) throw std::runtime_error("Could not initialize HTTP client.");

    CURL* raw = curl_easy_init();
    if (!raw) throw std::runtime_error("Could not create HTTP client.");
    struct CurlGuard {
        CURL* handle;
        ~CurlGuard() { curl_easy_cleanup(handle); }
    } guard{raw};

    curl_slist* rawHeaders = nullptr;
    struct HeadersGuard {
        curl_slist*& headers;
        ~HeadersGuard() { curl_slist_free_all(headers); }
    } headers{rawHeaders};

    auto addHeader = [&](const std::string& value) {
        curl_slist* next = curl_slist_append(rawHeaders, value.c_str());
        if (!next) throw std::runtime_error("Could not create HTTP headers.");
        rawHeaders = next;
    };
    if (!apiKey.empty()) addHeader("Authorization: Bearer " + apiKey);
    if (body) addHeader("Content-Type: application/json");

    std::string response;
    char error[CURL_ERROR_SIZE] = {};
    curl_easy_setopt(raw, CURLOPT_URL, url.c_str());
    curl_easy_setopt(raw, CURLOPT_HTTPHEADER, rawHeaders);
    curl_easy_setopt(raw, CURLOPT_ERRORBUFFER, error);
    curl_easy_setopt(raw, CURLOPT_WRITEFUNCTION, AppendResponse);
    curl_easy_setopt(raw, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(raw, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(raw, CURLOPT_TIMEOUT, timeoutSeconds);
    curl_easy_setopt(raw, CURLOPT_NOSIGNAL, 1L);
    if (body) {
        curl_easy_setopt(raw, CURLOPT_POST, 1L);
        curl_easy_setopt(raw, CURLOPT_POSTFIELDSIZE_LARGE,
                         static_cast<curl_off_t>(body->size()));
        curl_easy_setopt(raw, CURLOPT_POSTFIELDS, body->data());
    }

    const CURLcode result = curl_easy_perform(raw);
    if (result != CURLE_OK)
        throw std::runtime_error("curl failed (exit " + std::to_string(static_cast<int>(result)) +
            "): " + (error[0] ? error : curl_easy_strerror(result)));
    long status = 0;
    curl_easy_getinfo(raw, CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300)
        throw std::runtime_error("HTTP " + std::to_string(status) + ": " +
            response.substr(0, 2048));
    return response;
}
} // namespace

std::string HttpGet(const std::string& url, const std::string& apiKey, long timeoutSeconds) {
    return Request(url, apiKey, nullptr, timeoutSeconds);
}

std::string HttpPostJson(const std::string& url, const std::string& apiKey,
                         const std::string& body, long timeoutSeconds) {
    return Request(url, apiKey, &body, timeoutSeconds);
}
