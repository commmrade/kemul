#include "Buffer.hpp"
#include <cstdint>
#include <iterator>
#include <vector>
#include <utf8cpp/utf8.h>
#include <iostream>





TermBuffer::TermBuffer(int width, int height, int font_width, int font_height) : font_width_(font_width), font_height_(font_height) {
    width_cells_ = width / font_width;
    height_cells_ = height / font_height;
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
}

TermBuffer::~TermBuffer() {

}

void TermBuffer::clear_all() {
    reset();
}

void TermBuffer::push_str(const std::string &str) {
    std::vector<uint32_t> codepoints;
    ++pos_y; // Add check
    pos_x = 0; // Reset x
    utf8::utf8to32(str.cbegin(), str.cend(), std::back_inserter(codepoints));
    for (auto codepoint : codepoints) {
        Cell cell;
        cell.codepoint = codepoint;


        buffer_[pos_y][pos_x] = std::move(cell);
        
        pos_x += 1;
        if (pos_x >= width_cells_) {
            std::cout << str << std::endl;
            ++pos_y;
            pos_x = 0;
        }
    }
}

void TermBuffer::resize(int new_width, int new_height, int font_width, int font_height) {

}
void TermBuffer::reset() {
    buffer_.clear();
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
    pos_x = 0;
    pos_y = 0;
}

void TermBuffer::set_cursor(int row, int col) {
    pos_x = col;
    pos_y = row;
    std::cout << "set cursor\n";
}
void TermBuffer::reset_cursor(bool x_dir, bool y_dir) {
    if (x_dir) {
        pos_x = 0;
    }
    if (y_dir) {
        pos_y = 0;
    }
}

void TermBuffer::push_cells(std::vector<Cell> cells) {
    cursor_down();
    pos_x = 0; // Reset x
    for (auto &&cell : cells) {
        buffer_[pos_y][pos_x] = std::move(cell);
        
        pos_x += 1;
        if (pos_x >= width_cells_) {
            cursor_down();
            pos_x = 0;
        }
    }
}

void TermBuffer::add_cells(std::vector<Cell> cells) {
    for (auto &&cell : cells) {
        buffer_[pos_y][pos_x] = std::move(cell);
        
        pos_x += 1;
        if (pos_x >= width_cells_) {
            cursor_down();
            pos_x = 0;
        }
    }
}
void TermBuffer::add_str(std::string str) {
    std::vector<uint32_t> codepoints;
    utf8::utf8to32(str.cbegin(), str.cend(), std::back_inserter(codepoints));

    for (auto codepoint : codepoints) {
        Cell cell;
        cell.codepoint = codepoint;
        
        buffer_[pos_y][pos_x] = cell;

        ++pos_x;
        if (pos_x >= width_cells_) {
            cursor_down();
            pos_x = 0;
        }
    }
}
void TermBuffer::add_str_command(const char* sym) {
    command_.append(sym, SDL_strlen(sym));
    add_str(sym);
}

void TermBuffer::clear_command() {
    command_.clear();
    command_ = "";
}

void TermBuffer::cursor_down() {
    if (++pos_y == buffer_.size()) {
        buffer_.emplace_back(width_cells_);
    }
}