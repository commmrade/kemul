#include <fcntl.h>
#include <memory>
#include <pty.h>
#include <ranges>
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
#include <algorithm>




Window::Window(const std::string& font_path) {
    std::cout << "Creating window\n";
    init();
    load_font(font_path);
    
    auto max_size = get_max_texture_size();
    glyph_cache_ = std::make_unique<GlyphCache>(renderer_, font_, max_size.first, max_size.second);
}
Window::~Window() {
    SDL_DestroyWindow(window_);
    SDL_DestroyRenderer(renderer_);
    TTF_CloseFont(font_);
}

std::pair<int, int> Window::get_max_texture_size() const {
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer_, &info);
    return {info.max_texture_width, info.max_texture_height};
}
std::pair<int, int> Window::get_font_size() const {
    int width, height;
    TTF_SizeText(font_, " ", &width, &height);
    return {width, height};
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

void Window::draw(const TermBuffer& term_buffer) {
    if (!should_render_) {
        SDL_Delay(16);
        return;
    }
    cursor_pos_.x = 10;
    cursor_pos_.y = 10;

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    decltype(auto) buffer = term_buffer.get_buffer();
    for (auto i = scroll_offset_; i < buffer.size(); ++i) {
    
        for (auto cell : buffer[i]) {
            if (cell.codepoint == 0) cell.codepoint = ' ';
            auto* atlas = glyph_cache_->atlas();
            
            SDL_Rect src = glyph_cache_->get_or_create_glyph_pos(renderer_, cell.codepoint);
            SDL_Rect glyph_rect{cursor_pos_.x, cursor_pos_.y, src.w, src.h};

            SDL_SetTextureColorMod(atlas, cell.fg_color.r, cell.fg_color.g, cell.fg_color.b);

            if (cell.bg_color != SDL_Color{0, 0, 0, 255}) { // Default background
                SDL_SetRenderDrawColor(renderer_, cell.bg_color.r, cell.bg_color.g, cell.bg_color.b, cell.bg_color.a);
                SDL_Rect glyph_rect{ cursor_pos_.x, cursor_pos_.y, src.w, src.h};
                SDL_RenderFillRect(renderer_, &glyph_rect);
                SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            }

            if (cell.is_underline()) {
                SDL_SetRenderDrawColor(renderer_, cell.fg_color.r, cell.fg_color.g, cell.fg_color.b, cell.fg_color.a);
                SDL_RenderDrawLine(renderer_, cursor_pos_.x, cursor_pos_.y + src.h - src.h / 5, cursor_pos_.x + src.w, cursor_pos_.y + src.h - src.h / 5);
            }

            if (cell.is_bold()) {
                SDL_SetTextureColorMod(atlas, 255, 255, 255);
            }

            if (cell.is_strikethrough()) {
                SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
                SDL_RenderDrawLine(renderer_, cursor_pos_.x, cursor_pos_.y + src.h / 2, cursor_pos_.x + src.w, cursor_pos_.y + src.h / 2);
            }

            SDL_RenderCopy(renderer_, atlas, &src, &glyph_rect);

            

            cursor_pos_.x += src.w;

            
        }
        cursor_pos_.x = 10;
        cursor_pos_.y += TTF_FontHeight(font_);
        // if (i != buffer.size() - 1) {
        //     // cursor_pos_.y += TTF_FontHeight(font_);
        //     // cursor_pos_.x = 10;
        // }
    }


    auto font_size = get_font_size();
    auto [cursor_x, cursor_y] = term_buffer.get_cursor_pos();
    
    const auto& row = buffer[cursor_y];
    SDL_Rect cursor_rect{cursor_x * font_size.first + font_size.first, (cursor_y - (int)scroll_offset_) * font_size.second + font_size.second / 2, 10, font_size.second};
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer_, &cursor_rect);    

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

void Window::set_window_title(const std::string& win_title) {
    SDL_SetWindowTitle(window_, win_title.c_str());
}