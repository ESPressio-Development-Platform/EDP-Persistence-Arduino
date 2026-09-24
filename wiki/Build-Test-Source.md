# Build, Test and Source Map

C++20 and Arduino are required. `docs/PROVIDERS.MD` and `docs/BUILDING.MD` define provider behaviour. Tests cover path/key limits, empty values, truncation, property metadata and compile-time provider contracts; demos exercise mounted filesystem and Preferences integrations.


Arduino-facing compile validation also verifies that both concrete providers discover EDP-Memory through its public root header rather than cross-library `src/memory/...` includes.

Arduino IDE / Arduino CLI validation treats EDP-Persistence-Arduino as a normal installed/local library through `library.properties` and its root `ESPressio_Persistence_Arduino.hpp` include. The concrete-provider demo selects portable ByteOperations through the root-level narrow Platform-Portable entry point.
