# EDP-Persistence-Arduino

Concrete Arduino-ESP32 providers for the EDP-Persistence contract.

This repository is the explicit Arduino concrete sibling of the SDK-neutral `EDP-Persistence` repository. Its direct Arduino SDK references are permitted by the domain-specific exception in `Policy/V2/ABSTRACTION.MD`.

## Providers

- `FileSystemStorage<TBindingTag, TBindingProfile, TByteOperationsProvider>` adapts an already-mounted hierarchical Arduino `fs::FS` binding to `FileStorage`.
- `PreferencesKeyValueStorage<TBindingTag, TByteOperationsProvider>` adapts one dedicated Arduino Preferences namespace to `KeyValueStorage`.

`TBindingTag` gives independently selectable logical bindings distinct Composition identities.

`TByteOperationsProvider` must satisfy `EDP-Memory::ByteOperations`. The provider is supplied by Bootstrap, held non-owningly, and must outlive the Persistence provider. All raw byte copies performed by these adapters route through that Memory capability.

## Documentation

- [Concrete provider contracts](./docs/PROVIDERS.MD) — provider semantics, lifecycle, ownership, Memory integration, ISR boundary, failure guarantees, and Composition integration.
- [Build baseline](./docs/BUILDING.MD) — mandatory C++20 baseline, PIOArduino recommendation, and exact PlatformIO-compatible configuration.
- [Concrete Providers demo](./demos/ConcreteProviders/README.MD) — hardware expectations and target-specific build surfaces.

## Dependencies

- EDP-Persistence-Arduino
  - EDP-Persistence **[mandatory]**
    - EDP-System **[transitive]**
  - EDP-Memory **[mandatory]**
    - EDP-System **[transitive]**
    - EDP-Platform **[transitive]**
  - Arduino-ESP32 **[mandatory concrete SDK]**
  - EDP-Platform-Portable **[optional: supplied tests/demos use its standard ByteOperations provider]**

A consuming application may replace EDP-Platform-Portable with any provider satisfying `EDP-Memory::ByteOperations`.

See [DEPENDENCY_GRAPH.MD](./DEPENDENCY_GRAPH.MD) for the maintained dependency graph.

## Filesystem binding profile

A filesystem binding profile is a compile-time assertion about the specific mounted filesystem, not about Arduino `fs::FS` in general.

For example:

```cpp
using ByteOperationsProvider =
    ESPressio::Platform::Portable::Memory::ByteOperationsProvider;

using StorageProfile = ESPressio::Persistence::Arduino::FileSystemBindingProfile<
    ESPressio::Persistence::RetentionLevel::Restart,
    ESPressio::Persistence::TextCaseSensitivity::CaseSensitive,
    ESPressio::Persistence::MediaRemovability::Fixed,
    254U,
    254U,
    0xFFFFFFFFULL,
    ESPressio::Persistence::InvocationConcurrency::ConcurrentReads
>;

using StorageProvider =
    ESPressio::Persistence::Arduino::FileSystemStorage<
        MyBinding,
        StorageProfile,
        ByteOperationsProvider
    >;
```

Only select values which are true for the mounted substrate. A profile claiming `PowerLoss` means every successful mutation performed through the provider has crossed a power-loss-surviving commit boundary.

The final `InvocationConcurrency` template argument is optional and defaults to `CallerSerialized`, preserving the conservative behavior of existing profiles. Set it to `ConcurrentReads` only when the mounted filesystem genuinely permits safe concurrent read-side invocations. The provider adds no lock or runtime state for this guarantee; it advertises the binding fact exactly as supplied. Existing custom binding-profile types which do not expose a concurrency field also remain conservatively `CallerSerialized`.

## Lifecycle

The application must mount the selected filesystem before constructing `FileSystemStorage`. The filesystem object, mount, and ByteOperations provider must all remain valid for the provider's complete lifetime.

For Preferences, construct the provider with the selected ByteOperations provider, then call `Begin(namespace)` during Bootstrap. `Begin()` returns `PreferencesBeginStatus::Succeeded` or `PreferencesBeginStatus::ProviderFailure`. Call `End()` during teardown.

The bound Preferences namespace is dedicated to this provider while open because the provider reserves its native representation for logical values, including its present-zero-length marker.

## Concurrency and memory

`FileSystemStorage` advertises the concurrency guarantee supplied by its binding profile. Standard profiles default to `CallerSerialized`; a binding may explicitly advertise `ConcurrentReads` or `ConcurrentOperations` only when that is true for the mounted substrate. This changes no runtime RAM or locking behavior.

`PreferencesKeyValueStorage` remains `CallerSerialized`.

Both providers perform no per-operation dynamic allocation themselves.

Preferences contains one bounded 512-byte scratch buffer because native NVS retrieval cannot partially retrieve a blob. Truncated-copy transfer from that scratch buffer uses the injected EDP-Memory ByteOperations provider.

## ISR boundary

These providers are not ISR-safe and make no ISR-safety guarantee. Do not perform Persistence filesystem, Preferences, lifecycle, enumeration, read, or mutation work from an ISR. Capture only minimal ISR-local state and defer Persistence work to ordinary execution context.

## Build baseline

C++20 is mandatory. New Arduino builds should use PIOArduino. See [Build baseline](./docs/BUILDING.MD) for the exact supported configuration.

## Validation

From the repository root, with sibling `EDP-Persistence`, `EDP-Memory`, `EDP-Platform`, `EDP-Platform-Portable`, and `EDP-System` checkouts present:

```bash
python3 tests/run_tests.py
```

The runner validates:

1. complete concrete provider behavior/lifecycle coverage;
2. PlatformIO Arduino demo source;
3. PlatformIO ESP-IDF demo source;
4. the concrete providers against the installed real Arduino-ESP32 SDK/toolchain.

The runner also accepts explicit `--persistence`, `--memory`, `--platform`, `--platform-portable`, and `--system` paths when sibling discovery is not suitable.
