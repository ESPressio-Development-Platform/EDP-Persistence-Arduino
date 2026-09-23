#pragma once

#include <Preferences.h>

#include <ESPressio_Persistence.hpp>
#include <memory/ByteOperationsContract.hpp>

namespace ESPressio::Persistence::Arduino {

    namespace Framework = ESPressio::System::CompositionFramework;


    /// Result of opening the Preferences namespace owned by one provider instance.
    enum class PreferencesBeginStatus : std::uint8_t {
        Succeeded = 0U,
        ProviderFailure = 1U
    };


    /// Adapts one Arduino Preferences namespace to the EDP KeyValueStorage contract.
    ///
    /// @tparam TBindingTag Distinguishes independently selectable Preferences namespaces.
    /// @tparam TByteOperationsProvider Supplies EDP-Memory raw byte-copy operations used by truncated reads.
    template<
        class TBindingTag,
        class TByteOperationsProvider
    >
    class PreferencesKeyValueStorage final : public Framework::Provider<
        Domain,
        Framework::Offers<
            Framework::Offer<
                KeyValueStorage,
                Framework::PropertyValue<KeyValueAccessMode, AccessMode::ReadWrite>,
                Framework::PropertyValue<KeyValueRetention, RetentionLevel::PowerLoss>,
                Framework::PropertyValue<KeyCaseSensitivity, TextCaseSensitivity::CaseSensitive>,
                Framework::PropertyValue<KeyValueMediaRemovability, MediaRemovability::Fixed>,
                Framework::PropertyValue<MaximumKeyBytes, std::size_t{15U}>,
                Framework::PropertyValue<MaximumKeyValueSize, StorageSize{512U}>,
                Framework::PropertyValue<KeyEnumerationSupport, Support::Unsupported>,
                Framework::PropertyValue<ReadValueAtSupport, Support::Unsupported>,
                Framework::PropertyValue<ClearAllSupport, Support::Supported>,
                Framework::PropertyValue<KeyValueCapacityReportingSupport, Support::Unsupported>,
                Framework::PropertyValue<KeyValueInvocationConcurrency, InvocationConcurrency::CallerSerialized>,
                Framework::PropertyValue<KeyValueFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<KeyValueInterruptionAtomicity, InterruptionAtomicity::PowerLoss>,
                Framework::PropertyValue<ClearAllFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<ClearAllInterruptionAtomicity, InterruptionAtomicity::None>
            >
        >,
        Framework::Contract<
            Framework::Requirement<
                ESPressio::Memory::ByteOperations,
                Framework::RequirementScope::ExternalDomain,
                Framework::ExactlyProviders<1U>
            >
        >
    > {
    private:

        static_assert(
            sizeof(
                ESPressio::Memory::Detail::ByteOperationsProviderTraits<
                    TByteOperationsProvider
                >
            ) > 0U,
            "Arduino PreferencesKeyValueStorage requires an EDP-Memory ByteOperations provider"
        );


        // Bound dependencies.

        /// Arduino Preferences object owning the open NVS handle.
        mutable Preferences Preferences_;

        /// Non-owning EDP-Memory byte-operation provider used for bounded raw copies.
        const TByteOperationsProvider* ByteOperations_;

        /// Bounded scratch space used only when the caller requests a truncated blob read.
        mutable std::uint8_t ReadScratch_[512U];

        /// Indicates whether Begin() successfully opened the namespace.
        bool IsReady_;

        // Native key representation helpers.

        /// Result of converting an EDP key to the native Preferences key representation.
        enum class KeyCopyStatus : std::uint8_t {
            Succeeded = 0U,
            TooLong = 1U,
            NotRepresentable = 2U
        };

        /// Marker type/value used to represent one present zero-length logical blob.
        static constexpr std::uint8_t EmptyValueMarker = 0xA5U;

        /// Converts an EDP key to the documented ASCII NVS key representation.
        [[nodiscard]] static KeyCopyStatus CopyKey(
            KeyView Key,
            char (&Buffer)[16U]
        ) noexcept {
            if (Key.Size() > 15U) {
                return KeyCopyStatus::TooLong;
            }

            for (std::size_t Index = 0U; Index < Key.Size(); ++Index) {
                const auto Byte = static_cast<unsigned char>(Key.Data()[Index]);

                if (Byte > 0x7FU) {
                    return KeyCopyStatus::NotRepresentable;
                }

                Buffer[Index] = Key.Data()[Index];
            }

            Buffer[Key.Size()] = '\0';
            return KeyCopyStatus::Succeeded;
        }

        /// Reports whether a native Preferences entry encodes an empty logical value.
        [[nodiscard]] bool IsEmptyValue(const char* Key) const noexcept {
            return Preferences_.getType(Key) == PT_U8
                && Preferences_.getUChar(
                    Key,
                    static_cast<std::uint8_t>(~EmptyValueMarker)
                ) == EmptyValueMarker;
        }

    public:

        // Lifecycle controlled by Bootstrap/application wiring.

        /// Constructs an unopened provider using one caller-owned ByteOperations provider.
        explicit PreferencesKeyValueStorage(
            const TByteOperationsProvider& ByteOperations
        ) noexcept
            : Preferences_(),
              ByteOperations_(&ByteOperations),
              IsReady_(false) {}

        /// Opens the caller-selected Preferences namespace.
        [[nodiscard]] PreferencesBeginStatus Begin(const char* Namespace) noexcept {
            if (IsReady_) {
                return PreferencesBeginStatus::Succeeded;
            }

            IsReady_ = Preferences_.begin(
                Namespace,
                false
            );

            return IsReady_
                ? PreferencesBeginStatus::Succeeded
                : PreferencesBeginStatus::ProviderFailure;
        }

        /// Closes the Preferences namespace.
        void End() noexcept {
            if (!IsReady_) {
                return;
            }

            Preferences_.end();
            IsReady_ = false;
        }

        // KeyValueStorage contract.

        /// Reports whether the Preferences namespace is open.
        [[nodiscard]] bool IsKeyValueStorageReady() const noexcept {
            return IsReady_;
        }

        /// Returns the complete blob size for a key.
        [[nodiscard]] KeyValueSizeResult GetValueSize(KeyView Key) const noexcept {
            if (!IsReady_) {
                return {KeyValueSizeStatus::NotReady, StorageSize{}};
            }

            char NativeKey[16U];

            const auto KeyStatus = CopyKey(
                Key,
                NativeKey
            );

            if (KeyStatus == KeyCopyStatus::TooLong) {
                return {KeyValueSizeStatus::KeyTooLong, StorageSize{}};
            }

            if (KeyStatus == KeyCopyStatus::NotRepresentable) {
                return {KeyValueSizeStatus::KeyNotRepresentable, StorageSize{}};
            }

            const auto Type = Preferences_.getType(NativeKey);

            if (Type == PT_INVALID) {
                return {KeyValueSizeStatus::NotFound, StorageSize{}};
            }

            if (Type == PT_U8) {
                return IsEmptyValue(NativeKey)
                    ? KeyValueSizeResult{
                        KeyValueSizeStatus::Succeeded,
                        StorageSize{}
                    }
                    : KeyValueSizeResult{
                        KeyValueSizeStatus::ProviderFailure,
                        StorageSize{}
                    };
            }

            if (Type != PT_BLOB) {
                return {KeyValueSizeStatus::ProviderFailure, StorageSize{}};
            }

            const auto Size = Preferences_.getBytesLength(NativeKey);

            if (Size > 512U) {
                return {KeyValueSizeStatus::ProviderFailure, StorageSize{}};
            }

            return {
                KeyValueSizeStatus::Succeeded,
                StorageSize{Size}
            };
        }

        /// Reads as much of the complete blob as the caller destination can hold.
        [[nodiscard]] KeyValueReadResult ReadValue(
            KeyView Key,
            DestinationBufferView Destination
        ) const noexcept {
            if (!IsReady_) {
                return {KeyValueReadStatus::NotReady, 0U, 0U, StorageSize{}};
            }

            char NativeKey[16U];

            const auto KeyStatus = CopyKey(
                Key,
                NativeKey
            );

            if (KeyStatus == KeyCopyStatus::TooLong) {
                return {KeyValueReadStatus::KeyTooLong, 0U, 0U, StorageSize{}};
            }

            if (KeyStatus == KeyCopyStatus::NotRepresentable) {
                return {KeyValueReadStatus::KeyNotRepresentable, 0U, 0U, StorageSize{}};
            }

            const auto SizeResult = GetValueSize(Key);

            if (SizeResult.Status == KeyValueSizeStatus::NotFound) {
                return {KeyValueReadStatus::NotFound, 0U, 0U, StorageSize{}};
            }

            if (SizeResult.Status == KeyValueSizeStatus::KeyTooLong) {
                return {KeyValueReadStatus::KeyTooLong, 0U, 0U, StorageSize{}};
            }

            if (SizeResult.Status == KeyValueSizeStatus::KeyNotRepresentable) {
                return {KeyValueReadStatus::KeyNotRepresentable, 0U, 0U, StorageSize{}};
            }

            if (SizeResult.Status != KeyValueSizeStatus::Succeeded) {
                return {KeyValueReadStatus::ProviderFailure, 0U, 0U, StorageSize{}};
            }

            const auto CompleteSize = static_cast<std::size_t>(SizeResult.Size.RawValue);
            const auto TransferSize = CompleteSize < Destination.Capacity ? CompleteSize : Destination.Capacity;

            if (TransferSize != 0U) {
                if (CompleteSize <= Destination.Capacity) {
                    if (Preferences_.getBytes(
                        NativeKey,
                        Destination.Address,
                        CompleteSize
                    ) != CompleteSize) {
                        return {KeyValueReadStatus::IoFailure, 0U, 0U, StorageSize{}};
                    }
                } else {
                    if (Preferences_.getBytes(
                        NativeKey,
                        ReadScratch_,
                        CompleteSize
                    ) != CompleteSize) {
                        return {KeyValueReadStatus::IoFailure, 0U, 0U, StorageSize{}};
                    }

                    ByteOperations_->CopyBytes(
                        Destination.Address,
                        ReadScratch_,
                        TransferSize
                    );
                }
            }

            std::uint8_t Facts = static_cast<std::uint8_t>(ReadFact::AvailableDataSizeIsKnown);

            if (CompleteSize > Destination.Capacity) {
                Facts |= static_cast<std::uint8_t>(ReadFact::WasTruncated);
            } else if (CompleteSize < Destination.Capacity) {
                Facts |= static_cast<std::uint8_t>(ReadFact::IsSmallerThanAvailableBuffer);
            }

            return {KeyValueReadStatus::Succeeded, Facts, TransferSize, SizeResult.Size};
        }

        /// Stores one complete opaque value.
        [[nodiscard]] KeyValueStoreStatus StoreValue(
            KeyView Key,
            SourceBufferView Source
        ) noexcept {
            if (!IsReady_) {
                return KeyValueStoreStatus::NotReady;
            }

            if (Source.Size > 512U) {
                return KeyValueStoreStatus::ValueTooLarge;
            }

            char NativeKey[16U];

            const auto KeyStatus = CopyKey(
                Key,
                NativeKey
            );

            if (KeyStatus == KeyCopyStatus::TooLong) {
                return KeyValueStoreStatus::KeyTooLong;
            }

            if (KeyStatus == KeyCopyStatus::NotRepresentable) {
                return KeyValueStoreStatus::KeyNotRepresentable;
            }

            if (Source.Size == 0U) {
                return Preferences_.putUChar(
                    NativeKey,
                    EmptyValueMarker
                ) == 1U ? KeyValueStoreStatus::Succeeded : KeyValueStoreStatus::IoFailure;
            }

            return Preferences_.putBytes(
                NativeKey,
                Source.Address,
                Source.Size
            ) == Source.Size ? KeyValueStoreStatus::Succeeded : KeyValueStoreStatus::IoFailure;
        }

        /// Removes one key.
        [[nodiscard]] KeyValueRemoveStatus RemoveKey(KeyView Key) noexcept {
            if (!IsReady_) {
                return KeyValueRemoveStatus::NotReady;
            }

            char NativeKey[16U];

            const auto KeyStatus = CopyKey(
                Key,
                NativeKey
            );

            if (KeyStatus == KeyCopyStatus::TooLong) {
                return KeyValueRemoveStatus::KeyTooLong;
            }

            if (KeyStatus == KeyCopyStatus::NotRepresentable) {
                return KeyValueRemoveStatus::KeyNotRepresentable;
            }

            if (!Preferences_.isKey(NativeKey)) {
                return KeyValueRemoveStatus::NotFound;
            }

            return Preferences_.remove(NativeKey) ? KeyValueRemoveStatus::Succeeded : KeyValueRemoveStatus::IoFailure;
        }

        /// Removes every key in the bound Preferences namespace.
        [[nodiscard]] KeyValueClearStatus ClearAllKeys() noexcept {
            if (!IsReady_) {
                return KeyValueClearStatus::NotReady;
            }

            return Preferences_.clear() ? KeyValueClearStatus::Succeeded : KeyValueClearStatus::IoFailure;
        }

    };

} // ESPressio::Persistence::Arduino
