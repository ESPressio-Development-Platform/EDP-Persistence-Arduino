# Build, Test and Source Map

C++20 and Arduino are required. `docs/PROVIDERS.MD` and `docs/BUILDING.MD` define provider behaviour. Tests cover path/key limits, empty values, truncation, property metadata and compile-time provider contracts; demos exercise mounted filesystem and Preferences integrations.


Arduino-facing compile validation also verifies that both concrete providers discover EDP-Memory through its public root header rather than cross-library `src/memory/...` includes.
