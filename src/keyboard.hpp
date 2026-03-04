#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gfx {

class Keyboard {
public:
    bool isKeyDown(std::uint32_t key) const {
        if (key >= keyStates_.size()) {
            return false;
        }
        return keyStates_[key];
    }

    void setKeyState(std::uint32_t key, bool isDown) {
        if (key >= keyStates_.size()) {
            return;
        }
        keyStates_[key] = isDown;
    }

private:
    std::array<bool, 256> keyStates_{};
};

} // namespace gfx
