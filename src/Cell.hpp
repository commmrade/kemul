#include "SDL_pixels.h"

struct Cell {
    uint32_t codepoint;
    SDL_Color fg_color{200, 200, 200, 255};
    SDL_Color bg_color{0, 0, 0, 255};
    uint16_t flags{0}; // Underline, bold, etc

    void set_underline(bool value = true) {
        if (!value) {
            flags &= ~0b0000'0000'0000'0001;
            return;
        }
        flags |= 0b0000'0000'0000'0001;
    }
    bool is_underline() const {
        return flags & 0b0000'0000'0000'0001;
    }

    void set_bold(bool value = true) {
        if (!value) {
            flags &= ~0b0000'0000'0000'0010;
            return;
        }
        flags |= 0b0000'0000'0000'0010;
    }
    bool is_bold() const {
        return flags & 0b0000'0000'0000'0010;
    }

    void set_strikethrough(bool value = true) {
        if (!value) {
            flags &= ~0b0000'0000'0000'0100;
            return;
        }
        flags |= 0b0000'0000'0000'0100;
    }
    bool is_strikethrough() const {
        return flags & 0b0000'0000'0000'0100;
    }

    void set_wrapline(bool value = true) {
        if (!value) {
            flags &= ~0b0000'0000'0000'1000;
            return;
        }
        flags |= 0b0000'0000'0000'1000;
    }
    bool is_wrapline() const {
        return flags & 0b0000'0000'0000'1000;
    }
    void clear() {
        codepoint = 0;
        fg_color = {200, 200, 200, 255};
        bg_color = {0, 0, 0, 255};
        flags = 0;
    }
};