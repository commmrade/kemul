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
// 0000 0000 0000 1000 - wrapline


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
};

class TermBuffer {
private:
    std::vector<std::vector<Cell>> buffer_;
    int cursor_x_{0};
    int cursor_y_{0};
    int max_pos_y_{1};

    int width_cells_;
    int height_cells_;

    std::pair<int, int> cell_size_;

    // mouse selection
    std::pair<int, int> mouse_start_cell{-1, -1};
    std::pair<int, int> mouse_end_cell{-1, -1};


public:
    explicit TermBuffer(int width, int height, int cell_width, int cell_height);
    ~TermBuffer();

    // Adding cells
    void add_cells(std::vector<Cell> cells);

    void clear_all();
    void reset();

    // Cursor
    void cursor_down();
    void set_cursor_position(int row, int col);
    void move_cursor_pos_relative(int row, int col);
    void reset_cursor(bool x_dir, bool y_dir);
    void cursor_up(int n = 1);
    void expand_down(int n = 1);

    // Chars manipulation
    void erase_last_symbol();
    void erase_in_line(int mode);
    void insert_chars(int n); // Insert n spaces henceforth shifting existing chars to the right
    void delete_chars(int n); // Delete n chars henceforth shiting existing chars to the left

    // Resize stuff
    void resize(std::pair<int, int> new_window_size, std::pair<int, int> font_size);
    void grow_lines(int n);
    void shrink_lines(int n);

    void grow_cols(int n, bool reflow);
    void shrink_cols(int n, bool reflow);


    // Mouse selection methods
    void set_selection(int start_x, int start_y, int end_x, int end_y, int scroll_offset); // Invert colors to signal 'selection'
    void remove_selection(); // Unset mouse selection vaiables and undo 
    std::string get_selected_text() const;

    // Some getters
    const std::vector<std::vector<Cell>>& get_buffer() const {
        return buffer_;
    }
    const std::pair<int, int> get_cursor_pos() const {
        return {cursor_x_, cursor_y_};
    }
    const int get_max_y() const {
        return max_pos_y_;
    }
};
