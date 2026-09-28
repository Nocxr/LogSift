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

The release-polish UI/event loop has a large amount of deliberately shared local runtime state. These files are lexical implementation modules included by the composition root, so this refactor stays behavior-neutral rather than simultaneously redesigning state ownership during release polish. CMake lists every fragment as HEADER_FILE_ONLY so IDEs expose the real module layout without compiling fragments twice.

A later architectural pass can introduce explicit AppState/services and promote stable fragments to independent .cpp translation units. Keeping that separate makes any runtime regression easy to isolate.
