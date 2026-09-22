#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"

namespace fs {

    class FS;


    class File final {
    private:

        FS* Owner_;
        std::string Path_;
        std::size_t Position_;
        std::size_t EnumerationIndex_;
        bool IsValid_;

    public:

        File() noexcept;

        File(
            FS& Owner,
            std::string Path,
            std::size_t Position
        ) noexcept;

        explicit operator bool() const noexcept;

        [[nodiscard]] bool isDirectory() const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const char* name() const noexcept;

        [[nodiscard]] bool seek(std::uint32_t Position) noexcept;

        [[nodiscard]] std::size_t read(
            std::uint8_t* Destination,
            std::size_t Size
        ) noexcept;

        [[nodiscard]] std::size_t write(
            const std::uint8_t* Source,
            std::size_t Size
        ) noexcept;

        void flush() noexcept;

        void close() noexcept;

        [[nodiscard]] File openNextFile() noexcept;

    };


    class FS {
    private:

        struct Entry final {
            bool IsDirectory;
            std::vector<std::uint8_t> Data;
        };

        std::map<std::string, Entry> Entries_;

        [[nodiscard]] static std::string ParentPath(const std::string& Path) {
            const auto Separator = Path.find_last_of('/');

            if (Separator == 0U) {
                return "/";
            }

            return Path.substr(
                0U,
                Separator
            );
        }

        [[nodiscard]] static bool IsDirectChild(
            const std::string& Parent,
            const std::string& Candidate
        ) {
            if (Candidate == Parent) {
                return false;
            }

            const auto Prefix = Parent == "/"
                ? std::string{"/"}
                : Parent + "/";

            if (!Candidate.starts_with(Prefix)) {
                return false;
            }

            return Candidate.find(
                '/',
                Prefix.size()
            ) == std::string::npos;
        }

        friend class File;

    public:

        FS() {
            Entries_.emplace(
                "/",
                Entry{true, {}}
            );
        }

        [[nodiscard]] File open(
            const char* Path,
            const char* Mode = FILE_READ
        ) noexcept {
            if (Path == nullptr || Mode == nullptr) {
                return {};
            }

            const std::string NativePath(Path);
            auto Iterator = Entries_.find(NativePath);

            if (Iterator == Entries_.end()) {
                if (std::strcmp(Mode, FILE_WRITE) != 0 &&
                    std::strcmp(Mode, FILE_APPEND) != 0) {
                    return {};
                }

                if (!exists(ParentPath(NativePath).c_str())) {
                    return {};
                }

                Iterator = Entries_.emplace(
                    NativePath,
                    Entry{false, {}}
                ).first;
            }

            if (Iterator->second.IsDirectory) {
                return File(
                    *this,
                    NativePath,
                    0U
                );
            }

            if (std::strcmp(Mode, FILE_WRITE) == 0) {
                Iterator->second.Data.clear();
            }

            const auto Position = std::strcmp(Mode, FILE_APPEND) == 0
                ? Iterator->second.Data.size()
                : 0U;

            return File(
                *this,
                NativePath,
                Position
            );
        }

        [[nodiscard]] bool exists(const char* Path) const noexcept {
            return Path != nullptr &&
                Entries_.contains(std::string(Path));
        }

        [[nodiscard]] bool remove(const char* Path) noexcept {
            if (Path == nullptr) {
                return false;
            }

            const auto Iterator = Entries_.find(std::string(Path));

            if (Iterator == Entries_.end() || Iterator->second.IsDirectory) {
                return false;
            }

            Entries_.erase(Iterator);
            return true;
        }

        [[nodiscard]] bool mkdir(const char* Path) noexcept {
            if (Path == nullptr) {
                return false;
            }

            const std::string NativePath(Path);

            if (Entries_.contains(NativePath) ||
                !exists(ParentPath(NativePath).c_str())) {
                return false;
            }

            Entries_.emplace(
                NativePath,
                Entry{true, {}}
            );
            return true;
        }

