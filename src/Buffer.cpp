#include "Buffer.hpp"
#include <cstdint>
#include <exception>
#include <iterator>
#include <vector>
#include <utf8cpp/utf8.h>
#include <iostream>





TermBuffer::TermBuffer(int width, int height, int font_width, int font_height) : font_width_(font_width), font_height_(font_height) {
    width_cells_ = width / font_width - 1;
    height_cells_ = height / font_height;
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
}

TermBuffer::~TermBuffer() {

}

void TermBuffer::clear_all() {
    reset();
}


void TermBuffer::resize(int new_width, int new_height, int font_width, int font_height) {

}
void TermBuffer::reset() {
    buffer_.clear();
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
    pos_x = 0;
    pos_y = 0;
}

void TermBuffer::set_cursor_position(int row, int col) {
    pos_x = std::max(0, std::min(col - 1, width_cells_ - 1));
    if (row - 1 >= buffer_.size()) {
        expand_down(row - buffer_.size());
    }
    pos_y = std::max(0, row - 1);
}

void TermBuffer::move_cursor_pos_relative(int row, int col) {
    pos_x += col;
    if (pos_x >= width_cells_) {
        cursor_down();
    }

    if (pos_y + row < 0) {
        return;
    } else if (pos_y + row >= buffer_.size()) {
        expand_down(pos_y + row - buffer_.size());
    }
    pos_y += row;
}

void TermBuffer::reset_cursor(bool x_dir, bool y_dir) {
    if (x_dir) {
        pos_x = 0;
    }
    if (y_dir) {
        pos_y = 0;
    }
}

void TermBuffer::add_cells(std::vector<Cell> cells) {
    for (auto &&cell : cells) {
        if (cell.codepoint == 0x0A) { // If newline
            cursor_down();
            reset_cursor(true, false);
            continue;
        }

        buffer_[pos_y][pos_x] = std::move(cell);
        
        pos_x += 1;
        if (pos_x >= width_cells_) {
            cursor_down();
            pos_x = 0;
        }
    }
}


void TermBuffer::cursor_down() {
    if (++pos_y == buffer_.size()) {
        buffer_.emplace_back(width_cells_);
    }
}

void TermBuffer::expand_down(int n) {
    for (auto i = 0; i < n; i++) {
        buffer_.emplace_back(width_cells_);
    }
}

void TermBuffer::erase_last_symbol() { // TODO CORNER CASES
    buffer_[pos_y][pos_x].codepoint = ' ';
    pos_x--;

    if (pos_x < 0) {
        pos_y--;
        pos_x = width_cells_ - 1;
    }
}