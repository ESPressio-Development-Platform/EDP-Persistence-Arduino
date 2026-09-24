# src/PreferencesKeyValueStorage.hpp

**Primary classification:** PUBLIC PROVIDER / EXTENSION API

**Source baseline:** `f3fce6a6882862e6eba447a02d21e1fdb1c80d36`

[Open exact source](https://github.com/ESPressio-Development-Platform/EDP-Persistence-Arduino/blob/f3fce6a6882862e6eba447a02d21e1fdb1c80d36/src/PreferencesKeyValueStorage.hpp)

## Direct includes

- `ESPressio_Memory.hpp`
- `Preferences.h`
- `ESPressio_Persistence.hpp`

## Documented declarations

### `PreferencesBeginStatus`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Result of opening the Preferences namespace owned by one provider instance.

```cpp
enum class PreferencesBeginStatus : std::uint8_t
```

### `TBindingTag`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Adapts one Arduino Preferences namespace to the EDP KeyValueStorage contract.

- **Template parameter `TBindingTag`:** Distinguishes independently selectable Preferences namespaces.
- **Template parameter `TByteOperationsProvider`:** Supplies EDP-Memory raw byte-copy operations used by truncated reads.

```cpp
template<
        class TBindingTag,
```

### `Preferences_`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Arduino Preferences object owning the open NVS handle.

```cpp
mutable Preferences Preferences_;
```

### `ByteOperations_`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Non-owning EDP-Memory byte-operation provider used for bounded raw copies.

```cpp
const TByteOperationsProvider* ByteOperations_;
```

### `mutable std::uint8_t ReadScratch_[512U];`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Bounded scratch space used only when the caller requests a truncated blob read.

```cpp
mutable std::uint8_t ReadScratch_[512U];
```

### `IsReady_`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Indicates whether Begin() successfully opened the namespace.

```cpp
bool IsReady_;
```

### `KeyCopyStatus`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Result of converting an EDP key to the native Preferences key representation.

```cpp
enum class KeyCopyStatus : std::uint8_t
```

### `EmptyValueMarker`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Marker type/value used to represent one present zero-length logical blob.

```cpp
static constexpr std::uint8_t EmptyValueMarker = 0xA5U;
```

### `CopyKey`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Converts an EDP key to the documented ASCII NVS key representation.

```cpp
[[nodiscard]] static KeyCopyStatus CopyKey(
            KeyView Key,
            char (&Buffer)[16U]
        ) noexcept
```

### `IsEmptyValue`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Reports whether a native Preferences entry encodes an empty logical value.

```cpp
[[nodiscard]] bool IsEmptyValue(const char* Key) const noexcept
```

### `PreferencesKeyValueStorage`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Constructs an unopened provider using one caller-owned ByteOperations provider.

```cpp
explicit PreferencesKeyValueStorage(
            const TByteOperationsProvider& ByteOperations
        ) noexcept
            : Preferences_(),
```

### `Begin`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Opens the caller-selected Preferences namespace.

```cpp
[[nodiscard]] PreferencesBeginStatus Begin(const char* Namespace) noexcept
```

### `End`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Closes the Preferences namespace.

```cpp
void End() noexcept
```

### `IsKeyValueStorageReady`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Reports whether the Preferences namespace is open.

```cpp
[[nodiscard]] bool IsKeyValueStorageReady() const noexcept
```

### `GetValueSize`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Returns the complete blob size for a key.

```cpp
[[nodiscard]] KeyValueSizeResult GetValueSize(KeyView Key) const noexcept
```

### `ReadValue`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Reads as much of the complete blob as the caller destination can hold.

```cpp
[[nodiscard]] KeyValueReadResult ReadValue(
            KeyView Key,
            DestinationBufferView Destination
        ) const noexcept
```

### `StoreValue`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Stores one complete opaque value.

```cpp
[[nodiscard]] KeyValueStoreStatus StoreValue(
            KeyView Key,
            SourceBufferView Source
        ) noexcept
```

### `RemoveKey`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Removes one key.

```cpp
[[nodiscard]] KeyValueRemoveStatus RemoveKey(KeyView Key) noexcept
```

### `ClearAllKeys`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Removes every key in the bound Preferences namespace.

```cpp
[[nodiscard]] KeyValueClearStatus ClearAllKeys() noexcept
```