        [[nodiscard]] bool rmdir(const char* Path) noexcept {
            if (Path == nullptr) {
                return false;
            }

            const std::string NativePath(Path);
            const auto Iterator = Entries_.find(NativePath);

            if (Iterator == Entries_.end() || !Iterator->second.IsDirectory) {
                return false;
            }

            for (const auto& [Candidate, Entry] : Entries_) {
                static_cast<void>(Entry);

                if (IsDirectChild(NativePath, Candidate)) {
                    return false;
                }
            }

            Entries_.erase(Iterator);
            return true;
        }

        [[nodiscard]] bool rename(
            const char* Source,
            const char* Destination
        ) noexcept {
            if (Source == nullptr || Destination == nullptr) {
                return false;
            }

            const std::string NativeSource(Source);
            const std::string NativeDestination(Destination);
            auto Iterator = Entries_.find(NativeSource);

            if (Iterator == Entries_.end() ||
                Entries_.contains(NativeDestination) ||
                !exists(ParentPath(NativeDestination).c_str())) {
                return false;
            }

            auto Entry = std::move(Iterator->second);
            Entries_.erase(Iterator);
            Entries_.emplace(
                NativeDestination,
                std::move(Entry)
            );
            return true;
        }

    };


    inline File::File() noexcept
        : Owner_(nullptr),
          Position_(0U),
          EnumerationIndex_(0U),
          IsValid_(false) {}


    inline File::File(
        FS& Owner,
        std::string Path,
        std::size_t Position
    ) noexcept
        : Owner_(&Owner),
          Path_(std::move(Path)),
          Position_(Position),
          EnumerationIndex_(0U),
          IsValid_(true) {}


    inline File::operator bool() const noexcept {
        return IsValid_ &&
            Owner_ != nullptr &&
            Owner_->Entries_.contains(Path_);
    }


    inline bool File::isDirectory() const noexcept {
        if (!static_cast<bool>(*this)) {
            return false;
        }

        return Owner_->Entries_.at(Path_).IsDirectory;
    }


    inline std::size_t File::size() const noexcept {
        if (!static_cast<bool>(*this) || isDirectory()) {
            return 0U;
        }

        return Owner_->Entries_.at(Path_).Data.size();
    }


    inline const char* File::name() const noexcept {
        return Path_.c_str();
    }


    inline bool File::seek(std::uint32_t Position) noexcept {
        if (!static_cast<bool>(*this) ||
            isDirectory() ||
            Position > size()) {
            return false;
        }

        Position_ = Position;
        return true;
    }


    inline std::size_t File::read(
        std::uint8_t* Destination,
        std::size_t Size
    ) noexcept {
        if (!static_cast<bool>(*this) ||
            isDirectory() ||
            Destination == nullptr) {
            return 0U;
        }

        const auto& Data = Owner_->Entries_.at(Path_).Data;
        const auto Available = Data.size() - Position_;
        const auto Transfer = std::min(
            Size,
            Available
        );

        if (Transfer != 0U) {
            std::memcpy(
                Destination,
                Data.data() + Position_,
                Transfer
            );
        }

        Position_ += Transfer;
        return Transfer;
    }


    inline std::size_t File::write(
        const std::uint8_t* Source,
        std::size_t Size
    ) noexcept {
        if (!static_cast<bool>(*this) ||
            isDirectory() ||
            (Source == nullptr && Size != 0U)) {
            return 0U;
        }

        auto& Data = Owner_->Entries_.at(Path_).Data;

        if (Position_ + Size > Data.size()) {
            Data.resize(Position_ + Size);
        }

        if (Size != 0U) {
            std::memcpy(
                Data.data() + Position_,
                Source,
                Size
            );
        }

        Position_ += Size;
        return Size;
    }


    inline void File::flush() noexcept {}


    inline void File::close() noexcept {
        IsValid_ = false;
    }


    inline File File::openNextFile() noexcept {
        if (!static_cast<bool>(*this) || !isDirectory()) {
            return {};
        }

        std::size_t Current = 0U;

        for (auto& [Candidate, Entry] : Owner_->Entries_) {
            static_cast<void>(Entry);

            if (!FS::IsDirectChild(Path_, Candidate)) {
                continue;
            }

            if (Current++ < EnumerationIndex_) {
                continue;
            }

            ++EnumerationIndex_;
            return File(
                *Owner_,
                Candidate,
                0U
            );
        }

        return {};
    }

} // namespace fs
