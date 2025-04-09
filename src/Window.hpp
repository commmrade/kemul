#pragma once
#include <cstdint>
#include <memory>
#include <string_view>
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

constexpr int FONT_PTSIZE = 14;

struct CursorPos {
    int x{0};
    int y{0};
};

class Window {
private:
    // Helper stuff
    uint scroll_offset_{0};
    CursorPos cursor_pos_;
    uint curs_char_idx_;

    int8_t scroll_step_{2};

    bool is_running_{true};
    bool should_render_{true};

    // Smth else like glyph cache


    // Drawing (SDL) stuff;
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    TTF_Font* font_{nullptr};

    std::unique_ptr<GlyphCache> glyph_cache;
public:
    explicit Window(const std::string& font_path);
    ~Window();

    void process();
    void draw(const TermBuffer& buffer);

    void set_should_render(bool value);

    std::pair<int, int> get_max_texture_size();

    void scroll(Sint32 dir);

private:
    void load_font(const std::string& font_path);
    void init();
};