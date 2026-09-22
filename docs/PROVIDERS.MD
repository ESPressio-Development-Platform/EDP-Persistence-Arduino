# Concrete Arduino Persistence Providers

## Purpose and scope

EDP-Persistence-Arduino provides Arduino-ESP32 concrete adapters for the platform-neutral EDP-Persistence contract. It owns SDK adaptation only; Persistence semantics remain defined by EDP-Persistence.

## FileSystemStorage

`FileSystemStorage<TBindingTag, TBindingProfile>` adapts one caller-owned, already-mounted hierarchical `fs::FS` object.

The binding profile is part of the provider type and declares the semantic guarantees that Composition consumers may rely upon:

- retention;
- path case sensitivity;
- media removability;
- maximum complete path bytes;
- maximum path-segment bytes;
- maximum file size.

The generic adapter does not infer those guarantees from `fs::FS`. The application must choose a profile that is true for the actual mounted substrate. This provider structurally offers directory mutation/enumeration, rename, append and bounded write-at; use a different/specialized provider for a filesystem that does not genuinely provide those operations.

The provider does not mount, format, unmount, or own the filesystem. The `fs::FS` object and mount must outlive the provider.

All operations advertise `CallerSerialized`. File mutation failure preservation is `MayModify`, and interruption atomicity is `None`.

## PreferencesKeyValueStorage

`PreferencesKeyValueStorage<TBindingTag>` adapts one dedicated Arduino Preferences namespace.

The provider exposes opaque logical values up to 512 bytes. NVS-backed key names are limited to the documented 15-byte ASCII representation; a valid EDP UTF-8 key which is not ASCII returns `KeyNotRepresentable`.

Arduino Preferences rejects a zero-length `putBytes()` operation. To preserve the EDP-Persistence requirement that a present zero-length value is distinct from an absent key, the provider uses a reserved native marker representation. The bound Preferences namespace must therefore be dedicated to this provider while it is in use; other code must not write arbitrary native Preferences values into that namespace.

A fixed 512-byte scratch buffer is owned by the provider so truncated logical reads remain allocation-free even though Preferences/NVS requires storage for a complete blob.

The provider owns its Preferences handle but not global NVS lifecycle. Call `Begin()` during Bootstrap after the platform is ready and `End()` during teardown.

Base key/value mutation failure preservation is `MayModify`. Individual key mutation interruption atomicity remains `PowerLoss`; namespace-wide `ClearAllKeys()` is deliberately `None` because NVS erases matching entries iteratively.

## Composition identity

`TBindingTag` allows otherwise-compatible logical storage bindings to remain distinct provider types. Consumers should select providers through Composition requirements/qualifiers rather than a runtime registry.

## Validation

`tests/ProviderBehaviorTests.cpp` executes adapter-owned behavior with bounded `FS.h` and `Preferences.h` test doubles. It covers file mutation/read behavior, truncation facts, advertised size limits, key representability, zero-length logical values, clear/remove semantics, and related operation statuses.

`tests/ContractCompile.cpp` independently invokes `ValidatePersistenceProvider<T>()` for both concrete providers. `tests/run_tests.py` runs the host behavior tests first and then compiles the contract source with C++20 against the real Arduino-ESP32 SDK, EDP-Persistence and EDP-System.
