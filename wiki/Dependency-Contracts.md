# Dependency Contracts

EDP-Persistence-Arduino depends on **EDP-Persistence** and **EDP-Memory**.

## EDP-Persistence

The repository implements the abstract Persistence-domain capabilities:

- `FileSystemStorage<TBindingTag,TBindingProfile,TByteOperationsProvider>` offers `FileStorage`;
- `PreferencesKeyValueStorage<TBindingTag,TByteOperationsProvider>` offers `KeyValueStorage`.

The provider properties advertised by each concrete type are part of its compile-time contract and are consumed by higher-level Requirement qualification.

## EDP-Memory ByteOperations

Both concrete providers declare exactly one external `Memory::ByteOperations` Requirement and validate the supplied provider through `Memory::Detail::ByteOperationsProviderTraits`.

The provider headers enter EDP-Memory through the public `<ESPressio_Memory.hpp>` umbrella so Arduino IDE / Arduino CLI can discover the mandatory sibling library before Memory contracts are referenced.

ByteOperations is used for bounded copy/scratch handling; the Persistence providers do not create a second memory abstraction.

## FileSystemStorage property contract

The provider advertises read/write hierarchical storage with binding-profile-defined retention, case sensitivity, removability and size/path limits; directory mutation/enumeration, rename, append and ranged write are supported; capacity reporting is unsupported. Invocation concurrency is taken from the binding profile.

The provider does not own filesystem mount/format/unmount lifecycle. A profile may advertise ConcurrentReads only when the complete mounted substrate and injected dependencies truly satisfy it.

## PreferencesKeyValueStorage property contract

Advertises read/write, PowerLoss retention, case-sensitive fixed media, 15-byte keys, 512-byte values, CallerSerialized invocation, ClearAll support and no enumeration/ranged-read/capacity-reporting support.

## Ownership

Bootstrap owns ByteOperations and native filesystem/Preferences lifecycle. Provider objects borrow/use those facilities under the advertised contract.

> Dependency contract audit baseline: `fce532a3ca7d06373b91ef90b7e39401d342391a` (`main`).
