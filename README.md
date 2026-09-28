# Log Sift

Standalone desktop utility for reducing noisy build and runtime logs to the diagnostics that matter.

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
```

The executable target is `logsift`. Log profiles are copied beside the executable as `log-sift-profiles` after each build.

## Platforms

- Windows: tray integration, clipboard watching, notifications, optional WAV notification sound.
- macOS: menu-bar integration through Cocoa.
