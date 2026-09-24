# src/FileSystemStorage.hpp

**Primary classification:** PUBLIC PROVIDER / EXTENSION API

**Source baseline:** `f3fce6a6882862e6eba447a02d21e1fdb1c80d36`

[Open exact source](https://github.com/ESPressio-Development-Platform/EDP-Persistence-Arduino/blob/f3fce6a6882862e6eba447a02d21e1fdb1c80d36/src/FileSystemStorage.hpp)

## Direct includes

- `ESPressio_Memory.hpp`
- `cstring`
- `FS.h`
- `ESPressio_Persistence.hpp`

## Documented declarations

### `TBindingProfile`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Returns the binding's explicit invocation-concurrency guarantee when present.

Older/custom profiles which predate the concurrency field remain conservative.

```cpp
template<class TBindingProfile>
        [[nodiscard]] consteval InvocationConcurrency BindingConcurrency() noexcept
```

### `TRetention`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Declares the compile-time guarantees of one hierarchical Arduino filesystem binding.

- **Template parameter `TRetention`:** Commit-boundary retention guaranteed by the bound filesystem.
- **Template parameter `TCaseSensitivity`:** Path comparison behaviour guaranteed by the bound filesystem.
- **Template parameter `TRemovability`:** Whether the backing medium can disappear while the system is running.
- **Template parameter `TMaximumPathBytes`:** Largest complete EDP path accepted by the binding.
- **Template parameter `TMaximumPathSegmentBytes`:** Largest individual path segment accepted by the binding.
- **Template parameter `TMaximumFileSize`:** Largest logical file supported by the binding.
- **Template parameter `TInvocationConcurrency`:** Safe invocation concurrency guaranteed by the mounted filesystem.

```cpp
template<
        RetentionLevel TRetention,
```

### `Retention`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Commit-boundary retention guaranteed by the bound filesystem.

```cpp
static constexpr RetentionLevel Retention = TRetention;
```

### `CaseSensitivity`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Case-sensitivity semantics guaranteed for paths.

```cpp
static constexpr TextCaseSensitivity CaseSensitivity = TCaseSensitivity;
```

### `Removability`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Whether the backing medium can be removed while the application is running.

```cpp
static constexpr MediaRemovability Removability = TRemovability;
```

### `MaximumPathBytes`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Maximum complete provider-relative path accepted by the binding.

```cpp
static constexpr std::size_t MaximumPathBytes = TMaximumPathBytes;
```

### `MaximumPathSegmentBytes`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Maximum individual path segment accepted by the binding.

```cpp
static constexpr std::size_t MaximumPathSegmentBytes = TMaximumPathSegmentBytes;
```

### `static constexpr StorageSize MaximumFileSize{TMaximumFileSize};`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Maximum logical file size supported by the binding.

```cpp
static constexpr StorageSize MaximumFileSize{TMaximumFileSize};
```

### `Concurrency`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Safe invocation concurrency guaranteed by the mounted filesystem substrate.

```cpp
static constexpr InvocationConcurrency Concurrency = TInvocationConcurrency;
```

### `TBindingTag`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Adapts one already-mounted Arduino filesystem to the EDP FileStorage contract.

- **Template parameter `TBindingTag`:** Distinguishes independently selectable logical filesystem bindings.
- **Template parameter `TBindingProfile`:** Declares the semantic guarantees of the mounted filesystem substrate.
- **Template parameter `TByteOperationsProvider`:** Supplies EDP-Memory raw byte-copy operations used by the adapter.

```cpp
template<
        class TBindingTag,
```

### `NativePathStatus`

**Classification:** PRIVATE IMPLEMENTATION · source access: `private`

Result of assembling one provider-native path.

```cpp
enum class NativePathStatus : std::uint8_t
```

### `FileSystem_`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Non-owning filesystem reference; Bootstrap owns mount and lifetime.

```cpp
fs::FS* FileSystem_;
```

### `ByteOperations_`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Non-owning EDP-Memory byte-operation provider used for bounded raw copies.

```cpp
const TByteOperationsProvider* ByteOperations_;
```

### `IsPathRepresentable`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Reports whether a canonical EDP path fits the binding's advertised limits.

```cpp
[[nodiscard]] static bool IsPathRepresentable(FilePathView Path) noexcept
```

### `MakeNativePath`

**Classification:** PUBLIC PROVIDER / EXTENSION API

Converts an EDP relative path to Arduino FS's rooted path form.

```cpp
[[nodiscard]] NativePathStatus MakeNativePath(
            FilePathView Path,
            char (&Buffer)[256U]
        ) const noexcept
```

### `FileSystemStorage`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Constructs a provider bound to caller-owned filesystem and byte-operation providers.

```cpp
FileSystemStorage(
            fs::FS& FileSystem,
            const TByteOperationsProvider& ByteOperations
        ) noexcept
            : FileSystem_(&FileSystem),
```

### `IsFileStorageReady`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Reports whether a filesystem object is bound; mounting remains the owner's responsibility.

```cpp
[[nodiscard]] bool IsFileStorageReady() const noexcept
```

### `GetFileSize`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Returns the size of an existing regular file.

```cpp
[[nodiscard]] FileSizeResult GetFileSize(FilePathView Path) const noexcept
```

### `ReadFileAt`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Reads a bounded range from an existing regular file.

```cpp
[[nodiscard]] FileReadResult ReadFileAt(
            FilePathView Path,
            StorageOffset Offset,
            DestinationBufferView Destination
        ) const noexcept
```

### `ReplaceFile`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Creates or replaces one complete file.

```cpp
[[nodiscard]] FileReplaceStatus ReplaceFile(
            FilePathView Path,
            SourceBufferView Source
        ) noexcept
```

### `RemoveFile`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Removes an existing regular file.

```cpp
[[nodiscard]] FileRemoveStatus RemoveFile(FilePathView Path) noexcept
```

### `CreateDirectory`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Creates one directory without recursively creating parents.

```cpp
[[nodiscard]] DirectoryCreateStatus CreateDirectory(FilePathView Path) noexcept
```

### `RemoveDirectory`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Removes one empty directory.

```cpp
[[nodiscard]] DirectoryRemoveStatus RemoveDirectory(FilePathView Path) noexcept
```

### `EnumerateDirectory`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

TCallback is the caller-owned noexcept enumeration callback.

```cpp
template<FileEnumerationCallback TCallback>
        [[nodiscard]] FileEnumerationResult EnumerateDirectory(
            DirectoryPathView Directory,
            DestinationBufferView NameBuffer,
            TCallback& Callback
        ) const noexcept
```

### `RenameEntry`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Renames one existing file or directory without overwriting the destination.

```cpp
[[nodiscard]] FileRenameStatus RenameEntry(
            FilePathView Source,
            FilePathView Destination
        ) noexcept
```

### `AppendFile`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Appends a complete source buffer to an existing file.

```cpp
[[nodiscard]] FileAppendStatus AppendFile(
            FilePathView Path,
            SourceBufferView Source
        ) noexcept
```

### `WriteFileAt`

**Classification:** PUBLIC PROVIDER / EXTENSION API · source access: `public`

Replaces bytes within the existing file extent without extending the file.

```cpp
[[nodiscard]] FileWriteAtStatus WriteFileAt(
            FilePathView Path,
            StorageOffset Offset,
            SourceBufferView Source
        ) noexcept
```

