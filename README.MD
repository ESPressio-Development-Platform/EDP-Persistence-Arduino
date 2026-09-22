# EDP-Persistence-Arduino

Concrete Arduino-ESP32 providers for the EDP-Persistence contract.

## Providers

- `FileSystemStorage<TBindingTag, TBindingProfile>` adapts an already-mounted hierarchical Arduino `fs::FS` binding to `FileStorage`. The binding profile explicitly declares retention, path case sensitivity, media removability, path/segment limits, and maximum file size. This provider is intentionally not a universal claim about every `fs::FS` substrate: use it only when the bound filesystem actually supplies the advertised hierarchical directory, rename, append, and bounded write-at operations.
- `PreferencesKeyValueStorage<TBindingTag>` adapts one dedicated Arduino Preferences namespace to `KeyValueStorage`. NVS keys are represented using the documented ASCII key form; valid EDP UTF-8 keys outside ASCII return `KeyNotRepresentable`. Zero-length logical values are represented internally by a reserved Preferences type/value marker because `Preferences::putBytes()` rejects zero-length blobs. Non-empty logical values remain NVS blobs.

`TBindingTag` is intentionally part of each provider type so independently selectable logical bindings can have distinct Composition identities.

## Documentation

- [Concrete provider contracts](./docs/PROVIDERS.MD) — provider semantics, lifecycle, ownership, memory, failure guarantees and Composition integration.

## Dependencies

- EDP-Persistence-Arduino
  - EDP-Persistence
    - EDP-System
  - Arduino-ESP32

[Dependency graph](./DEPENDENCY_GRAPH.MD)

## Filesystem binding profile

A filesystem binding profile is part of the provider type. It is a compile-time assertion about the specific mounted filesystem, not about Arduino `fs::FS` in general.

For example:

```cpp
using StorageProfile = ESPressio::Persistence::Arduino::FileSystemBindingProfile<
    ESPressio::Persistence::RetentionLevel::Restart,
    ESPressio::Persistence::TextCaseSensitivity::CaseSensitive,
    ESPressio::Persistence::MediaRemovability::Fixed,
    254U,
    254U,
    0xFFFFFFFFULL
>;

using StorageProvider =
    ESPressio::Persistence::Arduino::FileSystemStorage<MyBinding, StorageProfile>;
```

Only select values that are true for the actual mounted substrate. A profile claiming `PowerLoss` means every successful mutation performed through the provider has crossed a power-loss-surviving commit boundary. The generic adapter cannot infer that guarantee from `fs::FS`.

## Lifecycle

The providers do not take ownership of platform lifecycle. The application must mount the selected filesystem before constructing/using `FileSystemStorage` and keep that mount and `fs::FS` object valid for the provider's complete use lifetime. For Preferences, call `Begin(namespace)` during Bootstrap and `End()` during teardown. The bound Preferences namespace is provider-owned for its use lifetime: other code must not insert native Preferences values into the same namespace because the provider reserves its native representation (including the marker used for a present zero-length logical value).

## Concurrency and memory

Both providers advertise `CallerSerialized`. They perform no per-operation dynamic allocation themselves. Preferences contains one bounded 512-byte scratch buffer because NVS cannot partially retrieve a blob; it is used only when the caller destination requires truncation.

## Demos

- [Concrete Providers](./demos/ConcreteProviders/README.MD) — exercises both FileStorage and KeyValueStorage concrete providers across the required Arduino IDE, PlatformIO Arduino, and PlatformIO ESP-IDF demo surfaces.

## Validation

Concrete SDK compilation and hardware/runtime tests belong in this repository. The abstract contract is validated independently by EDP-Persistence.

## Local contract compilation

From this repository root, with sibling `EDP-Persistence` and `EDP-System` checkouts present, run:

```bash
python3 tests/run_tests.py
```

The runner performs four validation stages: it compiles and executes `tests/ProviderBehaviorTests.cpp` on the host against bounded Arduino SDK test doubles, source-compiles both PlatformIO demo translation units against bounded demo stubs, then compiles `tests/ContractCompile.cpp` against the real Arduino-ESP32 SDK and real sibling EDP contract repositories. The behavior tests exercise adapter-owned semantics including truncation, size limits, append/write-at, key representability, present zero-length values, and clear/remove behavior. The demo-source stages deliberately bypass PlatformIO package orchestration so C++/provider API validation remains available even when the local PlatformIO package manager is unhealthy. Use `--host-compiler` to override the host compiler and `--keep-build` to retain generated files for diagnosis.
