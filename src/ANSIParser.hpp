#include "Application.hpp"
#include "Buffer.hpp"
#include <iostream>
#include <ostream>
#include <utf8cpp/utf8.h>
#include <sstream>

class AnsiParser {
    private:
        Application& application;      // Reference to the Application instance
        Cell current_cell;            // Current state of cell attributes
        std::vector<Cell> cells;      // Buffer of parsed cells
    
        // State machine states
        enum class GeneralState { TEXT, ESCAPE, CSI };
        GeneralState state = GeneralState::TEXT;
    
    public:
        AnsiParser(Application& app) : application(app) {
            // Initialize default cell attributes
            current_cell.fg_color = {255, 255, 255, 255}; // White
            current_cell.bg_color = {0, 0, 0, 255};       // Black
            current_cell.flags = 0;
        }
    
        void parse(const std::string& text) {
            std::string::const_iterator it = text.begin();
            while (it != text.end()) {
                if (state == GeneralState::TEXT) {
                    try {
                        uint32_t codepoint = utf8::next(it, text.end());
                        if (codepoint == 0x1B) { // ESC character
                            state = GeneralState::ESCAPE;
                        } else {
                            // Create a cell with current attributes and add it to the buffer
                            Cell cell = current_cell;
                            cell.codepoint = codepoint;
                            cells.push_back(cell);
                        }
                    } catch (utf8::invalid_utf8&) {
                        ++it; // Skip invalid UTF-8 bytes
                    }
                } else if (state == GeneralState::ESCAPE) {
                    if (it != text.end()) {
                        char c = *it++;
                        if (c == '[') {
                            state = GeneralState::CSI;
                            std::string csi_sequence;
                            // Collect CSI sequence until a letter is found
                            while (it != text.end() && !std::isalpha(*it)) {
                                csi_sequence += *it++;
                            }
                            if (it != text.end()) {
                                char command = *it++;
                                handleCSI(command, parseParams(csi_sequence));
                            }
                            state = GeneralState::TEXT;
                        } else {
                            state = GeneralState::TEXT; // Unknown escape, treat next as text
                        }
                    }
                }
            }
            // Send collected cells to Application after parsing
            if (!cells.empty()) {
                application.on_set_cells(cells);
                cells.clear();
            }
        }
    
    private:
        // Parse CSI parameters (e.g., "1;31" -> {1, 31})
        std::vector<int> parseParams(const std::string& csi_sequence) {
            std::vector<int> params;
            std::istringstream iss(csi_sequence);
            std::string param;
            while (std::getline(iss, param, ';')) {
                // std::cout << param << std::endl;
                if (param.empty()) {
                    params.push_back(0);
                } else if (param[0] == '?') {
                    // Handle private mode parameters (e.g., "?2004")
                    try {
                        params.push_back(std::stoi(param.substr(1))); // Store number after '?'
                        params.push_back(-1); // Indicator for private mode
                    } catch (const std::invalid_argument&) {
                        // Invalid private mode parameter, skip or handle
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
    
        // Handle CSI commands (e.g., 'm' for SGR, 'H' for cursor position)
        void handleCSI(char command, const std::vector<int>& params) {
            static const SDL_Color color_map[8] = {
                {0, 0, 0, 255},       // Black
                {255, 0, 0, 255},     // Red
                {0, 255, 0, 255},     // Green
                {255, 255, 0, 255},   // Yellow
                {0, 0, 255, 255},     // Blue
                {255, 0, 255, 255},   // Magenta
                {0, 255, 255, 255},   // Cyan
                {255, 255, 255, 255}  // White
            };
    
            if (command == 'm') { // Select Graphic Rendition (SGR)
                for (int param : params) {
                    if (param == 0) { // Reset
                        current_cell.fg_color = {255, 255, 255, 255};
                        current_cell.bg_color = {0, 0, 0, 255};
                        current_cell.flags = 0;
                    } else if (param == 1) {
                        current_cell.flags |= 1; // Bold (assuming flag bit 0)
                    } else if (param == 4) {
                        current_cell.flags |= 2; // Underline (assuming flag bit 1)
                    } else if (30 <= param && param <= 37) {
                        current_cell.fg_color = color_map[param - 30];
                    } else if (40 <= param && param <= 47) {
                        current_cell.bg_color = color_map[param - 40];
                    }
                }
            } else if (command == 'H') { // Cursor position
                int row = params.size() >= 1 ? params[0] : 1;
                int col = params.size() >= 2 ? params[1] : 1;
                application.on_move_cursor(row, col);
            } else if (command == 'J' && params.size() >= 1 && params[0] == 2) {
                std::cout << "clear screen\n";
                // application.clear_screen(); // Clear screen (mode 2)
            } else if (command == 'A') { // Cursor up
                int n = params.size() >= 1 ? params[0] : 1;
                // application.move_cursor_relative(-n, 0);
            } else if (command == 'B') { // Cursor down
                int n = params.size() >= 1 ? params[0] : 1;
                // application.move_cursor_relative(n, 0);
            } else if (command == 'C') { // Cursor forward
                int n = params.size() >= 1 ? params[0] : 1;
                // application.move_cursor_relative(0, n);
            } else if (command == 'D') { // Cursor backward
                int n = params.size() >= 1 ? params[0] : 1;
                // application.move_cursor_relative(0, -n);
            }
        }
    };