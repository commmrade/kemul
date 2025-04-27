#include "Buffer.hpp"
#include <algorithm>
#include <iterator>
#include <utf8cpp/utf8/cpp17.h>
#include <vector>
#include <utf8cpp/utf8.h>
#include <iostream>
#include <ranges>



TermBuffer::TermBuffer(int width, int height, int font_width, int font_height) : font_width_(font_width), font_height_(font_height) {
    width_cells_ = width / font_width - 1; 
    height_cells_ = height / font_height - 1;
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
}

TermBuffer::~TermBuffer() {

}

void TermBuffer::clear_all() {
    reset();
}


void TermBuffer::reset() {
    buffer_.clear();
    buffer_.resize(height_cells_, std::vector<Cell>(width_cells_));
    pos_x = 0;
    pos_y = 0;
    max_pos_y = 0;
}

void TermBuffer::set_cursor_position(int row, int col) {
    pos_x = std::max(0, std::min(col - 1, width_cells_ - 1));
    if (row - 1 >= buffer_.size()) {
        expand_down(row - buffer_.size());
    }
    pos_y = std::max(0, row - 1);
}
void TermBuffer::move_cursor_pos_relative(int d_row, int d_col) {
    int new_y = pos_y + d_row;
    new_y = std::max(0, std::min(new_y, (int)buffer_.size() - 1));
    pos_y = new_y;

    int new_x = pos_x + d_col;
    new_x = std::max(0, std::min(new_x, width_cells_ - 1));
    pos_x = new_x;
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
            // Set wrapline flag
            buffer_[pos_y][pos_x - 1].set_wrapline();
            cursor_down();
            pos_x = 0;
        }
    }
}


void TermBuffer::cursor_down() {
    if (++pos_y == buffer_.size()) {
        expand_down();
    }
    max_pos_y = std::max(pos_y, max_pos_y);
}

void TermBuffer::expand_down(int n) {
    for (auto i = 0; i < n; i++) {
        buffer_.emplace_back(width_cells_);
    }
    height_cells_ += n;
}


void TermBuffer::erase_in_line(int mode) {
    if (pos_y >= buffer_.size()) return;

    int start = 0;
    int end = width_cells_;

    if (mode == 0) { // fron cursor to the ened
        start = pos_x;
    } else if (mode == 1) { // from start to cursor
        end = pos_x + 1;
        std::cout << "heh\n";
    } else if (mode == 2) { // whole line
        start = 0;
        end = width_cells_;
    }

    for (int x = start; x < end; ++x) {
        buffer_[pos_y][x].codepoint = ' '; // reset cel
        buffer_[pos_y][x].flags = 0;
    }
}

void TermBuffer::erase_last_symbol() { 
    buffer_[pos_y][pos_x].codepoint = ' ';
    buffer_[pos_y][pos_x].flags = 0;
    pos_x--;
    if (pos_x < 0) {
        pos_y--;
        pos_x = width_cells_ - 1;
    }
}

void TermBuffer::insert_chars(int n) {
    if (n <= 0 || pos_x >= width_cells_) return;

    n = std::min(n, width_cells_ - pos_x);


    for (auto i = width_cells_ - n - 1; i >= pos_x; --i) { // Move characters to the right from the cursor 
        buffer_[pos_y][i + n] = buffer_[pos_y][i];
    }

    for (auto i = pos_x; i < pos_x + n; ++i) { // Fill from cursor to cursor + n with spaces
        buffer_[pos_y][i].codepoint = ' ';
        buffer_[pos_y][pos_x].flags = 0;
    }
}
void TermBuffer::delete_chars(int n) {
    if (n <= 0 || pos_x < 0) return;
    
    
    for (auto i = pos_x; i < width_cells_ - n - 1; ++i) { // Move characters left after the cursor
        buffer_[pos_y][i] = buffer_[pos_y][i + n];
    }

    for (auto i = width_cells_ - n - 1; i < width_cells_; ++i) { // FIll moved characters in the end with spaces
        buffer_[pos_y][i].codepoint = ' ';
        buffer_[pos_y][pos_x].flags = 0;
    }
}

void TermBuffer::set_selection(int start_x, int start_y, int end_x, int end_y, int scroll_offset) {
    if (start_y <= end_y) {
        mouse_start_cell.first = start_x / font_width_;
        mouse_start_cell.second = start_y / font_height_;

        mouse_end_cell.first = end_x / font_width_;
        mouse_end_cell.second = end_y / font_height_;
    } else if (start_y > end_y) {
        mouse_start_cell.first = end_x / font_width_;
        mouse_start_cell.second = end_y / font_height_;

        mouse_end_cell.first = start_x / font_width_;
        mouse_end_cell.second = start_y / font_height_;
    }

    if (mouse_start_cell.second >= height_cells_) {
        mouse_start_cell.first = -1;
        mouse_start_cell.second = -1;
        mouse_end_cell.first = -1;
        mouse_end_cell.second = -1;
        return;
    }


    mouse_start_cell.second += scroll_offset;
    mouse_end_cell.second += scroll_offset;
    // std::cout << scroll_offset << std::endl;
    mouse_start_cell.second = std::min(mouse_start_cell.second, height_cells_);
    mouse_end_cell.second = std::min(mouse_end_cell.second, height_cells_ );

    // std::cout << mouse_end_cell.second << " " << height_cells_ << std::endl;

    auto i = mouse_start_cell.second;
    do {
        int x_start;
        int x_end;
        if (mouse_start_cell.second == mouse_end_cell.second) {
            // Selection on the same line
            if (mouse_start_cell.first < mouse_end_cell.first) {
                x_start = mouse_start_cell.first;
                x_end = mouse_end_cell.first;
            } else if (x_start > x_end) {
                x_start = mouse_end_cell.first;
                x_end = mouse_start_cell.first;  
            }
            
        } else if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second - 1) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }

        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j < x_end; ++j) {
            // printf("i: %d; j: %d; height: %d; width: %d\n", i, j, height_cells_, width_cells_);
            auto temp = buffer_[i][j].fg_color;
            buffer_[i][j].fg_color = buffer_[i][j].bg_color;
            buffer_[i][j].bg_color = temp;
        }
        ++i;
    } while (i < mouse_end_cell.second);
}

