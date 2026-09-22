#pragma once

#include <FS.h>


class SDStub final : public fs::FS {
public:

    [[nodiscard]] bool begin() noexcept {
        return true;
    }

};


inline SDStub SD;
