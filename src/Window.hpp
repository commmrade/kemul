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

    int8_t scroll_step_{2};

    bool is_running_{true};
    bool is_scrolling_{false};
    bool should_render_{true};

    // Smth else like glyph cache


    // Drawing (SDL) stuff;
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    
    

    std::unique_ptr<GlyphCache> glyph_cache_;

public:
    TTF_Font* font_{nullptr}; // temp
    explicit Window(const std::string& font_path, int font_ptsize, int width, int height);
    ~Window();

    void draw(const TermBuffer& term_buffer);
    void set_should_render(bool value);

    std::pair<int, int> get_max_texture_size() const;
    std::pair<int, int> get_font_size() const;

    void scroll(Sint32 dir, std::pair<int, int> cursor_pos, int max_y);
    void resize();

    void set_window_title(const std::string& win_title);
    void set_scroll_offset(int n) {
        scroll_offset_ = n;
    }
    const int get_scroll_offset() const {
        return scroll_offset_;
    }

    const std::pair<int, int> get_window_size() {
        int w, h;
        SDL_GetWindowSize(window_, &w, &h);
        return {w, h};
    }

private:
    void load_font(const std::string& font_path);
    void init();
};