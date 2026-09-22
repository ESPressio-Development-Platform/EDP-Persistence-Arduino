#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

typedef enum {
    PT_I8,
    PT_U8,
    PT_I16,
    PT_U16,
    PT_I32,
    PT_U32,
    PT_I64,
    PT_U64,
    PT_STR,
    PT_BLOB,
    PT_INVALID
} PreferenceType;


class Preferences {
private:

    struct Entry final {
        PreferenceType Type;
        std::vector<std::uint8_t> Data;
    };

    std::map<std::string, Entry> Entries_;
    bool IsReady_;
    bool IsReadOnly_;

public:

    Preferences() noexcept
        : IsReady_(false),
          IsReadOnly_(false) {}

    [[nodiscard]] bool begin(
        const char* Namespace,
        bool ReadOnly = false,
        const char* PartitionLabel = nullptr
    ) noexcept {
        static_cast<void>(PartitionLabel);

        if (Namespace == nullptr || IsReady_) {
            return false;
        }

        IsReady_ = true;
        IsReadOnly_ = ReadOnly;
        return true;
    }

    void end() noexcept {
        IsReady_ = false;
    }

    [[nodiscard]] bool clear() noexcept {
        if (!IsReady_ || IsReadOnly_) {
            return false;
        }

        Entries_.clear();
        return true;
    }

    [[nodiscard]] bool remove(const char* Key) noexcept {
        if (!IsReady_ || IsReadOnly_ || Key == nullptr) {
            return false;
        }

        return Entries_.erase(std::string(Key)) == 1U;
    }

    [[nodiscard]] std::size_t putUChar(
        const char* Key,
        std::uint8_t Value
    ) noexcept {
        if (!IsReady_ || IsReadOnly_ || Key == nullptr) {
            return 0U;
        }

        Entries_[std::string(Key)] = Entry{
            PT_U8,
            std::vector<std::uint8_t>{Value}
        };
        return 1U;
    }

    [[nodiscard]] std::size_t putBytes(
        const char* Key,
        const void* Value,
        std::size_t Length
    ) noexcept {
        if (!IsReady_ ||
            IsReadOnly_ ||
            Key == nullptr ||
            Value == nullptr ||
            Length == 0U) {
            return 0U;
        }

        const auto* Bytes =
            static_cast<const std::uint8_t*>(Value);

        Entries_[std::string(Key)] = Entry{
            PT_BLOB,
            std::vector<std::uint8_t>(
                Bytes,
                Bytes + Length
            )
        };
        return Length;
    }

    [[nodiscard]] bool isKey(const char* Key) noexcept {
        return getType(Key) != PT_INVALID;
    }

    [[nodiscard]] PreferenceType getType(const char* Key) noexcept {
        if (!IsReady_ || Key == nullptr) {
            return PT_INVALID;
        }

        const auto Iterator = Entries_.find(std::string(Key));
        return Iterator == Entries_.end()
            ? PT_INVALID
            : Iterator->second.Type;
    }

    [[nodiscard]] std::uint8_t getUChar(
        const char* Key,
        std::uint8_t DefaultValue = 0U
    ) noexcept {
        const auto Iterator = Key == nullptr
            ? Entries_.end()
            : Entries_.find(std::string(Key));

        if (!IsReady_ ||
            Iterator == Entries_.end() ||
            Iterator->second.Type != PT_U8 ||
            Iterator->second.Data.size() != 1U) {
            return DefaultValue;
        }

        return Iterator->second.Data[0];
    }

    [[nodiscard]] std::size_t getBytesLength(const char* Key) noexcept {
        const auto Iterator = Key == nullptr
            ? Entries_.end()
            : Entries_.find(std::string(Key));

        if (!IsReady_ ||
            Iterator == Entries_.end() ||
            Iterator->second.Type != PT_BLOB) {
            return 0U;
        }

        return Iterator->second.Data.size();
    }

    [[nodiscard]] std::size_t getBytes(
        const char* Key,
        void* Destination,
        std::size_t MaximumLength
    ) noexcept {
        const auto Iterator = Key == nullptr
            ? Entries_.end()
            : Entries_.find(std::string(Key));

        if (!IsReady_ ||
            Iterator == Entries_.end() ||
            Iterator->second.Type != PT_BLOB) {
            return 0U;
        }

        const auto Length = Iterator->second.Data.size();

        if (Destination == nullptr || MaximumLength == 0U) {
            return Length;
        }

        if (Length > MaximumLength) {
            return 0U;
        }

        std::memcpy(
            Destination,
            Iterator->second.Data.data(),
            Length
        );
        return Length;
    }

};
