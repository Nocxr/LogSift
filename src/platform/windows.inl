// Windows tray, icons, native notifications, startup registration.
// Included by main.cpp; keep this module focused on this responsibility.

#ifdef _WIN32
constexpr UINT kTrayMessage = WM_APP + 42;
constexpr UINT kSingleInstanceMessage = WM_APP + 43;
constexpr UINT kTrayId = 1;
HANDLE gSingleInstanceMutex = nullptr;
HWND gTrayHwnd = nullptr;
NOTIFYICONDATAW gTrayIcon{};
bool gTrayRestoreRequested = false;
bool gTrayExitRequested = false;
bool gTrayCopyRequested = false;
bool gTrayWatchToggleRequested = false;
bool gTrayOcrToggleRequested = false;
bool gTrayAutoCopyToggleRequested = false;
bool gTraySoundToggleRequested = false;
bool gTrayOpenLogRequested = false;
bool gTrayWatchEnabled = true;
bool gTrayOcrEnabled = true;
bool gTrayAutoCopyEnabled = false;
bool gTraySoundEnabled = true;
bool gClipboardUpdatePending = false;
bool gIgnoreNextClipboardUpdate = false;
HICON gAppIconSmall = nullptr;
HICON gAppIconBig = nullptr;

HICON CreateLogSiftHIcon(int size) {
    if (size < 16) size = 16;

    BITMAPV5HEADER bi{};
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = size;
    bi.bV5Height = -size;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    void* raw = nullptr;
    HDC dc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&bi),
        DIB_RGB_COLORS, &raw, nullptr, 0);
    ReleaseDC(nullptr, dc);
    if (!color || !raw) return nullptr;

    auto* pixels = static_cast<unsigned int*>(raw);
    std::fill(pixels, pixels + size * size, 0u);

    auto rgba = [](unsigned r, unsigned g, unsigned b, unsigned a = 255u) {
        return (a << 24) | (r << 16) | (g << 8) | b;
    };
    auto put = [&](int x, int y, unsigned int c) {
        if (x >= 0 && y >= 0 && x < size && y < size) pixels[y * size + x] = c;
    };
    auto rect = [&](float x0, float y0, float x1, float y1, unsigned int c) {
        const int ax = static_cast<int>(x0 * size), ay = static_cast<int>(y0 * size);
        const int bx = static_cast<int>(x1 * size), by = static_cast<int>(y1 * size);
        for (int y = ay; y < by; ++y) for (int x = ax; x < bx; ++x) put(x, y, c);
    };
    auto disc = [&](float cx, float cy, float rr, unsigned int c) {
        const int r = std::max(1, static_cast<int>(rr * size));
        const int x0 = static_cast<int>(cx * size), y0 = static_cast<int>(cy * size);
        for (int y = -r; y <= r; ++y) for (int x = -r; x <= r; ++x)
            if (x*x + y*y <= r*r) put(x0 + x, y0 + y, c);
    };

    const unsigned int bg = rgba(9, 22, 31);
    const unsigned int border = rgba(25, 45, 59);
    const unsigned int gray = rgba(112, 132, 149);
    const unsigned int dim = rgba(60, 75, 87);
    const unsigned int red = rgba(255, 66, 44);
    const unsigned int green = rgba(24, 225, 102);
    const unsigned int sieve = rgba(177, 199, 216);

    rect(0.05f, 0.05f, 0.95f, 0.95f, border);
    rect(0.09f, 0.09f, 0.91f, 0.91f, bg);

    rect(0.16f,0.23f,0.40f,0.27f,gray); rect(0.16f,0.33f,0.34f,0.37f,red);
    rect(0.16f,0.43f,0.42f,0.47f,gray); rect(0.16f,0.53f,0.31f,0.57f,dim);
    rect(0.16f,0.63f,0.38f,0.67f,red);  rect(0.16f,0.73f,0.41f,0.77f,gray);
    rect(0.61f,0.24f,0.82f,0.28f,gray); rect(0.64f,0.36f,0.84f,0.40f,dim);
    rect(0.67f,0.50f,0.85f,0.55f,green);rect(0.67f,0.62f,0.85f,0.67f,green);
    rect(0.61f,0.75f,0.84f,0.79f,gray);

    for (int i = 0; i <= 36; ++i) {
        const float t = i / 36.0f;
        const float y = 0.18f + 0.64f * t;
        const float x = 0.49f + 0.075f * std::sin(t * 6.2831853f);
        disc(x, y, 0.035f, sieve);
    }

    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = color;
    ii.hbmMask = mask;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(mask);
    DeleteObject(color);
    return icon;
}

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == kSingleInstanceMessage) {
        gTrayRestoreRequested = true;
        return 0;
    }
    if (msg == WM_CLIPBOARDUPDATE) {
        if (gIgnoreNextClipboardUpdate) {
            gIgnoreNextClipboardUpdate = false;
            gClipboardUpdatePending = false;
            return 0;
        }
        gClipboardUpdatePending = true;
        return 0;
    }
    if (msg == kTrayMessage && wParam == kTrayId) {
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK || lParam == NIN_BALLOONUSERCLICK) gTrayRestoreRequested = true;
        if (lParam == WM_RBUTTONUP) {
            POINT pt{}; GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, 1, L"Open");
            AppendMenuW(menu, MF_STRING | (gTrayWatchEnabled ? MF_CHECKED : MF_UNCHECKED), 4, L"Watch Clipboard");
            AppendMenuW(menu, MF_STRING | (gTrayOcrEnabled ? MF_CHECKED : MF_UNCHECKED), 8, L"OCR");
            AppendMenuW(menu, MF_STRING | (gTrayAutoCopyEnabled ? MF_CHECKED : MF_UNCHECKED), 5, L"Auto Copy");
            AppendMenuW(menu, MF_STRING | (gTraySoundEnabled ? MF_CHECKED : MF_UNCHECKED), 7, L"Sound");
            AppendMenuW(menu, MF_STRING, 6, L"Open Log");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, 3, L"Exit");
            SetForegroundWindow(hwnd);
            const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(menu);
            if (cmd == 1) gTrayRestoreRequested = true;
            if (cmd == 3) gTrayExitRequested = true;
            if (cmd == 4) gTrayWatchToggleRequested = true;
            if (cmd == 8) gTrayOcrToggleRequested = true;
            if (cmd == 5) gTrayAutoCopyToggleRequested = true;
            if (cmd == 6) gTrayOpenLogRequested = true;
            if (cmd == 7) gTraySoundToggleRequested = true;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool InitTrayIcon() {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.lpfnWndProc = TrayWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"LogSiftTrayWindow";
    RegisterClassW(&wc);
    gTrayHwnd = CreateWindowExW(0, wc.lpszClassName, L"Log Sift Tray", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
    if (!gTrayHwnd) return false;
    AddClipboardFormatListener(gTrayHwnd);
    gTrayIcon.cbSize = sizeof(gTrayIcon);
    gTrayIcon.hWnd = gTrayHwnd;
    gTrayIcon.uID = kTrayId;
    gTrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    gTrayIcon.uCallbackMessage = kTrayMessage;
    gTrayIcon.hIcon = gAppIconSmall ? gAppIconSmall : LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    wcscpy_s(gTrayIcon.szTip, L"Log Sift");
    return Shell_NotifyIconW(NIM_ADD, &gTrayIcon) != FALSE;
}

std::string PickWaveFile(HWND owner) {
    wchar_t path[32768]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Wave audio (*.wav)\0*.wav\0All files (*.*)\0*.*\0\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = static_cast<DWORD>(std::size(path));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = L"wav";
    if (!GetOpenFileNameW(&ofn)) return {};
    return std::filesystem::path(path).string();
}

void ShowSystemToast(const std::string& title, const std::string& body) {
    if (!gTrayHwnd) return;
    NOTIFYICONDATAW n = gTrayIcon;
    n.uFlags = NIF_INFO;
    std::wstring wt(title.begin(), title.end()), wb(body.begin(), body.end());
    wcsncpy_s(n.szInfoTitle, wt.c_str(), _TRUNCATE);
    wcsncpy_s(n.szInfo, wb.c_str(), _TRUNCATE);
    n.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &n);
}

bool WindowsGetStartAtLogin() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    wchar_t value[32768]{};
    DWORD type = 0;
    DWORD bytes = sizeof(value);
    const LONG rc = RegQueryValueExW(key, L"Log Sift", nullptr, &type,
        reinterpret_cast<BYTE*>(value), &bytes);
    RegCloseKey(key);
    return rc == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ);
}

bool WindowsSetStartAtLogin(bool enabled) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;

    LONG rc = ERROR_SUCCESS;
    if (enabled) {
        wchar_t exePath[32768]{};
        const DWORD len = GetModuleFileNameW(nullptr, exePath, static_cast<DWORD>(std::size(exePath)));
        if (len == 0 || len >= std::size(exePath)) {
            RegCloseKey(key);
            return false;
        }
        std::wstring command = L"\"" + std::wstring(exePath, len) + L"\" --background";
        rc = RegSetValueExW(key, L"Log Sift", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        rc = RegDeleteValueW(key, L"Log Sift");
        if (rc == ERROR_FILE_NOT_FOUND) rc = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return rc == ERROR_SUCCESS;
}
#endif

