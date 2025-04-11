// #include "ANSIParser.hpp"
// #include <utf8cpp/utf8.h>
// #include <vector>
// #include <string>
// #include <sstream>
// #include <cctype>

// void AnsiParser::parse(const std::string &text) {

    

//     std::string::const_iterator it = text.begin();
//     while (it != text.end()) {
//         if (general_state == GeneralState::TEXT) {
//             try {
//                 uint32_t codepoint = utf8::next(it, text.end());
//                 if (codepoint == 0x1B) { // ESC character
//                     general_state = GeneralState::ESCAPE;
//                 } else {
//                     application.add_cell(codepoint);
//                 }
//             } catch (utf8::invalid_utf8&) {
//                 // Skip invalid UTF-8 bytes
//                 ++it;
//             }
//         } else if (general_state == GeneralState::ESCAPE) {
//             if (it != text.end()) {
//                 char c = *it;
//                 ++it;
//                 if (c == '[') {
//                     general_state = GeneralState::CSI;
//                     std::string csi_sequence;
//                     // Collect CSI sequence until a letter is encountered
//                     while (it != text.end() && !std::isalpha(*it)) {
//                         csi_sequence += *it;
//                         ++it;
//                     }
//                     if (it != text.end()) {
//                         char command = *it;
//                         ++it;
//                         // Parse parameters
//                         std::vector<int> params;
//                         std::istringstream iss(csi_sequence);
//                         std::string param;
//                         while (std::getline(iss, param, ';')) {
//                             if (!param.empty()) {
//                                 params.push_back(std::stoi(param));
//                             } else {
//                                 params.push_back(0);
//                             }
//                         }
//                         handleCSI(command, params);
//                     }
//                     general_state = GeneralState::TEXT;
//                 } else {
//                     // Non-CSI escape sequence, treat next byte as text
//                     general_state = GeneralState::TEXT;
//                     // 'c' will be processed in the next iteration
//                 }
//             } else {
//                 general_state = GeneralState::TEXT;
//             }
//         }
//     }
// }

// // Private helper method to handle CSI commands
// void AnsiParser::handleCSI(char command, const std::vector<int>& params) {
//     static const SDL_Color default_fg = {255, 255, 255, 255}; // White
//     static const SDL_Color default_bg = {0, 0, 0, 255};       // Black
//     static const SDL_Color color_map[8] = {
//         {0, 0, 0, 255},       // Black
//         {255, 0, 0, 255},     // Red
//         {0, 255, 0, 255},     // Green
//         {255, 255, 0, 255},   // Yellow
//         {0, 0, 255, 255},     // Blue
//         {255, 0, 255, 255},   // Magenta
//         {0, 255, 255, 255},   // Cyan
//         {255, 255, 255, 255}  // White
//     };

//     if (command == 'm') {
//         // SGR - Select Graphic Rendition
//         // for (int param : params) {
//         //     if (param == 0) {
//         //         // Reset attributes
//         //         application.setForegroundColor(default_fg);
//         //         application.setBackgroundColor(default_bg);
//         //         application.setBold(false);
//         //         application.setUnderline(false);
//         //     } else if (param == 1) {
//         //         application.setBold(true);
//         //     } else if (param == 4) {
//         //         application.setUnderline(true);
//         //     } else if (30 <= param && param <= 37) {
//         //         // Foreground colors
//         //         application.setForegroundColor(color_map[param - 30]);
//         //     } else if (40 <= param && param <= 47) {
//         //         // Background colors
//         //         application.setBackgroundColor(color_map[param - 40]);
//         //     }
//             // Additional SGR codes can be added here
//         // }
//     } else if (command == 'H') {
//         // Move cursor to position (row;col)
//         int row = params.size() >= 1 ? params[0] : 1;
//         int col = params.size() >= 2 ? params[1] : 1;
//         application.move_cursor(row, col);
//     }
//     // } else if (command == 'J') {
//     //     // Clear screen
//     //     // int mode = params.size() >= 1 ? params[0] : 0;
//     //     // if (mode == 2) {
//     //     //     application.clearScreen();
//     //     // }
//     //     // Additional clear modes can be added here
//     // } else if (command == 'A') {
//     //     // Cursor up
//     //     // int n = params.size() >= 1 ? params[0] : 1;
//     //     // application.moveCursorRelative(-n, 0);
//     // } else if (command == 'B') {
//     //     // Cursor down
//     //     int n = params.size() >= 1 ? params[0] : 1;
//     //     application.moveCursorRelative(n, 0);
//     // } else if (command == 'C') {
//     //     // Cursor forward
//     //     int n = params.size() >= 1 ? params[0] : 1;
//     //     application.moveCursorRelative(0, n);
//     // } else if (command == 'D') {
//     //     // Cursor backward
//     //     int n = params.size() >= 1 ? params[0] : 1;
//     //     application.moveCursorRelative(0, -n);
//     // }
//     // Additional CSI commands can be added here
// }