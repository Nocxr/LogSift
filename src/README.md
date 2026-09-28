# Log Sift source layout

Log Sift's release-polish implementation used to live in a single ~6,400-line `main.cpp`. The source is now split by responsibility with application state owned explicitly and several services compiled independently.

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

The release-polish UI/event loop has a large amount of deliberately shared local runtime state. Processing, model discovery/health, statistics/recents, atomic file writes, and settings persistence compile independently. Their headers declare the shared interfaces, with configuration data in `config/config_types.h`. Desktop runtime values now live in `app/state.inl` as `AppState`. The UI and event-loop files still bind references to that state and are included by the composition root; they have not yet become independent translation units. CMake lists those fragments as HEADER_FILE_ONLY so IDEs show the layout without compiling them twice.

The CLI dispatch is a dedicated function. The HTTP client and popup timer also compile independently. The remaining architectural step is to pass `AppState` to event/UI handlers directly and move the lexical UI fragments into independent translation units.
