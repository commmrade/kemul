#include "Buffer.hpp"
#include <algorithm>
#include <iterator>
#include <utf8cpp/utf8/cpp17.h>
#include <utility>
#include <vector>
#include <utf8cpp/utf8.h>
#include <iostream>
#include <ranges>



TermBuffer::TermBuffer(int width, int height, int cell_width, int cell_height) : cell_size_({cell_width, cell_height}) {
    width_cells_ = width / cell_width - 1;
    height_cells_ = height / cell_height;
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
    cursor_x_ = 0;
    cursor_y_ = 0;
    max_pos_y_ = 0;
}

void TermBuffer::set_cursor_position(int row, int col) {
    cursor_x_ = std::max(0, std::min(col - 1, width_cells_ - 1));
    if (row - 1 >= buffer_.size()) {
        expand_down(row - buffer_.size());
    }
    cursor_y_ = std::max(0, row - 1);
}
void TermBuffer::move_cursor_pos_relative(int d_row, int d_col) {
    int new_y = cursor_y_ + d_row;
    new_y = std::max(0, std::min(new_y, (int)buffer_.size() - 1));
    cursor_y_ = new_y;

    int new_x = cursor_x_ + d_col;
    new_x = std::max(0, std::min(new_x, width_cells_ - 1));
    cursor_x_ = new_x;
}


void TermBuffer::reset_cursor(bool x_dir, bool y_dir) {
    if (x_dir) {
        cursor_x_ = 0;
    }
    if (y_dir) {
        cursor_y_ = 0;
    }
}


void TermBuffer::add_cells(std::vector<Cell>&& cells) {
    for (auto&& cell : cells) {
        if (cell.codepoint == 0x0A) { // If newline
            cursor_down();
            reset_cursor(true, false);
            continue;
        }
        buffer_[cursor_y_][cursor_x_] = std::move(cell);
        cursor_x_ += 1;
        if (cursor_x_ >= width_cells_) {
            // Set wrapline flag for the last symbol
            buffer_[cursor_y_][cursor_x_ - 1].set_wrapline();
            cursor_down();
            cursor_x_ = 0;
        }
    }
}


void TermBuffer::cursor_down() {
    if (++cursor_y_ == buffer_.size()) {
        expand_down();
    }
    max_pos_y_ = std::max(cursor_y_, max_pos_y_);
}

void TermBuffer::cursor_up(int n) {
    if ((cursor_y_ -= n) < 0) {
        cursor_y_ = 0;
    }
}

void TermBuffer::expand_down(int n) {
    for (auto i = 0; i < n; i++) {
        buffer_.emplace_back(width_cells_);
    }
    height_cells_ += n;
}


void TermBuffer::erase_in_line(int mode) {
    if (cursor_y_ >= buffer_.size()) return;

    int start = 0;
    int end = width_cells_;

    if (mode == 0) { // fron cursor to the ened
        start = cursor_x_;
    } else if (mode == 1) { // from start to cursor
        end = cursor_x_ + 1;
        std::cout << "heh\n";
    } else if (mode == 2) { // whole line
        start = 0;
        end = width_cells_;
    }

    for (int x = start; x < end; ++x) {
        buffer_[cursor_y_][x] = Cell{};
    }
}

void TermBuffer::erase_last_symbol() { 
    buffer_[cursor_y_][cursor_x_] = Cell{};
    cursor_x_--;
    if (cursor_x_ < 0) { // Reflow cursor to the prev line
        cursor_y_--;
        cursor_x_ = width_cells_ - 1;
    }
}

void TermBuffer::insert_chars(int n) {
    if (n <= 0 || cursor_x_ >= width_cells_) return;
    n = std::min(n, width_cells_ - cursor_x_);

    for (auto i = width_cells_ - n - 1; i >= cursor_x_; --i) { // Move characters to the right from the cursor 
        buffer_[cursor_y_][i + n] = buffer_[cursor_y_][i];
    }

    for (auto i = cursor_x_; i < cursor_x_ + n; ++i) { // Fill from cursor to cursor + n with spaces
        buffer_[cursor_y_][i].codepoint = 0;
        buffer_[cursor_y_][cursor_x_].flags = 0;
    }
}
void TermBuffer::delete_chars(int n) {
    if (n <= 0 || cursor_x_ < 0) return;
    n = std::min(n, width_cells_ - cursor_x_); // Prevent out of bounds
    
    for (auto i = cursor_x_; i < width_cells_ - n - 1; ++i) { // Move characters left after the cursor
        buffer_[cursor_y_][i] = buffer_[cursor_y_][i + n];
    }

    for (auto i = width_cells_ - n - 1; i < width_cells_; ++i) { // FIll moved characters in the end with spaces
        buffer_[cursor_y_][i].codepoint = 0;
        buffer_[cursor_y_][cursor_x_].flags = 0;
    }
}

