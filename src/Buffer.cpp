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

    while (pos_x < 0) {
        if (pos_y == 0) {
            pos_x = 0;
            break;
        }
        pos_y--;
        pos_x += width_cells_;
    }

    while (pos_x >= width_cells_) {
        pos_x -= width_cells_;
        cursor_down();
    }

    int new_row = pos_y + row;
    if (new_row < 0) {
        pos_y = 0;
    } else if (new_row >= buffer_.size()) {
        expand_down(new_row - buffer_.size() + 1);
        pos_y = new_row;
    } else {
        pos_y = new_row;
    }
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
            std::cout << "add cells down\n";
            cursor_down();
            reset_cursor(true, false);
            continue;
        }

        buffer_[pos_y][pos_x] = std::move(cell);
        
        pos_x += 1;
        // std::cout << pos_x << " " << pos_y << " " << width_cells_ << std::endl;
        if (pos_x >= width_cells_) {
            std::cout << "down\n";
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

void TermBuffer::erase_in_line(int mode) {
    // if (pos_y >= buffer_.size()) return;

    // int start = 0;
    // int end = width_cells_;

    // if (mode == 0) { // от курсора до конца строки
    //     start = pos_x;
    // } else if (mode == 1) { // от начала до курсора (включительно)
    //     end = pos_x + 1;
    // } else if (mode == 2) { // вся строка
    //     start = 0;
    //     end = width_cells_;
    // }

    // for (int x = start; x < end; ++x) {
    //     buffer_[pos_y][x] = Cell{}; // reset cell (codepoint=0)
    // }
}

void TermBuffer::erase_last_symbol() { 
    buffer_[pos_y][pos_x].codepoint = ' ';
    pos_x--;
    std::cout << pos_x << "cuck" << std::endl;
    if (pos_x < 0) {
        pos_y--;
        pos_x = width_cells_ - 1;
    }
}