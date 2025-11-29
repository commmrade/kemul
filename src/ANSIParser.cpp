#include "ANSIParser.hpp"
#include "Application.hpp"
#include <utf8cpp/utf8.h>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <iostream>

AnsiParser::AnsiParser(Application& app) : application(app) {
    // Initialize default cell attributes
    current_cell.fg_color = {200, 200, 200, 255}; // White
    current_cell.bg_color = {0, 0, 0, 255};       // Black
    current_cell.flags = 0;
}


void AnsiParser::parse(const std::string& text) {
    std::string::const_iterator it = text.cbegin();
    while (it != text.end()) {
        if (state == GeneralState::TEXT) {
            try {
                uint32_t codepoint = utf8::next(it, text.end());
                if (codepoint == 0x1B) { // ESC character
                    state = GeneralState::ESCAPE;
                } else if (codepoint == 0x0D) { // Carriage Return ('\r', codepoint 13)
                    application.on_reset_cursor(true, false);
                } else if (codepoint == 0x09) {
                    Cell tabul = current_cell;
                    tabul.codepoint = ' ';
                    std::vector<Cell> tabs{tabul, tabul, tabul, tabul};
                    application.on_add_cells(std::move(tabs)); // Inserting four spaces
                } else if (codepoint == 0x08) { // Appears after you send DEL codepoint to the shell so it deletes it.
                    // application.on_erase_event();
                    application.on_move_cursor(0, -1);
                } else if (codepoint == 0x07) { // TODO: Play bell sound

                } else {
                    // Create a cell with current attribs and add it to the buffer
                    Cell cell = current_cell;
                    cell.codepoint = codepoint;
                    application.on_add_cells({cell});
                }
            } catch (utf8::invalid_utf8&) {
                ++it;
            }
        } else if (state == GeneralState::ESCAPE) {
            if (it != text.end()) {
                char c = *it++;
                if (c == '[') {
                    state = GeneralState::CSI;
                    std::string csi_sequence;
                    // Collect CSI sequence until a letter is found
                    while (it != text.end() && !std::isalpha(*it) && *it != '@') {
                        csi_sequence += *it++;
                    }
                    if (it != text.end()) {
                        char command = *it++;
                        handle_CSI(command, parse_params(csi_sequence));
                    }
                    state = GeneralState::TEXT;
                } else if (c == ']') { // Handle OSC sequences
                    std::string osc_sequence;
                    while (it != text.end() && *it != '\a' && *it != 0x1B) { // OSC ends with BEL (\a) or ESC
                        osc_sequence += *it++;
                    }

                    application.on_change_window_title(osc_sequence);
                    if (it != text.end()) {
                        if (*it == '\a') {
                            ++it; // Consume /a
                        } else if (*it == 0x1B && it + 1 != text.end() && *(it + 1) == '\\') {
                            it += 2; /* Consume ESC and \ */
                        }
                    }
                    state = GeneralState::TEXT;
                } else {
                    state = GeneralState::TEXT; // Unknown escape, treat next as text
                }
            }
        }
    }
}

std::vector<int> AnsiParser::parse_params(const std::string& csi_sequence) {
    std::vector<int> params;
    std::istringstream iss(csi_sequence);
    std::string param;
    while (std::getline(iss, param, ';')) {
        if (param.empty()) {
            params.push_back(0);
        } else if (param[0] == '?') {
            // Handle private mode parameters
            try {
                params.push_back(std::stoi(param.substr(1))); // Store number after '?'
                params.push_back(-1); // Indicator for private mode
            } catch (const std::invalid_argument&) {
                // Skip for now
            }
        } else {
            try {
                params.push_back(std::stoi(param));
            } catch (const std::invalid_argument&) {
                params.push_back(0); // Fallback for invalid numbers
            }
        }
    }
    return params;
}


void AnsiParser::handle_CSI(char command, std::vector<int> params) {
    static const SDL_Color color_map[8] = {
        {0, 0, 0, 255},       // Black
        {255, 0, 0, 255},     // Red
        {0, 255, 0, 255},     // Green
        {255, 255, 0, 255},   // Yellow
        {0, 0, 255, 255},     // Blue
        {255, 0, 255, 255},   // Magenta
        {0, 255, 255, 255},   // Cyan
        {255, 255, 255, 255}  // White
    }; // TODO: add more colors

    if (command == 'm') { // Select Graphic Rendition (SGR)
        if (params.empty()) params.push_back(0);
        for (int param : params) {
            if (param == 0) { // Reset
                current_cell.fg_color = {200, 200, 200, 255};
                current_cell.bg_color = {0, 0, 0, 255};
                current_cell.flags = 0;
            } else if (param == 1) {
                current_cell.set_bold();
            } else if (param == 22) {
                current_cell.set_bold(false);
            } else if (param == 7) {
                auto temp_color = current_cell.fg_color;
                current_cell.fg_color = current_cell.bg_color;
                current_cell.bg_color = temp_color;
            } else if (param == 27) {
                auto temp_color = current_cell.fg_color;
                current_cell.fg_color = current_cell.bg_color;
                current_cell.bg_color = temp_color;
            } else if (param == 4) {
                current_cell.set_underline();
            } else if (param == 24) {
                current_cell.set_underline(false);
            } else if (param == 9) {
                current_cell.set_strikethrough();
            } else if (param == 29) {
                current_cell.set_strikethrough(false);
            } else if (30 <= param && param <= 37) {
                current_cell.fg_color = color_map[param - 30];
            } else if (40 <= param && param <= 48) {
                current_cell.bg_color = color_map[param - 40];
            }
        }
    } else if (command == 'H') { // Cursor position
        int row = params.size() >= 1 ? params[0] : 1;
        int col = params.size() >= 2 ? params[1] : 1;
        application.on_set_cursor(row, col);
    } else if (command == 'J' && params.size() >= 1) {
        application.on_clear_requested(params[0] == 3); // Clear screen
    } else if (command == 'A') { // Cursor up
        int n = params.size() >= 1 ? params[0] : 1;
        application.on_move_cursor(-n, 0);
    } else if (command == 'B') { // Cursor down
        int n = params.size() >= 1 ? params[0] : 1;
        application.on_move_cursor(n, 0);
    } else if (command == 'C') { // Cursor forward
        // std::cout << "forward\n";
        int n = params.size() >= 1 ? params[0] : 1;
        application.on_move_cursor(0, n);
    } else if (command == 'D') { // Cursor backward
        int n = params.size() >= 1 ? params[0] : 1;
        application.on_move_cursor(0, -n);
    } else if (command == 'K') {
        int mode = params.empty() ? 0 : params[0];
        application.on_erase_in_line(mode);
    } else if (command == '@') { // ANSI to insert characters and shift existing right
        int n = params.empty() ? 0 : params[0];
        // std::cout << "inserted" << std::endl;
        application.on_insert_chars(n);
    } else if (command == 'P') {
        int n = params.empty() ? 0 : params[0];
        application.on_delete_chars(n);
    }
    // std::cout << command << std::endl;
}
