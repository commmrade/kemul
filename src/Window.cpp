#include <algorithm>
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
#include <utf8cpp/utf8.h>
#include <utf8cpp/utf8/cpp11.h>
#include "Color.hpp"


Window::Window(const std::string& font_path, int font_ptsize, int width, int height) : width_(width), height_(height), font_ptsize_(font_ptsize) {
    init();
    load_font(font_path);

    auto max_size = get_max_texture_size();
    glyph_cache_ = std::make_unique<GlyphCache>(renderer_, font_, max_size);

    auto font_size = get_font_size();
    buffer_ = std::make_unique<TermBuffer>(width, height, font_size.first, font_size.second);
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
    window_ = SDL_CreateWindow("Kemul", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_, height_, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!window_) {
        throw std::runtime_error(std::string{"Could not create window: "} + SDL_GetError());
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        throw std::runtime_error(std::string{"Could not create renderer: "} + SDL_GetError());
    }
}

void Window::load_font(const std::string& font_path) {
    font_ = TTF_OpenFont(font_path.c_str(), font_ptsize_);
    if (!font_) {
        throw std::runtime_error(std::string{"Could not load a font: "} + TTF_GetError());
    }
}
void Window::set_should_render(bool value) {
    should_render_ = value;
}

void Window::draw() {
    if (!should_render_) {
        SDL_Delay(16);
        return;
    }

    auto font_size = get_font_size();
    cursor_pos_.x = 10;
    cursor_pos_.y = font_size.second / 2;

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    const auto& buffer = buffer_->get_buffer();
    auto* atlas = glyph_cache_->atlas();
    auto render_limit = get_window_size().second / font_size.second - 1;
    auto [t_cursor_x, t_cursor_y] = buffer_->get_cursor_pos();
    if (t_cursor_y > (int)scroll_offset_ + render_limit - 1 && !is_scrolling_) {
        scroll_offset_ += t_cursor_y - (scroll_offset_ + render_limit) + 1;
    }
    for (int i = scroll_offset_; i < std::min((int)scroll_offset_ + render_limit, (int)buffer.size()); ++i) {
        for (auto cell : buffer[i]) {
            if (cell.codepoint == 0) cell.codepoint = ' ';

            SDL_Rect src = glyph_cache_->get_or_create_glyph_pos(renderer_, font_, cell.codepoint);
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
        cursor_pos_.y += font_size.second;
    }

    SDL_Rect cursor_rect{t_cursor_x * font_size.first + font_size.first, (t_cursor_y - (int)scroll_offset_) * font_size.second + font_size.second / 2, font_size.first, font_size.second};
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer_, &cursor_rect);
    SDL_RenderPresent(renderer_);
    should_render_ = false;
}

void Window::scroll(Sint32 dir) {
    auto cursor_pos = buffer_->get_cursor_pos();
    if (dir < 0) {
        if (cursor_pos.second - scroll_offset_ + 1 < height_ / TTF_FontHeight(font_)) {
            is_scrolling_ = false;
            return;
        }
        scroll_offset_ += scroll_step_;
    } else if (dir > 0) {
        if ((int)scroll_offset_ >= scroll_step_) {
            scroll_offset_ -= scroll_step_;
            is_scrolling_ = true;
        }
    }
    set_should_render(true);
}

void Window::clear_buf(bool remove) {
    if (remove) {
        buffer_->clear_all();
        set_scroll_offset(0);
    } else {
        auto [x, y] = get_cursor_pos();
        buffer_->set_cursor_position(buffer_->get_max_y() + 1, x);
        set_scroll_offset(buffer_->get_max_y());
    }
    set_should_render(true);
}

std::pair<int, int> Window::get_cursor_pos() const {
    return buffer_->get_cursor_pos();
}

void Window::on_selection(int x, int y) {
    mouse_x = x;
    mouse_y = y;

    if (mouse_start_x != -1) {
        remove_selection();
        set_selection(mouse_start_x, mouse_start_y, mouse_x, mouse_y);
        mouse_end_x = mouse_x;
        mouse_end_y = mouse_y;
    }
}
void Window::on_remove_selection() {
    remove_selection();

    mouse_start_x = mouse_x;
    mouse_start_y = mouse_y;
}
void Window::reset_selection() {
    mouse_start_x = -1;
    mouse_start_y = -1;
    mouse_end_x = -1;
    mouse_end_y = -1;
}

void Window::set_selection(int start_x, int start_y, int end_x, int end_y) {
    buffer_->set_selection(start_x, start_y, end_x, end_y, get_scroll_offset());
    set_should_render(true);
}

void Window::remove_selection() {
    buffer_->remove_selection();
    set_should_render(true);
}

void Window::erase_at_end() {
    buffer_->erase_last_symbol();
}

void Window::set_cursor(int row, int col) {
    buffer_->set_cursor_position(row, col);
}

void Window::move_cursor(int row, int col) {
    buffer_->move_cursor_pos_relative(row, col);
}

void Window::add_cells(std::vector<Cell>&& cells) {
    buffer_->add_cells(std::move(cells));
}

void Window::reset_cursor(bool x_dir, bool y_dir) {
    buffer_->reset_cursor(x_dir, y_dir);
}

void Window::erase_in_line(int mode) {
    buffer_->erase_in_line(mode);
}

void Window::insert_chars(int n) {
    buffer_->insert_chars(n);
}

void Window::delete_chars(int n) {
    buffer_->delete_chars(n);
}

std::string Window::get_selected_text() const {
    return buffer_->get_selected_text();
}

void Window::resize() {
    auto dim = get_window_size();
    height_ = dim.second;
    width_ = dim.first;

    auto font_size = get_font_size();
    buffer_->resize(dim, font_size);
    set_should_render(true);
}

void Window::set_window_title(const std::string& win_title) {
    SDL_SetWindowTitle(window_, win_title.c_str());
}
