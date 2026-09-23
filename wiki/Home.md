# EDP-Persistence-Arduino Developer Wiki

EDP-Persistence-Arduino provides Arduino API-backed concrete FileStorage and KeyValueStorage providers over an already-mounted `fs::FS` and a dedicated Arduino Preferences namespace.

This Wiki is maintained beside the code on `main`. Source code and repository `docs/` remain normative; the Wiki is the internal developer explanation/navigation layer and must evolve with code changes.

## Public entry point

```cpp
#include <ESPressio_Persistence_Arduino.hpp>
```

## Dependencies

Mandatory: EDP-Persistence and EDP-Memory.

Use [Architecture](Architecture.md), [Public API](Public-API.md), [Internal API](Internal-API.md), [Implementation](Implementation.md), [Composition](Composition.md), [Resources / Lifecycle / Concurrency](Resources-Lifecycle-Concurrency.md), and [Build / Test / Source](Build-Test-Source.md).
