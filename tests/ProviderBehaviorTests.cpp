#include <cassert>
#include <cstdint>

#include <memory/ByteOperationsProvider.hpp>

#include <ESPressio_Persistence_Arduino.hpp>

namespace {

    using namespace ESPressio::Persistence;

    /// Logical binding identity used by the behavior tests.
    struct TestBinding final {};


    // Concrete test provider types.

    /// Standard C/C++ ByteOperations concrete selected for test wiring.
    using TestByteOperationsProvider =
        ESPressio::Platform::Portable::Memory::ByteOperationsProvider;

    /// Filesystem profile deliberately bounded so limit behavior is easy to exercise.
    using TestFileProfile = Arduino::FileSystemBindingProfile<
        RetentionLevel::Restart,
        TextCaseSensitivity::CaseSensitive,
        MediaRemovability::Fixed,
        31U,
        15U,
        8ULL
    >;

    /// Arduino filesystem provider under test.
    using TestFileStorageProvider = Arduino::FileSystemStorage<
        TestBinding,
        TestFileProfile,
        TestByteOperationsProvider
    >;

    /// Arduino Preferences provider under test.
    using TestKeyValueStorageProvider =
        Arduino::PreferencesKeyValueStorage<
            TestBinding,
            TestByteOperationsProvider
        >;


    /// Records how many FileStorage enumeration callbacks were delivered.
    struct EnumerationObserver final {

        // Observed state.

        /// Number of entries delivered by the provider.
        std::size_t Count = 0U;


        // Enumeration callback.

        /// Accepts one entry and continues enumeration.
        [[nodiscard]] EnumerationControl operator()(const FileEnumerationEntry&) noexcept {
            ++Count;
            return EnumerationControl::Continue;
        }

    };


    /// Exercises every exposed FileStorage operation supported by the Arduino provider.
    void RunFileStorageTests() {
        TestByteOperationsProvider ByteOperations;
        fs::FS FileSystem;
        TestFileStorageProvider Storage(
            FileSystem,
            ByteOperations
        );

        assert(Storage.IsFileStorageReady());

        constexpr auto Path = FilePathView::Validate("file.bin");
        constexpr auto Renamed = FilePathView::Validate("renamed.bin");
        constexpr auto Directory = FilePathView::Validate("dir");

        static_assert(Path.Status == FilePathValidationStatus::Succeeded);
        static_assert(Renamed.Status == FilePathValidationStatus::Succeeded);
        static_assert(Directory.Status == FilePathValidationStatus::Succeeded);

        assert(Storage.GetFileSize(Path.Value).Status == FileSizeStatus::NotFound);

        const std::uint8_t Initial[] = {
            1U,
            2U,
            3U,
            4U
        };

        assert(
            Storage.ReplaceFile(
                Path.Value,
                SourceBufferView{
                    Initial,
                    sizeof(Initial)
                }
            ) == FileReplaceStatus::Succeeded
        );

        const auto InitialSize = Storage.GetFileSize(Path.Value);
        assert(InitialSize.Status == FileSizeStatus::Succeeded);
        assert(InitialSize.Size.RawValue == sizeof(Initial));

        std::uint8_t Truncated[2U]{};
        const auto TruncatedRead = Storage.ReadFileAt(
            Path.Value,
            StorageOffset{},
            DestinationBufferView{
                Truncated,
                sizeof(Truncated)
            }
        );

        assert(TruncatedRead.Status == FileReadStatus::Succeeded);
        assert(TruncatedRead.BytesTransferred == 2U);
        assert(TruncatedRead.AvailableDataSize.RawValue == 4U);
        assert(
            HasFact(
                TruncatedRead.Facts,
                ReadFact::WasTruncated
            )
        );

        std::uint8_t NameBuffer[32U]{};
        EnumerationObserver Observer;
        const auto Enumeration = Storage.EnumerateDirectory(
            DirectoryPathView::Root(),
            DestinationBufferView{
                NameBuffer,
                sizeof(NameBuffer)
            },
            Observer
        );

        assert(Enumeration.Status == FileEnumerationStatus::Completed);
        assert(Enumeration.EntriesVisited.RawValue >= 1U);
        assert(Observer.Count == Enumeration.EntriesVisited.RawValue);

        const std::uint8_t Appended[] = {
            5U,
            6U
        };

        assert(
            Storage.AppendFile(
                Path.Value,
                SourceBufferView{
                    Appended,
                    sizeof(Appended)
                }
            ) == FileAppendStatus::Succeeded
        );

        const std::uint8_t Replacement = 9U;

        assert(
            Storage.WriteFileAt(
                Path.Value,
                StorageOffset{1U},
                SourceBufferView{
                    &Replacement,
                    1U
                }
            ) == FileWriteAtStatus::Succeeded
        );

        std::uint8_t Complete[6U]{};
        const auto CompleteRead = Storage.ReadFileAt(
            Path.Value,
            StorageOffset{},
            DestinationBufferView{
                Complete,
                sizeof(Complete)
            }
        );

        assert(CompleteRead.Status == FileReadStatus::Succeeded);
        assert(CompleteRead.BytesTransferred == sizeof(Complete));
        assert(Complete[0U] == 1U);
        assert(Complete[1U] == 9U);
        assert(Complete[4U] == 5U);
        assert(Complete[5U] == 6U);

        const std::uint8_t Oversized[9U]{};

        assert(
            Storage.ReplaceFile(
                Path.Value,
                SourceBufferView{
                    Oversized,
                    sizeof(Oversized)
                }
            ) == FileReplaceStatus::FileTooLarge
        );

        assert(
            Storage.RenameEntry(
                Path.Value,
                Renamed.Value
            ) == FileRenameStatus::Succeeded
        );

        assert(Storage.GetFileSize(Path.Value).Status == FileSizeStatus::NotFound);
        assert(Storage.GetFileSize(Renamed.Value).Status == FileSizeStatus::Succeeded);

        assert(
            Storage.RemoveFile(
                Renamed.Value
            ) == FileRemoveStatus::Succeeded
        );
        assert(Storage.RemoveFile(Renamed.Value) == FileRemoveStatus::NotFound);

        assert(
            Storage.CreateDirectory(
                Directory.Value
            ) == DirectoryCreateStatus::Succeeded
        );
        assert(
            Storage.CreateDirectory(
                Directory.Value
            ) == DirectoryCreateStatus::AlreadyExists
        );
        assert(
            Storage.RemoveDirectory(
                Directory.Value
            ) == DirectoryRemoveStatus::Succeeded
        );
        assert(
            Storage.RemoveDirectory(
                Directory.Value
            ) == DirectoryRemoveStatus::NotFound
        );
    }


