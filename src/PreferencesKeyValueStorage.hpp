#pragma once

#include <cstring>

#include <Preferences.h>

#include <ESPressio_Persistence.hpp>

namespace ESPressio::Persistence::Arduino {

    namespace Framework = ESPressio::System::CompositionFramework;


    /// TBindingTag distinguishes independently selectable Preferences namespaces in Composition.
    template<class TBindingTag>
    class PreferencesKeyValueStorage final : public Framework::Provider<
        Domain,
        Framework::Provides<
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
                Framework::PropertyValue<KeyValueFailurePreservation, FailurePreservation::PreservesCommittedState>,
                Framework::PropertyValue<KeyValueInterruptionAtomicity, InterruptionAtomicity::PowerLoss>,
                Framework::PropertyValue<ClearAllFailurePreservation, FailurePreservation::MayModify>,
                Framework::PropertyValue<ClearAllInterruptionAtomicity, InterruptionAtomicity::PowerLoss>
            >
        >
    > {
    private:

        // Bound Preferences namespace.

        /// Arduino Preferences object owning the open NVS handle.
        mutable Preferences Preferences_;

        /// Bounded scratch space used only when the caller requests a truncated blob read.
        mutable std::uint8_t ReadScratch_[512U];

        /// Indicates whether Begin() successfully opened the namespace.
        bool IsReady_;

        /// Converts a validated EDP key to the null-terminated representation required by Preferences.
        [[nodiscard]] static bool CopyKey(
            KeyView Key,
            char (&Buffer)[16U]
        ) noexcept {
            if (Key.Size() > 15U) {
                return false;
            }

            for (std::size_t Index = 0U; Index < Key.Size(); ++Index) {
                Buffer[Index] = Key.Data()[Index];
            }

            Buffer[Key.Size()] = '\0';
            return true;
        }

    public:

        // Lifecycle controlled by Bootstrap/application wiring.

        /// Constructs an unopened Preferences provider.
        PreferencesKeyValueStorage() noexcept
            : Preferences_(),
              IsReady_(false) {}

        /// Opens the caller-selected Preferences namespace.
        [[nodiscard]] bool Begin(const char* Namespace) noexcept {
            if (IsReady_) {
                return true;
            }

            IsReady_ = Preferences_.begin(
                Namespace,
                false
            );
            return IsReady_;
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

            if (!CopyKey(Key, NativeKey)) {
                return {KeyValueSizeStatus::KeyTooLong, StorageSize{}};
            }

            if (!Preferences_.isKey(NativeKey)) {
                return {KeyValueSizeStatus::NotFound, StorageSize{}};
            }

            return {
                KeyValueSizeStatus::Succeeded,
                StorageSize{Preferences_.getBytesLength(NativeKey)}
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

            if (!CopyKey(Key, NativeKey)) {
                return {KeyValueReadStatus::KeyTooLong, 0U, 0U, StorageSize{}};
            }

            if (!Preferences_.isKey(NativeKey)) {
                return {KeyValueReadStatus::NotFound, 0U, 0U, StorageSize{}};
            }

            const auto SizeResult = GetValueSize(Key);
            const auto CompleteSize = static_cast<std::size_t>(SizeResult.Size.RawValue);

            if (CompleteSize > 512U) {
                return {KeyValueReadStatus::ProviderFailure, 0U, 0U, StorageSize{}};
            }
            const auto TransferSize = CompleteSize < Destination.Capacity ? CompleteSize : Destination.Capacity;

            if (TransferSize != 0U) {
                if (CompleteSize <= Destination.Capacity) {
                    if (Preferences_.getBytes(NativeKey, Destination.Address, CompleteSize) != CompleteSize) {
                        return {KeyValueReadStatus::IoFailure, 0U, 0U, StorageSize{}};
                    }
                } else {
                    if (Preferences_.getBytes(NativeKey, ReadScratch_, CompleteSize) != CompleteSize) {
                        return {KeyValueReadStatus::IoFailure, 0U, 0U, StorageSize{}};
                    }

                    std::memcpy(
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

            if (Key.Size() > 15U) {
                return KeyValueStoreStatus::KeyTooLong;
            }

            if (Source.Size > 512U) {
                return KeyValueStoreStatus::ValueTooLarge;
            }

            char NativeKey[16U];

            if (!CopyKey(Key, NativeKey)) {
                return KeyValueStoreStatus::KeyTooLong;
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

            if (!CopyKey(Key, NativeKey)) {
                return KeyValueRemoveStatus::KeyTooLong;
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