void TermBuffer::set_selection(int start_x, int start_y, int end_x, int end_y, int scroll_offset) {
    if (start_y <= end_y) {
        mouse_start_cell.first = start_x / cell_size_.first;
        mouse_start_cell.second = start_y / cell_size_.second;

        mouse_end_cell.first = end_x / cell_size_.first;
        mouse_end_cell.second = end_y / cell_size_.second;
    } else if (start_y > end_y) {
        mouse_start_cell.first = end_x / cell_size_.first;
        mouse_start_cell.second = end_y / cell_size_.second;

        mouse_end_cell.first = start_x / cell_size_.first;
        mouse_end_cell.second = start_y / cell_size_.second;
    }

    if (mouse_start_cell.second >= height_cells_) {
        mouse_start_cell.first = -1; mouse_start_cell.second = -1;
        mouse_end_cell.first = -1; mouse_end_cell.second = -1;
        return;
    }

    // Counting offset
    mouse_start_cell.second += scroll_offset; 
    mouse_end_cell.second += scroll_offset;
    
    // No need to check start_cell.y because if (mouse_start_cell.second >= height_cells_) It cant be more than height_cells_ - 1
    mouse_end_cell.second = std::min(mouse_end_cell.second, height_cells_ - 1);

    if (mouse_start_cell.second == mouse_end_cell.second) {
        // Selection on the same line
        int x_start;
        int x_end;
        if (mouse_start_cell.first <= mouse_end_cell.first) {
            x_start = mouse_start_cell.first;
            x_end = mouse_end_cell.first;
        } else if (mouse_start_cell.first > mouse_end_cell.first) {
            x_start = mouse_end_cell.first;
            x_end = mouse_start_cell.first;  
        }
        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            std::swap(buffer_[mouse_start_cell.second][j].fg_color, buffer_[mouse_start_cell.second][j].bg_color);
        }
        return;
    } 

    auto i = mouse_start_cell.second; 
    do { // Iterating from start_cell.y up until end_cell.y

        int x_start;
        int x_end;
        
        if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }

        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            std::swap(buffer_[i][j].fg_color, buffer_[i][j].bg_color);
        }
        ++i;
    } while (i <= mouse_end_cell.second);
}

void TermBuffer::remove_selection() {
    if (mouse_start_cell.first == -1 || mouse_start_cell.second == -1 || mouse_end_cell.first == -1 || mouse_end_cell.second == -1) {
        return;
    }

    mouse_end_cell.second = std::min(mouse_end_cell.second, height_cells_ - 1);

    if (mouse_start_cell.second == mouse_end_cell.second) {
        // Selection on the same line
        int x_start;
        int x_end;
        if (mouse_start_cell.first <= mouse_end_cell.first) {
            x_start = mouse_start_cell.first;
            x_end = mouse_end_cell.first;
        } else if (mouse_start_cell.first > mouse_end_cell.first) {
            x_start = mouse_end_cell.first;
            x_end = mouse_start_cell.first;  
        }
        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            std::swap(buffer_[mouse_start_cell.second][j].fg_color, buffer_[mouse_start_cell.second][j].bg_color);
        }
        mouse_start_cell.first = -1;
        mouse_start_cell.second = -1;
        mouse_end_cell.first = -1;
        mouse_end_cell.second = -1;
        return;
    } 

    auto i = mouse_start_cell.second; 
    do { // Iterating from start_cell.y up until end_cell.y

        int x_start;
        int x_end;
        
        if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }

        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            std::swap(buffer_[i][j].fg_color, buffer_[i][j].bg_color);
        }
        ++i;
    } while (i <= mouse_end_cell.second);

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

    if (mouse_start_cell.second == mouse_end_cell.second) {
        // Selection on the same line
        int x_start;
        int x_end;
        if (mouse_start_cell.first <= mouse_end_cell.first) {
            x_start = mouse_start_cell.first;
            x_end = mouse_end_cell.first;
        } else if (mouse_start_cell.first > mouse_end_cell.first) {
            x_start = mouse_end_cell.first;
            x_end = mouse_start_cell.first;  
        }
        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            auto character = buffer_[mouse_start_cell.second][j].codepoint;
            result += std::move(utf8::utf32to8(std::u32string{character}));
        }
        return result;
    } 

    auto i = mouse_start_cell.second; 
    do { // Iterating from start_cell.y up until end_cell.y

        int x_start;
        int x_end;
        
        if (i == mouse_start_cell.second) {
            x_start = mouse_start_cell.first;
            x_end = width_cells_;
        } else if (i == mouse_end_cell.second) {
            x_start = 0;
            x_end = mouse_end_cell.first;
        } else {
            x_start = 0;
            x_end = width_cells_;
        }

        x_end = std::min(x_end, width_cells_ - 1);

        for (auto j = x_start; j <= x_end; ++j) {
            auto character = buffer_[i][j].codepoint;
            result += std::move(utf8::utf32to8(std::u32string{character}));
        }
        ++i;
    } while (i <= mouse_end_cell.second);
    return result;
}


// Resizing
void TermBuffer::resize(std::pair<int, int> new_window_size, std::pair<int, int> font_size) {
    remove_selection(); // Check if something is selected already inside remove func

    cell_size_.first = font_size.first;
    cell_size_.second = font_size.second;

    int new_height_cells = new_window_size.second / cell_size_.second - 1;
    int new_width_cells = new_window_size.first / cell_size_.first;
    
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
    if (buffer_.end() - n > buffer_.begin() + max_pos_y_) {
        buffer_.erase(buffer_.end() - n, buffer_.end()); // Erase only empty lines. Not empty lines are tracked by max_pos_y
        height_cells_ -= n;
    }
}

