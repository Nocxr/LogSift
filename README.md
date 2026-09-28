# Log Sift

Standalone desktop utility for reducing noisy build and runtime logs to the diagnostics that matter.

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

The executable target is `logsift`. Bundled profiles are copied beside the executable at build time and seeded into the user's editable profile folder on first run.

## User data

Log Sift keeps persistent user data in one dot-folder:

- macOS: `~/.logsift/`
- Windows: `%USERPROFILE%\.logsift\`

The folder contains:

- `settings.json` — connection, filtering, notification, clipboard, and sound settings.
- `profiles/` — editable log profiles. Missing bundled profiles are seeded here without overwriting existing user copies.

## Desktop behavior

- Optional start-at-login on macOS and Windows. Login launches start hidden in the menu bar/tray.
- Clipboard watch with local deterministic filtering and optional LLM refinement.
- Optional automatic copying of actionable results only; empty / `NO_DIAGNOSTICS` results are never copied.
- Separate configurable sounds for scan start, completion, offline fallback, and failure.
- Offline fallback remains available when the configured model endpoint cannot be reached.

## Platforms

- Windows: tray integration, event-driven clipboard watching, notifications, optional custom WAV completion sound.
- macOS: Cocoa menu-bar integration and lightweight pasteboard change-count watching.
