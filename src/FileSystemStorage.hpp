#pragma once

#include <cstring>

#include <FS.h>

#include <ESPressio_Persistence.hpp>
#include <memory/ByteOperationsContract.hpp>

namespace ESPressio::Persistence::Arduino {

    namespace Framework = ESPressio::System::CompositionFramework;


    /// Declares the compile-time guarantees of one hierarchical Arduino filesystem binding.
    /// TRetention is the commit-boundary retention guaranteed by the bound filesystem.
    /// TCaseSensitivity is the path comparison behaviour of the bound filesystem.
    /// TRemovability describes whether the backing medium can disappear while the system is running.
    /// TMaximumPathBytes is the largest complete EDP path accepted by the binding.
    /// TMaximumPathSegmentBytes is the largest individual path segment accepted by the binding.
    /// TMaximumFileSize is the largest logical file supported by the binding.
    template<
        RetentionLevel TRetention,
        TextCaseSensitivity TCaseSensitivity,
        MediaRemovability TRemovability,
        std::size_t TMaximumPathBytes,
        std::size_t TMaximumPathSegmentBytes,
        std::uint64_t TMaximumFileSize
    >
    struct FileSystemBindingProfile final {

        /// Commit-boundary retention guaranteed by the bound filesystem.
        static constexpr RetentionLevel Retention = TRetention;

        /// Case-sensitivity semantics guaranteed for paths.
        static constexpr TextCaseSensitivity CaseSensitivity = TCaseSensitivity;

        /// Whether the backing medium can be removed while the application is running.
        static constexpr MediaRemovability Removability = TRemovability;

        /// Maximum complete provider-relative path accepted by the binding.
        static constexpr std::size_t MaximumPathBytes = TMaximumPathBytes;

        /// Maximum individual path segment accepted by the binding.
        static constexpr std::size_t MaximumPathSegmentBytes = TMaximumPathSegmentBytes;

        /// Maximum logical file size supported by the binding.
        static constexpr StorageSize MaximumFileSize{TMaximumFileSize};

    };


