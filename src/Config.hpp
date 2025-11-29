#include <filesystem>
#include <iostream>
#include <string>
#include <fstream>

struct Config {
    std::string font_path{"/usr/share/fonts/TTF/DejaVuSansMono.ttf"};
    int font_ptsize{16};
    int default_window_width{400};
    int default_window_height{200};

    Config(const std::filesystem::path& path) {
        std::ifstream file{path};
        if (!file.is_open()) {
            std::cerr << "No config file located" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (auto pos = line.find("="); pos != std::string::npos) {
                auto name = std::string{line.substr(0, pos)};
                auto value = std::string{line.substr(pos + 1)};
                if (name == "fontSize") {
                    try {
                        auto ptsize = std::stoi(value);
                        if (ptsize <= 0) continue;
                        font_ptsize = ptsize;
                    } catch (const std::exception& ex) {
                        std::cerr << ex.what() << std::endl;
                    }
                } else if (name == "fontPath") {
                    if (!std::filesystem::exists(value)) {
                        std::cerr << "Such font doesn't exist" << std::endl;
                        continue;
                    }
                    font_path = std::move(value);
                } else if (name == "defaultWindowWidth") {
                    try {
                        auto width = std::stoi(value);
                        if (width <= 0) continue;
                        default_window_width = width;
                    } catch (const std::exception& ex) {
                        std::cerr << ex.what() << std::endl;
                    }
                } else if (name == "defaultWindowHeight") {
                    try {
                        auto height = std::stoi(value);
                        if (height <= 0) continue;
                        default_window_height = height;
                    } catch (const std::exception& ex) {
                        std::cerr << ex.what() << std::endl;
                    }
                }
            } else {
                continue;
            }
        }
    }
};