    /// Exercises every exposed KeyValueStorage operation and lifecycle state.
    void RunKeyValueStorageTests() {
        TestByteOperationsProvider ByteOperations;
        TestKeyValueStorageProvider Storage(ByteOperations);

        constexpr auto Key = KeyView::Validate("payload");
        constexpr auto OtherKey = KeyView::Validate("other");
        constexpr auto NonAscii = KeyView::Validate("\xC3\xA9");

        static_assert(Key.Status == KeyValidationStatus::Succeeded);
        static_assert(OtherKey.Status == KeyValidationStatus::Succeeded);
        static_assert(NonAscii.Status == KeyValidationStatus::Succeeded);

        assert(!Storage.IsKeyValueStorageReady());
        assert(Storage.GetValueSize(Key.Value).Status == KeyValueSizeStatus::NotReady);
        assert(Storage.RemoveKey(Key.Value) == KeyValueRemoveStatus::NotReady);
        assert(Storage.ClearAllKeys() == KeyValueClearStatus::NotReady);

        assert(
            Storage.Begin(
                "test"
            ) == Arduino::PreferencesBeginStatus::Succeeded
        );
        assert(Storage.IsKeyValueStorageReady());
        assert(
            Storage.Begin(
                "test"
            ) == Arduino::PreferencesBeginStatus::Succeeded
        );

        const std::uint8_t Value[] = {
            10U,
            11U,
            12U
        };

        assert(
            Storage.StoreValue(
                Key.Value,
                SourceBufferView{
                    Value,
                    sizeof(Value)
                }
            ) == KeyValueStoreStatus::Succeeded
        );

        std::uint8_t Truncated[2U]{};
        const auto Read = Storage.ReadValue(
            Key.Value,
            DestinationBufferView{
                Truncated,
                sizeof(Truncated)
            }
        );

        assert(Read.Status == KeyValueReadStatus::Succeeded);
        assert(Read.BytesTransferred == sizeof(Truncated));
        assert(Read.AvailableDataSize.RawValue == sizeof(Value));
        assert(
            HasFact(
                Read.Facts,
                ReadFact::WasTruncated
            )
        );

        assert(
            Storage.StoreValue(
                NonAscii.Value,
                SourceBufferView{
                    Value,
                    sizeof(Value)
                }
            ) == KeyValueStoreStatus::KeyNotRepresentable
        );

        assert(
            Storage.StoreValue(
                Key.Value,
                SourceBufferView{
                    nullptr,
                    0U
                }
            ) == KeyValueStoreStatus::Succeeded
        );

        const auto EmptySize = Storage.GetValueSize(Key.Value);
        assert(EmptySize.Status == KeyValueSizeStatus::Succeeded);
        assert(EmptySize.Size.RawValue == 0U);

        assert(
            Storage.StoreValue(
                OtherKey.Value,
                SourceBufferView{
                    Value,
                    sizeof(Value)
                }
            ) == KeyValueStoreStatus::Succeeded
        );
        assert(Storage.RemoveKey(OtherKey.Value) == KeyValueRemoveStatus::Succeeded);
        assert(Storage.RemoveKey(OtherKey.Value) == KeyValueRemoveStatus::NotFound);

        assert(
            Storage.StoreValue(
                OtherKey.Value,
                SourceBufferView{
                    Value,
                    sizeof(Value)
                }
            ) == KeyValueStoreStatus::Succeeded
        );
        assert(Storage.ClearAllKeys() == KeyValueClearStatus::Succeeded);
        assert(Storage.GetValueSize(Key.Value).Status == KeyValueSizeStatus::NotFound);
        assert(Storage.GetValueSize(OtherKey.Value).Status == KeyValueSizeStatus::NotFound);

        Storage.End();
        assert(!Storage.IsKeyValueStorageReady());
        assert(Storage.GetValueSize(Key.Value).Status == KeyValueSizeStatus::NotReady);
    }

} // namespace


int main() {
    RunFileStorageTests();
    RunKeyValueStorageTests();
    return 0;
}