    /// Adapts one already-mounted Arduino filesystem to the EDP FileStorage contract.
    ///
    /// @tparam TBindingTag Distinguishes independently selectable logical filesystem bindings.
    /// @tparam TBindingProfile Declares the semantic guarantees of the mounted filesystem substrate.
    /// @tparam TByteOperationsProvider Supplies EDP-Memory raw byte-copy operations used by the adapter.
    template<
        class TBindingTag,
        class TBindingProfile,
        class TByteOperationsProvider
    >
    class FileSystemStorage final : public Framework::Provider<
        Domain,
        Framework::Provides<
            Framework::Offer<
                FileStorage,
                Framework::PropertyValue<FileAccessMode, AccessMode::ReadWrite>,
                Framework::PropertyValue<FileRetention, TBindingProfile::Retention>,
                Framework::PropertyValue<FileHierarchyMode, FileHierarchy::Hierarchical>,
                Framework::PropertyValue<FilePathCaseSensitivity, TBindingProfile::CaseSensitivity>,
                Framework::PropertyValue<FileMediaRemovability, TBindingProfile::Removability>,
                Framework::PropertyValue<MaximumPathBytes, TBindingProfile::MaximumPathBytes>,
                Framework::PropertyValue<MaximumPathSegmentBytes, TBindingProfile::MaximumPathSegmentBytes>,
                Framework::PropertyValue<MaximumFileSize, TBindingProfile::MaximumFileSize>,
                Framework::PropertyValue<DirectoryMutationSupport, Support::Supported>,
                Framework::PropertyValue<DirectoryEnumerationSupport, Support::Supported>,
                Framework::PropertyValue<RenameSupport, Support::Supported>,
                Framework::PropertyValue<AppendSupport, Support::Supported>,
                Framework::PropertyValue<WriteFileAtSupport, Support::Supported>,
                Framework::PropertyValue<FileCapacityReportingSupport, Support::Unsupported>,
                Framework::PropertyValue<FileInvocationConcurrency, InvocationConcurrency::CallerSerialized>,
                Framework::PropertyValue<FileFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<FileInterruptionAtomicity, InterruptionAtomicity::None>,
                Framework::PropertyValue<DirectoryMutationFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<DirectoryMutationInterruptionAtomicity, InterruptionAtomicity::None>,
                Framework::PropertyValue<RenameFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<RenameInterruptionAtomicity, InterruptionAtomicity::None>,
                Framework::PropertyValue<AppendFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<AppendInterruptionAtomicity, InterruptionAtomicity::None>,
                Framework::PropertyValue<WriteFileAtFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<WriteFileAtInterruptionAtomicity, InterruptionAtomicity::None>
            >
        >,
        Framework::Requires<>,
        Framework::DependsOn<
            Framework::Need<ESPressio::Memory::ByteOperations>
        >
    > {
    private:

        static_assert(
            sizeof(
                ESPressio::Memory::Detail::ByteOperationsProviderTraits<
                    TByteOperationsProvider
                >
            ) > 0U,
            "Arduino FileSystemStorage requires an EDP-Memory ByteOperations provider"
        );

        static_assert(
            TBindingProfile::MaximumPathBytes > 0U &&
            TBindingProfile::MaximumPathBytes <= 254U,
            "Arduino FileSystemStorage binding path limit must fit the provider's bounded native path buffer"
        );

        static_assert(
            TBindingProfile::MaximumPathSegmentBytes > 0U &&
            TBindingProfile::MaximumPathSegmentBytes <= TBindingProfile::MaximumPathBytes,
            "Arduino FileSystemStorage binding segment limit must be non-zero and no larger than the path limit"
        );

        static_assert(
            TBindingProfile::MaximumFileSize.RawValue <= 0xFFFFFFFFULL,
            "Arduino FileSystemStorage binding file limit must fit Arduino File::seek"
        );

        /// Result of assembling one provider-native path.
        enum class NativePathStatus : std::uint8_t {
            Succeeded = 0U,
            PathNotRepresentable = 1U
        };


        // Bound dependencies.

        /// Non-owning filesystem reference; Bootstrap owns mount and lifetime.
        fs::FS* FileSystem_;

        /// Non-owning EDP-Memory byte-operation provider used for bounded raw copies.
        const TByteOperationsProvider* ByteOperations_;

        /// Reports whether a canonical EDP path fits the binding's advertised limits.
        [[nodiscard]] static bool IsPathRepresentable(FilePathView Path) noexcept {
            if (Path.Size() > TBindingProfile::MaximumPathBytes) {
                return false;
            }

            std::size_t SegmentSize = 0U;

            for (std::size_t Index = 0U; Index < Path.Size(); ++Index) {
                if (Path.Data()[Index] == '/') {
                    if (SegmentSize > TBindingProfile::MaximumPathSegmentBytes) {
                        return false;
                    }

                    SegmentSize = 0U;
                    continue;
                }

                ++SegmentSize;
            }

            return SegmentSize <= TBindingProfile::MaximumPathSegmentBytes;
        }

        /// Converts an EDP relative path to Arduino FS's rooted path form.
        [[nodiscard]] NativePathStatus MakeNativePath(
            FilePathView Path,
            char (&Buffer)[256U]
        ) const noexcept {
            if (!IsPathRepresentable(Path)) {
                return NativePathStatus::PathNotRepresentable;
            }

            Buffer[0] = '/';
            ByteOperations_->CopyBytes(
                Buffer + 1U,
                Path.Data(),
                Path.Size()
            );
            Buffer[Path.Size() + 1U] = '\0';
            return NativePathStatus::Succeeded;
        }

    public:

        /// Constructs a provider bound to caller-owned filesystem and byte-operation providers.
        FileSystemStorage(
            fs::FS& FileSystem,
            const TByteOperationsProvider& ByteOperations
        ) noexcept
            : FileSystem_(&FileSystem),
              ByteOperations_(&ByteOperations) {}

        /// Reports whether a filesystem object is bound; mounting remains the owner's responsibility.
        [[nodiscard]] bool IsFileStorageReady() const noexcept {
            return FileSystem_ != nullptr;
        }

        /// Returns the size of an existing regular file.
        [[nodiscard]] FileSizeResult GetFileSize(FilePathView Path) const noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return {FileSizeStatus::PathTooLong, StorageSize{}};
            }

            auto File = FileSystem_->open(
                NativePath,
                FILE_READ
            );

            if (!File) {
                return {FileSizeStatus::NotFound, StorageSize{}};
            }

            if (File.isDirectory()) {
                File.close();
                return {FileSizeStatus::NotFound, StorageSize{}};
            }

            const auto Size = static_cast<std::uint64_t>(File.size());
            File.close();

