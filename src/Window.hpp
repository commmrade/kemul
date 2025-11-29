#pragma once
#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <utility>
#include "Buffer.hpp"
#include "GlyphCache.hpp"
#include <unicode/uchar.h>

// echo -e "\033[48;5;2m Test Backgroundsdasd \033[0m"

// inline int cell_width(uint32_t codepoint) {
//     if (codepoint == 0) return 1;
//     if (u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH) == U_EA_FULLWIDTH ||
//         u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH) == U_EA_WIDE) {
//         return 2;
//     }
//     return 1;
// }



struct CursorPos {
    int x{0};
    int y{0};
};

class Window {
private:
    int width_; int height_;
    int font_ptsize_;
    // Helper stuff
    uint scroll_offset_{0};
    CursorPos cursor_pos_;
    uint curs_char_idx_;

    // Selection stuff
    int mouse_x{-1}, mouse_y{-1};
    int mouse_start_x{-1};
    int mouse_start_y{-1};
    int mouse_end_x{-1};
    int mouse_end_y{-1};

    int8_t scroll_step_{2};

    bool is_running_{true};
    bool is_scrolling_{false};
    bool should_render_{true};

    // Smth else like glyph cache


    // Drawing (SDL) stuff;
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};


    std::unique_ptr<TermBuffer> buffer_;
    std::unique_ptr<GlyphCache> glyph_cache_;

public:
    TTF_Font* font_{nullptr}; // temp
    explicit Window(const std::string& font_path, int font_ptsize, int width, int height);
    ~Window();

    // void draw(const TermBuffer& term_buffer);
    void draw();
    void set_should_render(bool value);

    std::pair<int, int> get_max_texture_size() const;
    std::pair<int, int> get_font_size() const;

    // void scroll(Sint32 dir, std::pair<int, int> cursor_pos, int max_y);
    void scroll(Sint32 dir);
    void resize();

    void set_window_title(const std::string& win_title);
    void set_scroll_offset(int n) {
        scroll_offset_ = n;
    }
    int get_scroll_offset() const {
        return scroll_offset_;
    }

    const std::pair<int, int> get_window_size() {
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        return {w, h};
    }

    // Selection tracking
    void on_selection(int x, int y);
    void on_remove_selection();
    void reset_selection();

    // Buffer stuff
    void clear_buf(bool remove = false);
    void set_selection(int start_x, int start_y, int end_x, int end_y);
    void remove_selection();
    void erase_at_end();
    void set_cursor(int row, int col);
    void move_cursor(int row, int col);
    void reset_cursor(bool x_dir, bool y_dir);
    std::pair<int, int> get_cursor_pos() const;
    void add_cells(std::vector<Cell>&& cells);
    void erase_in_line(int mode);
    void insert_chars(int n);
    void delete_chars(int n);
    std::string get_selected_text() const;
private:
    void load_font(const std::string& font_path);
    void init();
};
