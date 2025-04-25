#pragma once
#include "SDL2/SDL_stdinc.h"
#include <SDL_pixels.h>
#include <cstdint>
#include <string>
#include <utility>
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
    int max_pos_y{0};

    int width_cells_;
    int height_cells_;

    int font_width_; // TODO: Remove?
    int font_height_;

    // mouse selection
    std::pair<int, int> mouse_start_cell{-1, -1};
    std::pair<int, int> mouse_end_cell{-1, -1};


public:
    explicit TermBuffer(int width, int height, int font_width, int font_height);
    ~TermBuffer();

    void add_cells(std::vector<Cell> cells);

    void clear_all();

    void set_cursor_position(int row, int col);
    void move_cursor_pos_relative(int row, int col);
    void reset_cursor(bool x_dir, bool y_dir);


    void resize(int new_width, int new_height, int font_width, int font_height);
    void reset();

    void cursor_down();
    void expand_down(int n = 1);


    void erase_last_symbol();
    void erase_in_line(int mode);

    void insert_chars(int n); // Insert n spaces henceforth shifting existing chars to the right
    void delete_chars(int n); // Delete n chars henceforth shiting existing chars to the left

    // Mouse selection methods
    void set_selection(int start_x, int start_y, int end_x, int end_y, int scroll_offset); // Invert colors to signal 'selection'
    void remove_selection(); // Unset mouse selection vaiables and undo 
    std::string get_selected_text() const;


    const std::vector<std::vector<Cell>>& get_buffer() const {
        return buffer_;
    }

    const std::pair<int, int> get_cursor_pos() const {
        return {pos_x, pos_y};
    }
    const int get_max_y() const {
        return max_pos_y;
    }
};
