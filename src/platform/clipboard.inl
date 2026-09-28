// Cross-platform clipboard text/image access.
// Included by main.cpp; keep this module focused on this responsibility.

bool SetOwnedClipboardText(const std::string& text, std::string* lastClipboardText = nullptr) {
#ifdef _WIN32
    gIgnoreNextClipboardUpdate = true;
#endif
    if (!SDL_SetClipboardText(text.c_str())) {
#ifdef _WIN32
        gIgnoreNextClipboardUpdate = false;
#endif
        return false;
    }
    if (lastClipboardText) *lastClipboardText = text;
    return true;
}

struct ClipboardImage {
    std::string mimeType;
    std::vector<unsigned char> bytes;
};

bool GetClipboardImage(ClipboardImage& image) {
    // SDL exposes native PNG clipboard data on Windows/macOS and generic MIME
    // clipboard data where available. Prefer PNG, then JPEG aliases.
    static constexpr const char* kImageMimeTypes[] = {
        "image/png",
        "image/jpeg",
        "image/jpg"
    };

    for (const char* mime : kImageMimeTypes) {
        if (!SDL_HasClipboardData(mime)) continue;

        size_t size = 0;
        void* raw = SDL_GetClipboardData(mime, &size);
        if (!raw || size == 0) {
            if (raw) SDL_free(raw);
            continue;
        }

        const auto* begin = static_cast<const unsigned char*>(raw);
        image.mimeType = mime;
        image.bytes.assign(begin, begin + size);
        SDL_free(raw);
        return true;
    }
    return false;
}