            if (Size > TBindingProfile::MaximumFileSize.RawValue) {
                return {FileSizeStatus::ProviderFailure, StorageSize{}};
            }

            return {FileSizeStatus::Succeeded, StorageSize{Size}};
        }

        /// Reads a bounded range from an existing regular file.
        [[nodiscard]] FileReadResult ReadFileAt(
            FilePathView Path,
            StorageOffset Offset,
            DestinationBufferView Destination
        ) const noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return {FileReadStatus::PathTooLong, 0U, 0U, StorageSize{}};
            }

            auto File = FileSystem_->open(
                NativePath,
                FILE_READ
            );

            if (!File) {
                return {FileReadStatus::NotFound, 0U, 0U, StorageSize{}};
            }

            const auto CompleteSize = static_cast<std::uint64_t>(File.size());

            if (CompleteSize > TBindingProfile::MaximumFileSize.RawValue) {
                File.close();
                return {FileReadStatus::ProviderFailure, 0U, 0U, StorageSize{}};
            }

            if (Offset.RawValue > CompleteSize) {
                File.close();
                return {FileReadStatus::InvalidOffset, 0U, 0U, StorageSize{}};
            }

            const auto Available = CompleteSize - Offset.RawValue;
            const auto TransferSize = Available < Destination.Capacity ? static_cast<std::size_t>(Available) : Destination.Capacity;

            if (!File.seek(static_cast<std::uint32_t>(Offset.RawValue))) {
                File.close();
                return {FileReadStatus::IoFailure, 0U, 0U, StorageSize{}};
            }

            const auto Read = TransferSize == 0U ? 0U : File.read(
                static_cast<std::uint8_t*>(Destination.Address),
                TransferSize
            );
            File.close();

            if (Read != TransferSize) {
                return {FileReadStatus::IoFailure, 0U, 0U, StorageSize{}};
            }

            std::uint8_t Facts = static_cast<std::uint8_t>(ReadFact::AvailableDataSizeIsKnown);

            if (Available > Destination.Capacity) {
                Facts |= static_cast<std::uint8_t>(ReadFact::WasTruncated);
            } else if (Available < Destination.Capacity) {
                Facts |= static_cast<std::uint8_t>(ReadFact::IsSmallerThanAvailableBuffer);
            }

            return {FileReadStatus::Succeeded, Facts, Read, StorageSize{Available}};
        }

        /// Creates or replaces one complete file.
        [[nodiscard]] FileReplaceStatus ReplaceFile(
            FilePathView Path,
            SourceBufferView Source
        ) noexcept {
            if (Source.Size > TBindingProfile::MaximumFileSize.RawValue) {
                return FileReplaceStatus::FileTooLarge;
            }

            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return FileReplaceStatus::PathTooLong;
            }

            if (FileSystem_->exists(NativePath)) {
                auto Existing = FileSystem_->open(
                    NativePath,
                    FILE_READ
                );

                if (!Existing) {
                    return FileReplaceStatus::IoFailure;
                }

                const auto IsDirectory = Existing.isDirectory();
                Existing.close();

                if (IsDirectory) {
                    return FileReplaceStatus::EntryTypeConflict;
                }
            }

            auto File = FileSystem_->open(
                NativePath,
                FILE_WRITE
            );

            if (!File) {
                return FileReplaceStatus::IoFailure;
            }

            const auto Written = Source.Size == 0U ? 0U : File.write(
                static_cast<const std::uint8_t*>(Source.Address),
                Source.Size
            );
            File.flush();
            File.close();
            return Written == Source.Size ? FileReplaceStatus::Succeeded : FileReplaceStatus::IoFailure;
        }

        /// Removes an existing regular file.
        [[nodiscard]] FileRemoveStatus RemoveFile(FilePathView Path) noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return FileRemoveStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativePath)) {
                return FileRemoveStatus::NotFound;
            }

            auto Entry = FileSystem_->open(
                NativePath,
                FILE_READ
            );

            if (!Entry) {
                return FileRemoveStatus::IoFailure;
            }

            const auto IsDirectory = Entry.isDirectory();
            Entry.close();

            if (IsDirectory) {
                return FileRemoveStatus::EntryTypeConflict;
            }

            return FileSystem_->remove(NativePath) ? FileRemoveStatus::Succeeded : FileRemoveStatus::IoFailure;
        }

        /// Creates one directory without recursively creating parents.
        [[nodiscard]] DirectoryCreateStatus CreateDirectory(FilePathView Path) noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return DirectoryCreateStatus::PathTooLong;
            }

            if (FileSystem_->exists(NativePath)) {
                auto Entry = FileSystem_->open(
                    NativePath,
                    FILE_READ
                );

                if (!Entry) {
                    return DirectoryCreateStatus::IoFailure;
                }

                const auto IsDirectory = Entry.isDirectory();
                Entry.close();

                return IsDirectory
                    ? DirectoryCreateStatus::AlreadyExists
                    : DirectoryCreateStatus::EntryTypeConflict;
            }

            return FileSystem_->mkdir(NativePath) ? DirectoryCreateStatus::Succeeded : DirectoryCreateStatus::IoFailure;
        }

        /// Removes one empty directory.
        [[nodiscard]] DirectoryRemoveStatus RemoveDirectory(FilePathView Path) noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return DirectoryRemoveStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativePath)) {
                return DirectoryRemoveStatus::NotFound;
            }

            auto Directory = FileSystem_->open(
                NativePath,
                FILE_READ
            );

            if (!Directory) {
                return DirectoryRemoveStatus::IoFailure;
            }

            if (!Directory.isDirectory()) {
                Directory.close();
                return DirectoryRemoveStatus::EntryTypeConflict;
            }

            auto Child = Directory.openNextFile();
            const auto HasChildren = static_cast<bool>(Child);
            Child.close();
            Directory.close();

            if (HasChildren) {
                return DirectoryRemoveStatus::NotEmpty;
            }

            return FileSystem_->rmdir(NativePath) ? DirectoryRemoveStatus::Succeeded : DirectoryRemoveStatus::IoFailure;
        }

        /// TCallback is the caller-owned noexcept enumeration callback.
        template<FileEnumerationCallback TCallback>
        [[nodiscard]] FileEnumerationResult EnumerateDirectory(
            DirectoryPathView Directory,
            DestinationBufferView NameBuffer,
            TCallback& Callback
        ) const noexcept {
            char NativePath[256U];

            if (Directory.IsRoot()) {
                NativePath[0] = '/';
                NativePath[1] = '\0';
            } else if (MakeNativePath(
                Directory.Path(),
                NativePath
            ) != NativePathStatus::Succeeded) {
                return {FileEnumerationStatus::PathTooLong, StorageSize{}};
            }

            auto DirectoryFile = FileSystem_->open(NativePath);

            if (!DirectoryFile) {
                return {FileEnumerationStatus::NotFound, StorageSize{}};
            }

            if (!DirectoryFile.isDirectory()) {
                DirectoryFile.close();
                return {FileEnumerationStatus::EntryTypeConflict, StorageSize{}};
            }

            std::uint64_t Visited = 0U;
            auto Entry = DirectoryFile.openNextFile();

            while (Entry) {
                const char* FullName = Entry.name();
                const char* Name = std::strrchr(
                    FullName,
                    '/'
                );
                Name = Name == nullptr ? FullName : Name + 1U;
                const auto CompleteSize = std::strlen(Name);
                auto DeliveredSize = CompleteSize < NameBuffer.Capacity ? CompleteSize : NameBuffer.Capacity;

                while (DeliveredSize != 0U && (static_cast<unsigned char>(Name[DeliveredSize]) & 0xC0U) == 0x80U) {
                    --DeliveredSize;
                }

                if (DeliveredSize != 0U) {
                    ByteOperations_->CopyBytes(
                        NameBuffer.Address,
                        Name,
                        DeliveredSize
                    );
                }

                std::uint8_t Facts = 0U;

                if (CompleteSize > NameBuffer.Capacity) {
                    Facts |= static_cast<std::uint8_t>(FileEnumerationEntryFact::NameWasTruncated);
                } else if (CompleteSize < NameBuffer.Capacity) {
                    Facts |= static_cast<std::uint8_t>(FileEnumerationEntryFact::NameIsSmallerThanAvailableBuffer);
                }

                const auto IsDirectory = Entry.isDirectory();

                if (!IsDirectory) {
                    Facts |= static_cast<std::uint8_t>(FileEnumerationEntryFact::FileSizeIsKnown);
                }

                const FileEnumerationEntry Observation{
                    Detail::PersistenceProviderAccess::MakeTextView(
                        static_cast<const char*>(NameBuffer.Address),
                        DeliveredSize
                    ),
                    StorageSize{CompleteSize},
                    IsDirectory ? StorageSize{} : StorageSize{Entry.size()},
                    Facts,
                    IsDirectory ? FileEntryKind::Directory : FileEntryKind::File
                };

                ++Visited;
                const auto Control = Callback(Observation);
                Entry.close();

                if (Control == EnumerationControl::Stop) {
                    DirectoryFile.close();
                    return {FileEnumerationStatus::StoppedByCallback, StorageSize{Visited}};
                }

                Entry = DirectoryFile.openNextFile();
            }

            DirectoryFile.close();
            return {FileEnumerationStatus::Completed, StorageSize{Visited}};
        }

        /// Renames one existing file or directory without overwriting the destination.
        [[nodiscard]] FileRenameStatus RenameEntry(
            FilePathView Source,
            FilePathView Destination
        ) noexcept {
            char NativeSource[256U];
            char NativeDestination[256U];

            if (MakeNativePath(
                Source,
                NativeSource
            ) != NativePathStatus::Succeeded ||
                MakeNativePath(
                    Destination,
                    NativeDestination
                ) != NativePathStatus::Succeeded) {
                return FileRenameStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativeSource)) {
                return FileRenameStatus::SourceNotFound;
            }

            if (FileSystem_->exists(NativeDestination)) {
                return FileRenameStatus::DestinationAlreadyExists;
            }

            return FileSystem_->rename(
                NativeSource,
                NativeDestination
            ) ? FileRenameStatus::Succeeded : FileRenameStatus::IoFailure;
        }

        /// Appends a complete source buffer to an existing file.
        [[nodiscard]] FileAppendStatus AppendFile(
            FilePathView Path,
            SourceBufferView Source
        ) noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return FileAppendStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativePath)) {
                return FileAppendStatus::NotFound;
            }

            auto File = FileSystem_->open(
                NativePath,
                FILE_APPEND
            );

            if (!File) {
                return FileAppendStatus::IoFailure;
            }

            if (File.isDirectory()) {
                File.close();
                return FileAppendStatus::EntryTypeConflict;
            }

            const auto ExistingSize = static_cast<std::uint64_t>(File.size());

            if (
                ExistingSize > TBindingProfile::MaximumFileSize.RawValue ||
                Source.Size > TBindingProfile::MaximumFileSize.RawValue - ExistingSize
            ) {
                File.close();
                return FileAppendStatus::FileTooLarge;
            }

            const auto Written = Source.Size == 0U ? 0U : File.write(
                static_cast<const std::uint8_t*>(Source.Address),
                Source.Size
            );
            File.flush();
            File.close();
            return Written == Source.Size ? FileAppendStatus::Succeeded : FileAppendStatus::IoFailure;
        }

        /// Replaces bytes within the existing file extent without extending the file.
        [[nodiscard]] FileWriteAtStatus WriteFileAt(
            FilePathView Path,
            StorageOffset Offset,
            SourceBufferView Source
        ) noexcept {
            char NativePath[256U];

            if (MakeNativePath(
                Path,
                NativePath
            ) != NativePathStatus::Succeeded) {
                return FileWriteAtStatus::PathTooLong;
            }

            auto File = FileSystem_->open(
                NativePath,
                "r+"
            );

            if (!File) {
                return FileWriteAtStatus::NotFound;
            }

            if (File.isDirectory()) {
                File.close();
                return FileWriteAtStatus::EntryTypeConflict;
            }

            const auto Size = static_cast<std::uint64_t>(File.size());

            if (Size > TBindingProfile::MaximumFileSize.RawValue) {
                File.close();
                return FileWriteAtStatus::ProviderFailure;
            }

            if (Offset.RawValue > Size || Source.Size > Size - Offset.RawValue) {
                File.close();
                return FileWriteAtStatus::RangeOutOfBounds;
            }

            if (!File.seek(static_cast<std::uint32_t>(Offset.RawValue))) {
                File.close();
                return FileWriteAtStatus::IoFailure;
            }

            const auto Written = Source.Size == 0U ? 0U : File.write(
                static_cast<const std::uint8_t*>(Source.Address),
                Source.Size
            );
            File.flush();
            File.close();
            return Written == Source.Size ? FileWriteAtStatus::Succeeded : FileWriteAtStatus::IoFailure;
        }

    };

} // ESPressio::Persistence::Arduino
