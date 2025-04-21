#pragma once
#include "SDL2/SDL_stdinc.h"
#include <SDL_pixels.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unicode/uchar.h>

inline int cell_width(uint32_t codepoint) {
    if (codepoint == 0) return 1;
    if (u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH) == U_EA_FULLWIDTH ||
        u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH) == U_EA_WIDE) {
        return 2;
    }
    return 1;
}



// 0000 0000 0000 0001 - underline
// 0000 0000 0000 0010 - bold
// 0000 0000 0000 0100 - strikethrough


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
};

class TermBuffer {
private:
    std::vector<std::vector<Cell>> buffer_;
    int pos_x{0};
    int pos_y{0};

    int width_cells_;
    int height_cells_;

    int font_width_;
    int font_height_;


    std::string command_;

public:
    explicit TermBuffer(int width, int height, int font_width, int font_height);
    ~TermBuffer();

    void push_str(const std::string &str);
    void add_str(std::string str);
    void push_cells(std::vector<Cell> cells);
    void add_cells(std::vector<Cell> cells);

    void add_str_command(const char* sym);


    void clear_command();
    void clear_all();

    void set_cursor(int row, int col);
    void reset_cursor(bool x_dir, bool y_dir);

    void resize(int new_width, int new_height, int font_width, int font_height);
    void reset();

    void cursor_down();

    const std::vector<std::vector<Cell>>& get_buffer() const {
        return buffer_;
    }
    const std::string& get_command() const {
        return command_;
    }
};