void TermBuffer::grow_cols(int n, bool reflow) {
    auto old_width = width_cells_;
    width_cells_ += n;

    auto should_reflow = [reflow, this](const std::vector<Cell>& row) {
        auto length = row.size();
        return reflow && !row.empty() && length < width_cells_ && row.back().is_wrapline();
    }; // Check if you should reflow it at all, if there is a wrapline flag at the end of the row, it means it was carried-over so we should 'undo' it

    std::vector<std::vector<Cell>> reversed; // Buffer which tracks prev row
    reversed.reserve(height_cells_);

    int cursor_delta{0};

    for (auto it = buffer_.rbegin(), end = buffer_.rend(); it != end; ++it) {
        std::vector<Cell> row = *it; // Tracks cur row

        if (!reversed.empty() && should_reflow(row)) {
            row.back().set_wrapline(false); // If we gonna reflow it, we should remove wrapline flag

            auto& last_row = reversed.back();

            int can_take_n = std::min((int)last_row.size(), width_cells_ - (int)row.size()); // So we can at most take last_row.size() elements so if width - row.size is a too much
            // choose last row size, on the other hand, width - row.size means how many cells we can reflow at all, if last row size is too big, choose this

            // "Spliting front off" last_row
            row.insert(row.end(), last_row.begin(), last_row.begin() + can_take_n);
            for (auto i = 0; i < can_take_n; ++i) { // Temporary solution for erase giving a segfault
                last_row[i].clear();
            }
            // last_row.erase(last_row.begin(), last_row.begin() + can_take_n); // Causes segfault
            if (last_row.empty() || std::all_of(last_row.begin(), last_row.end(), [](const auto & elem) { return elem.codepoint == 0; })) {
                reversed.pop_back(); // We don't need empty line, so just pop it
                ++cursor_delta;
                --max_pos_y_; // So scrolling  isnt fucked
            } else {
                row.back().set_wrapline(); // If there are cells left, line is still wrapped, so set this flag again
            }
        }
        row.resize(width_cells_); // Make sure its correct size
        reversed.push_back(std::move(row));
    }
    // std::cout << "here" << reversed.size() << std::endl;

    buffer_.clear();
    buffer_.assign(reversed.rbegin(), reversed.rend()); // Reversing reversed buffer
    if (buffer_.size() < height_cells_) {
        expand_down(height_cells_ - buffer_.size());
    }
    cursor_up(cursor_delta);
}

void TermBuffer::shrink_cols(int n, bool reflow) {
    if (n >= width_cells_) {
        width_cells_ = 1;
    } else {
        width_cells_ -= n;
    }

    auto row_length = [](const std::vector<Cell>& row) {
        return std::count_if(row.begin(), row.end(), [](const Cell& cell) {
            return cell.codepoint != 0;
        }); // Real length of the row (kinda works so let it be)
    };

    std::vector<std::vector<Cell>> new_buffer;
    std::vector<Cell> carry;

    for (auto it = buffer_.begin(), end = buffer_.end(); it != end; ++it) {
        auto row = *it;

        if (!carry.empty()) { // If there is some carry-over
            if (carry.back().is_wrapline()) {  // And there was wrapline on the last symbol
                row.insert(row.begin(), carry.begin(), carry.end()); // Insert the carry-over into the current line
            } else { // If no wrapline
                carry.resize(width_cells_);
                new_buffer.push_back(std::move(carry)); // Push onto a new line
                cursor_down(); // Make sure it doesnt fuck things up
                ++max_pos_y_; // So scrolling  isnt fucked
            }
            carry.clear(); // Clear carry-over
        }

        auto row_length_real = row_length(row); // Real length which counts only codepoints != 0
        if (row_length_real > width_cells_) {
            carry.assign(row.begin() + width_cells_, row.begin() + row_length_real); // Taking overend cells

            if (row[width_cells_ - 1].codepoint != 0) {
                row[width_cells_ - 1].set_wrapline(); // Setting wrapline for the last Cell, and that cell is something because row_length-real > width_cells
            }
        }
        row.resize(width_cells_);
        new_buffer.push_back(std::move(row));
        // cursor_down();
    }

    while (!carry.empty()) { // If there is carry left append it
        std::vector<Cell> new_row;
        if (carry.size() > width_cells_) {
            new_row.assign(carry.begin(), carry.begin() + width_cells_);
            carry.erase(carry.begin(), carry.begin() + width_cells_);
            if (!new_row.empty() && new_row.back().codepoint != 0) {
                new_row.back().set_wrapline();
            }
        } else {
            new_row = std::move(carry);
            carry.clear();
        }
        new_row.resize(width_cells_);
        new_buffer.push_back(std::move(new_row));
    }

    buffer_ = std::move(new_buffer);
    if (buffer_.size() < height_cells_) {
        expand_down(height_cells_ - buffer_.size());
    }
}
