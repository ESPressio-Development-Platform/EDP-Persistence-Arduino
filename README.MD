# EDP-Persistence-Arduino

Concrete Arduino-ESP32 providers for the EDP-Persistence contract.

## Providers

- `FileSystemStorage<TBindingTag>` adapts an already-mounted Arduino `fs::FS` instance to hierarchical `FileStorage`. Bootstrap retains ownership of mounting and the filesystem object. Directory mutation/enumeration, rename, append and bounded write-at are supported.
- `PreferencesKeyValueStorage<TBindingTag>` adapts one caller-selected Arduino Preferences namespace to `KeyValueStorage`. Keys are limited to 15 UTF-8 bytes and values to 512 bytes. The 512-byte bound permits contract-correct truncated reads without heap allocation despite the underlying NVS whole-blob read API.

`TBindingTag` is intentionally part of each provider type so independently selectable logical bindings can have distinct Composition identities.

## Dependencies

- EDP-Persistence-Arduino
  - EDP-Persistence
    - EDP-System
  - Arduino-ESP32

## Lifecycle

The providers do not take ownership of platform lifecycle. The application mounts its filesystem before constructing/using `FileSystemStorage`. For Preferences, call `Begin(namespace)` during Bootstrap and `End()` during teardown.

## Concurrency and memory

Both providers advertise `CallerSerialized`. They perform no per-operation dynamic allocation themselves. Preferences contains one bounded 512-byte scratch buffer because NVS cannot partially retrieve a blob; it is used only when the caller destination requires truncation.

## Validation

Concrete SDK compilation and hardware/runtime tests belong in this repository. The abstract contract is validated independently by EDP-Persistence.
