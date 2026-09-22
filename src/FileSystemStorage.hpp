#pragma once

#include <cstring>

#include <FS.h>

#include <ESPressio_Persistence.hpp>

namespace ESPressio::Persistence::Arduino {

    namespace Framework = ESPressio::System::CompositionFramework;


    /// TBindingTag distinguishes independently selectable Arduino filesystem bindings in Composition.
    template<class TBindingTag>
    class FileSystemStorage final : public Framework::Provider<
        Domain,
        Framework::Provides<
            Framework::Offer<
                FileStorage,
                Framework::PropertyValue<FileAccessMode, AccessMode::ReadWrite>,
                Framework::PropertyValue<FileRetention, RetentionLevel::PowerLoss>,
                Framework::PropertyValue<FileHierarchyMode, FileHierarchy::Hierarchical>,
                Framework::PropertyValue<FilePathCaseSensitivity, TextCaseSensitivity::CaseSensitive>,
                Framework::PropertyValue<FileMediaRemovability, MediaRemovability::Fixed>,
                Framework::PropertyValue<MaximumPathBytes, std::size_t{254U}>,
                Framework::PropertyValue<MaximumPathSegmentBytes, std::size_t{254U}>,
                Framework::PropertyValue<MaximumFileSize, StorageSize{0xFFFFFFFFULL}>,
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
        >
    > {
    private:

        // Bound Arduino filesystem.

        /// Non-owning filesystem reference; Bootstrap owns mount and lifetime.
        fs::FS* FileSystem_;

        /// Converts an EDP relative path to Arduino FS's rooted path form.
        [[nodiscard]] static bool MakeNativePath(
            FilePathView Path,
            char (&Buffer)[256U]
        ) noexcept {
            if (Path.Size() > 254U) {
                return false;
            }

            Buffer[0] = '/';
            std::memcpy(
                Buffer + 1U,
                Path.Data(),
                Path.Size()
            );
            Buffer[Path.Size() + 1U] = '\0';
            return true;
        }

    public:

        /// Constructs a provider bound to an already-owned Arduino filesystem object.
        explicit FileSystemStorage(fs::FS& FileSystem) noexcept
            : FileSystem_(&FileSystem) {}

        /// Reports whether a filesystem object is bound; mounting remains the owner's responsibility.
        [[nodiscard]] bool IsFileStorageReady() const noexcept {
            return FileSystem_ != nullptr;
        }

        /// Returns the size of an existing regular file.
        [[nodiscard]] FileSizeResult GetFileSize(FilePathView Path) const noexcept {
            char NativePath[256U];

            if (!MakeNativePath(Path, NativePath)) {
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

            const auto Size = File.size();
            File.close();
            return {FileSizeStatus::Succeeded, StorageSize{Size}};
        }

        /// Reads a bounded range from an existing regular file.
        [[nodiscard]] FileReadResult ReadFileAt(
            FilePathView Path,
            StorageOffset Offset,
            DestinationBufferView Destination
        ) const noexcept {
            char NativePath[256U];

            if (!MakeNativePath(Path, NativePath)) {
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
            char NativePath[256U];

            if (!MakeNativePath(Path, NativePath)) {
                return FileReplaceStatus::PathTooLong;
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

            if (!MakeNativePath(Path, NativePath)) {
                return FileRemoveStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativePath)) {
                return FileRemoveStatus::NotFound;
            }

            return FileSystem_->remove(NativePath) ? FileRemoveStatus::Succeeded : FileRemoveStatus::IoFailure;
        }

        /// Creates one directory without recursively creating parents.
        [[nodiscard]] DirectoryCreateStatus CreateDirectory(FilePathView Path) noexcept {
            char NativePath[256U];

            if (!MakeNativePath(Path, NativePath)) {
                return DirectoryCreateStatus::PathTooLong;
            }

            if (FileSystem_->exists(NativePath)) {
                return DirectoryCreateStatus::AlreadyExists;
            }

            return FileSystem_->mkdir(NativePath) ? DirectoryCreateStatus::Succeeded : DirectoryCreateStatus::IoFailure;
        }

        /// Removes one empty directory.
        [[nodiscard]] DirectoryRemoveStatus RemoveDirectory(FilePathView Path) noexcept {
            char NativePath[256U];

            if (!MakeNativePath(Path, NativePath)) {
                return DirectoryRemoveStatus::PathTooLong;
            }

            if (!FileSystem_->exists(NativePath)) {
                return DirectoryRemoveStatus::NotFound;
            }

            return FileSystem_->rmdir(NativePath) ? DirectoryRemoveStatus::Succeeded : DirectoryRemoveStatus::NotEmpty;
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
            } else if (!MakeNativePath(Directory.Path(), NativePath)) {
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
                    std::memcpy(
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

            if (!MakeNativePath(Source, NativeSource) || !MakeNativePath(Destination, NativeDestination)) {
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

            if (!MakeNativePath(Path, NativePath)) {
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

            if (!MakeNativePath(Path, NativePath)) {
                return FileWriteAtStatus::PathTooLong;
            }

            auto File = FileSystem_->open(
                NativePath,
                "r+"
            );

            if (!File) {
                return FileWriteAtStatus::NotFound;
            }

            const auto Size = static_cast<std::uint64_t>(File.size());

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
