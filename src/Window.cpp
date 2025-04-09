#include <fcntl.h>
#include <memory>
#include <pty.h>
#include <stdexcept>
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
#include <string>
#include <termios.h>
#include <unistd.h>
#include "Window.hpp"
#include "GlyphCache.hpp"
#include <iostream>
#include <utf8cpp/utf8.h>
#include <utf8cpp/utf8/cpp11.h>

Window::Window(const std::string& font_path) {
    std::cout << "Creating window\n";
    init();
    load_font(font_path);
    
    auto max_size = get_max_texture_size();
    glyph_cache = std::make_unique<GlyphCache>(renderer_, font_, max_size.first, max_size.second);
}
Window::~Window() {
    SDL_DestroyWindow(window_);
    SDL_DestroyRenderer(renderer_);
    TTF_CloseFont(font_);
}

std::pair<int, int> Window::get_max_texture_size() {
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer_, &info);
    return {info.max_texture_width, info.max_texture_height};
}

void Window::init() {
    std::cout << "Creating SDL_Window\n";
    window_ = SDL_CreateWindow("Kemul", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 900, 600, SDL_WINDOW_SHOWN);
    if (!window_) {
        throw std::runtime_error(std::string{"Could not create window: "} + SDL_GetError());
    }
    
    std::cout << "Creating SDL_Renderer\n";
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        throw std::runtime_error(std::string{"Could not create renderer: "} + SDL_GetError());
    }
}

void Window::load_font(const std::string& font_path) {
    font_ = TTF_OpenFont(font_path.c_str(), FONT_PTSIZE);
    if (!font_) {
        throw std::runtime_error(std::string{"Could not load a font: "} + TTF_GetError());
    }
}
void Window::set_should_render(bool value) {
    should_render_ = value;
}

void Window::process() {

}

void Window::draw(const TermBuffer& buffer) {

    if (!should_render_) {
        SDL_Delay(16);
        return;
    }
    cursor_pos_.x = 10;
    cursor_pos_.y = 10;

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    for (auto i = scroll_offset_; i < buffer.get_buffer().size(); ++i) {
        std::vector<uint32_t> codepoints;
        utf8::utf8to32(buffer.get_buffer()[i].cbegin(), buffer.get_buffer()[i].cend(), std::back_inserter(codepoints));
        for (auto codepoint : codepoints) {
            
            auto* atlas = glyph_cache->atlas();

            SDL_Rect src = glyph_cache->get_or_create_glyph_pos(renderer_, codepoint);
            SDL_Rect glyph_rect{cursor_pos_.x, cursor_pos_.y, src.w, src.h};
            SDL_RenderCopy(renderer_, atlas, &src, &glyph_rect);

            cursor_pos_.x += src.w;
            if (cursor_pos_.x >= 900 - 50) {
                cursor_pos_.x = 10;
                cursor_pos_.y += src.h;
            }
        }
        if (i != buffer.get_buffer().size() - 1) {
            cursor_pos_.y += TTF_FontHeight(font_);
            cursor_pos_.x = 10;
        }
    }
    std::vector<uint32_t> codepoints;
    utf8::utf8to32(buffer.get_command().cbegin(), buffer.get_command().cend(), std::back_inserter(codepoints));
    for (auto codepoint : codepoints) {

        auto* atlas = glyph_cache->atlas();

        SDL_Rect src = glyph_cache->get_or_create_glyph_pos(renderer_, codepoint);
        SDL_Rect glyph_rect{cursor_pos_.x, cursor_pos_.y, src.w, src.h};
        SDL_RenderCopy(renderer_, atlas, &src, &glyph_rect);

        cursor_pos_.x += src.w;
        if (cursor_pos_.x >= 900 - 50) {
            cursor_pos_.x = 10;
            cursor_pos_.y += src.h;
        }
    }

    SDL_RenderPresent(renderer_);
    should_render_ = false;
}

void Window::scroll(Sint32 dir) {
  
    if (dir < 0) {
        scroll_offset_ += scroll_step_;
    } else if (dir > 0) {
        if (scroll_offset_ >= scroll_step_) {
            scroll_offset_ -= scroll_step_;
        }
    }
}