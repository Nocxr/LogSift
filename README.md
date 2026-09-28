# Log Sift

Copy a noisy build log; get back the errors and warnings worth reading. Log Sift filters locally first. You can also send the trimmed log to an OpenAI-compatible model, such as one running in LM Studio. If the model is offline, local filtering still works.

It runs in the Windows tray or macOS menu bar and can watch the clipboard. You can also drop a log file, paste text, or use the CLI.

![Log Sift main window with an Unreal Engine log](docs/screenshots/main.png)

## Start here

1. Build and open the app using the commands below.
2. Copy a build or runtime log. Clipboard watching is on by default.
3. Open the result popup to copy an individual diagnostic or the whole result.

A model is optional for text logs. To use one, set the **Endpoint** and **Model** in the app, then click **Check Model**. The default endpoint is LM Studio at `http://127.0.0.1:1234/v1/chat/completions`.

## Build and run

You need CMake 3.25+, Ninja, a C++20 compiler, and Git. CMake fetches SDL3, Dear ImGui, and nlohmann/json. Windows also builds libcurl; macOS uses the system libcurl.

**Windows (PowerShell)**

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
.\build\logsift.exe
```

**macOS**

```bash
cmake -S . -B build-mac -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac
ctest --test-dir build-mac --output-on-failure
open "build-mac/Log Sift.app"
```

Closing the main window leaves the tray or menu bar app running. Launching it again brings the existing window forward.

## Images and OCR

A vision-capable model can read a PNG or JPEG screenshot from the clipboard or a dropped file. Log Sift then filters the extracted text like any other log. Clipboard images can scan automatically or show a confirmation popup before the image is sent to the configured endpoint.

| Before OCR | Result |
| --- | --- |
| ![Image detected prompt](docs/screenshots/ocr-prompt.png) | ![OCR result popup](docs/screenshots/ocr-result.png) |

Text-only models still work for text logs. **Check Model** tests image support separately.

## Recents

Recent inputs and results are saved locally. The default limit is five; you can change it in Settings. The Recents tab lets you reload, copy, or remove an entry.

![Recents tab](docs/screenshots/recents.png)

## CLI

Use the built executable directly, or put it on your `PATH` to use `logsift` as below.

```bash
logsift --cli build.log                 # local filtering
cat build.log | logsift --cli -         # stdin
logsift --cli build.log --json          # structured result
logsift --cli build.log --llm           # use the saved model settings
logsift --image screenshot.png --json   # OCR, then filter
logsift --list-profiles
```

Run `logsift --help` for all options. CLI exit codes are `0` for diagnostics, `1` for none, `2` for invalid input, and `3` for a requested model or OCR failure.

## Files on your machine

Settings, recents, and editable JSON log profiles live in `%USERPROFILE%\.logsift\` on Windows or `~/.logsift/` on macOS. Bundled profiles are copied there on first run; updates do not overwrite your edits.

If you enter an API key, it is saved in plain text in `settings.json`. Screenshot OCR sends the image to your configured model endpoint; the confirmation option lets you decide before each clipboard image is sent.
