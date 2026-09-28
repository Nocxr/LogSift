# Log Sift source layout

Log Sift's release-polish implementation used to live in a single ~6,400-line `main.cpp`. The source is now split by responsibility while preserving the exact runtime/control-flow behavior of that release snapshot.

## Layout

- `app/` — application setup, event-loop phases, frame pacing, shutdown
- `audio/` — notification audio
- `cli/` — command-line help and execution
- `config/` — configuration schema and persistence
- `core/` — shared processing, stats/history, result types
- `io/` — file/image input
- `model/` — model client, deterministic fast path, OCR/vision
- `platform/` — native platform/clipboard integration
- `profiles/` — profile detection and log prefiltering
- `ui/` — shared widgets, main-window sections, notification popup sections

## Implementation note

The release-polish UI/event loop has a large amount of deliberately shared local runtime state. The processing helpers in `core/process.cpp` and settings persistence in `config/config_store.cpp` compile independently. Their headers declare the shared interfaces, with configuration data in `config/config_types.h`. The UI and event-loop files are still lexical fragments included by the composition root because they share local runtime state. CMake lists those fragments as HEADER_FILE_ONLY so IDEs show the layout without compiling them twice.

The next architectural pass should introduce explicit app state, then move the CLI, model, profiles, and UI sections into independent translation units.