void TermBuffer::remove_selection() {
    if (mouse_start_cell.first == -1 || mouse_start_cell.second == -1 || mouse_end_cell.first == -1 || mouse_end_cell.second == -1) {
        return;
    }

    auto i = mouse_start_cell.second;
    do {
        int x_start;
        int x_end;
        if (mouse_start_cell.second == mouse_end_cell.second) {
            // Selection on the same line
            if (mouse_start_cell.first < mouse_end_cell.first) {
                x_start = mouse_start_cell.first;
                x_end = mouse_end_cell.first;
            } else if (x_start > x_end) {
                // std::cout << "here\n";
                x_start = mouse_end_cell.first;
                x_end = mouse_start_cell.first;  
            }
        } else if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second - 1) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }
        
        x_end = std::min(x_end, width_cells_ - 1);
        for (auto j = x_start; j < x_end; ++j) {
            // printf("i: %d; j: %d\n", i, j);
    
            auto temp = buffer_[i][j].fg_color;
            buffer_[i][j].fg_color = buffer_[i][j].bg_color;
            buffer_[i][j].bg_color = temp;
        }
        ++i;
    } while (i < mouse_end_cell.second);

    mouse_start_cell.first = -1;
    mouse_start_cell.second = -1;
    mouse_end_cell.first = -1;
    mouse_end_cell.second = -1;
}

std::string TermBuffer::get_selected_text() const {
    if (mouse_start_cell.first == -1 || mouse_start_cell.second == -1 || mouse_end_cell.first == -1 || mouse_end_cell.second == -1) {
        return "";
    }
    std::string result;

    auto i = mouse_start_cell.second;
    do {
        int x_start;
        int x_end;
        if (mouse_start_cell.second == mouse_end_cell.second) {
            // Selection on the same line
            if (mouse_start_cell.first < mouse_end_cell.first) {
                x_start = mouse_start_cell.first;
                x_end = mouse_end_cell.first;
            } else if (x_start > x_end) {
                std::cout << "here\n";
                x_start = mouse_end_cell.first;
                x_end = mouse_start_cell.first;  
            }
        } else if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second - 1) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }
        

        for (auto j = x_start; j < x_end; ++j) {
            auto character = buffer_[i][j].codepoint;
            result += std::move(utf8::utf32to8(std::u32string{character}));
        }
        ++i;
    } while (i < mouse_end_cell.second);
    return result;
}


// Resizing
void TermBuffer::resize(std::pair<int, int> new_window_size, std::pair<int, int> font_size) {
    int new_height_cells = new_window_size.second / font_size.second - 1;
    int new_width_cells = new_window_size.first / font_size.first - 1;
    
    if (height_cells_ < new_height_cells) {
        grow_lines(new_height_cells - height_cells_ + 1);
    } else if (height_cells_ > new_height_cells) {
        shrink_lines(height_cells_ - new_height_cells);
    }

    if (width_cells_ < new_width_cells) {
        grow_cols(new_width_cells - width_cells_ + 1, true);
    } else if (width_cells_ > new_width_cells) {
        shrink_cols(width_cells_ - new_width_cells, true);
    }
}

void TermBuffer::grow_lines(int n) {
    expand_down(n);
}
void TermBuffer::shrink_lines(int n) {
    // std::cout << max_pos_y << " " << n << std::endl;
    if (buffer_.end() - n > buffer_.begin() + max_pos_y) {
        buffer_.erase(buffer_.end() - n, buffer_.end());
        height_cells_ -= n;
    }
    
}

void TermBuffer::grow_cols(int n, bool reflow) {
    width_cells_ += n;

    auto should_reflow = [reflow, this](const std::vector<Cell>& row) {
        auto length = row.size();
        return reflow && length > 0 && length < width_cells_ && row[length - 1].is_wrapline();
    };

    std::vector<std::vector<Cell>> reversed;
    reversed.reserve(height_cells_);

    for (auto it = buffer_.rbegin(), end = buffer_.rend(); it != end; ++it) {
        std::vector<Cell> row = std::move(*it);

        if (!reversed.empty() && should_reflow(row)) {
            row.back().set_wrapline(false);

            auto& last_row = reversed.back();

            auto can_take_n = std::min(last_row.size(), width_cells_ - row.size());
            row.insert(row.end(), last_row.begin(), last_row.begin() + can_take_n);
            last_row.erase(last_row.begin(), last_row.begin() + can_take_n);

            if (std::all_of(last_row.begin(), last_row.end(), [](const auto & elem) { return elem.codepoint == 0; })) {
                reversed.pop_back();
            } else {
                row.back().set_wrapline();
            }
        }
        if (row.size() < width_cells_) {
            row.resize(width_cells_);
        }
        reversed.push_back(std::move(row));
    }

    buffer_ = reversed | std::views::reverse | std::ranges::to<std::vector<std::vector<Cell>>>();
    buffer_.resize(height_cells_);
}

void TermBuffer::shrink_cols(int n, bool reflow) {
    width_cells_ -= n;
}
