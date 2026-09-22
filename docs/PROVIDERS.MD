# Concrete Arduino Persistence Providers

## Purpose and scope

EDP-Persistence-Arduino provides Arduino-ESP32 concrete adapters for the platform-neutral EDP-Persistence contract.

This repository is intentionally the platform-specific Arduino concrete sibling of `EDP-Persistence`. Under the domain-specific exception defined by `Policy/V2/ABSTRACTION.MD`, Arduino SDK references which implement Persistence-domain behavior may reside here directly. The domain-neutral `EDP-Persistence` repository remains SDK-free.

Persistence semantics remain defined by EDP-Persistence.

## ByteOperations dependency

Both concrete providers consume one caller-owned provider satisfying `EDP-Memory::ByteOperations`.

The ByteOperations provider is:

- supplied by application Bootstrap;
- held non-owningly by the Persistence provider;
- required to outlive the Persistence provider;
- used for raw byte-copy operations instead of calling Standard C/C++ memory-copy primitives directly.

The supplied tests and demos select `EDP-Platform-Portable::Memory::ByteOperationsProvider`, but an application may select any provider satisfying the EDP-Memory capability contract.

## FileSystemStorage

`FileSystemStorage<TBindingTag, TBindingProfile, TByteOperationsProvider>` adapts one caller-owned, already-mounted hierarchical `fs::FS` object.

The binding profile is part of the provider type and declares the semantic guarantees Composition consumers may rely upon:

- retention;
- path case sensitivity;
- media removability;
- maximum complete path bytes;
- maximum path-segment bytes;
- maximum file size.

The adapter does not infer those guarantees from `fs::FS`. The application must choose a profile that is true for the actual mounted substrate.

The provider structurally offers:

- mandatory FileStorage operations;
- directory creation/removal;
- directory enumeration;
- rename;
- append;
- bounded write-at.

Use a different/specialized provider for a filesystem which does not genuinely support those semantics.

The provider does not mount, format, unmount, or own the filesystem. The `fs::FS` object, filesystem mount, and ByteOperations provider must all outlive the provider.

All FileStorage operations advertise `CallerSerialized`. File mutation failure preservation is `MayModify`, and interruption atomicity is `None`.

## PreferencesKeyValueStorage

`PreferencesKeyValueStorage<TBindingTag, TByteOperationsProvider>` adapts one dedicated Arduino Preferences namespace.

The provider exposes opaque logical values up to 512 bytes.

NVS-backed key names are limited to the documented 15-byte ASCII representation. A valid EDP UTF-8 key which is not representable as ASCII returns `KeyNotRepresentable`.

Arduino Preferences rejects a zero-length `putBytes()` operation. To preserve the EDP-Persistence distinction between a present zero-length value and an absent key, the provider uses a reserved native marker representation. The bound Preferences namespace must therefore be dedicated to this provider while it is open.

A fixed 512-byte scratch buffer is owned by the provider so truncated logical reads remain allocation-free even though Preferences/NVS requires storage for a complete blob. Copying from that scratch buffer to caller memory is performed through the injected EDP-Memory ByteOperations provider.

The provider owns its Preferences handle but not global NVS lifecycle.

`Begin()` returns `PreferencesBeginStatus` rather than `bool`:

- `Succeeded` means the namespace is ready, including an idempotent call made while already open;
- `ProviderFailure` means Arduino Preferences could not open the namespace.

Call `End()` during teardown.

Base key/value mutation failure preservation is `MayModify`. Individual key mutation interruption atomicity remains `PowerLoss`; namespace-wide `ClearAllKeys()` is deliberately `None` because NVS erases matching entries iteratively.

## Composition identity

`TBindingTag` allows otherwise-compatible logical storage bindings to remain distinct provider types.

Consumers should select Persistence providers through Composition requirements/qualifiers rather than a runtime registry.

The injected ByteOperations provider remains a separate Memory-domain provider selected by Bootstrap; it is not folded into the Persistence capability identity.

## ISR boundary

These Persistence providers are **not ISR-safe and make no ISR-safety guarantee**.

Do not call filesystem, Preferences, Persistence mutation, enumeration, lifecycle, or ordinary read operations from an ISR. Follow the ESPressio ISR policy: capture only the minimum ISR-local observation/state required, return promptly, and perform Persistence work later from ordinary execution context.

## Validation

`tests/ProviderBehaviorTests.cpp` exercises every concrete provider operation plus lifecycle/not-ready behavior using bounded Arduino SDK test doubles.

`tests/ContractCompile.cpp` independently invokes `ValidatePersistenceProvider<T>()` for both providers while wiring the real EDP-Platform-Portable ByteOperations concrete type.

`tests/run_tests.py` performs:

1. host behavior-test compilation/execution;
2. PlatformIO Arduino demo source compilation;
3. PlatformIO ESP-IDF demo source compilation;
4. real Arduino-ESP32 SDK contract compilation.

The runner requires sibling EDP-Persistence, EDP-Memory, EDP-Platform, EDP-Platform-Portable, and EDP-System checkouts.

## Build baseline

See [BUILDING.MD](./BUILDING.MD) for the mandatory C++20 baseline, PIOArduino recommendation, and exact supported PlatformIO configuration.
