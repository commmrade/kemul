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

struct Cell {
    uint32_t codepoint;
    SDL_Color fg_color{200, 200, 200, 255};
    SDL_Color bg_color;
    uint16_t flags; // Underline, bold, etc
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

    void add_str_command(const char* sym) {
        command_.append(sym, SDL_strlen(sym));
        add_str(sym);
    }

    void clear_command() {
        command_.clear();
        command_ = "";
    }

    void set_cursor(int row, int col);

    void resize(int new_width, int new_height);

    const std::vector<std::vector<Cell>>& get_buffer() const {
        return buffer_;
    }
    const std::string& get_command() const {
        return command_;
    }
};
