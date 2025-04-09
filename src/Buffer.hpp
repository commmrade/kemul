#pragma once 
#include "SDL_stdinc.h"
#include <string>
#include <vector>



class TermBuffer {
private:
    std::vector<std::string> buffer;
    std::string command;

public:
    explicit TermBuffer();
    ~TermBuffer();

    void push_str(std::string str);
    void add_str(std::string str);

    void add_str_command(const char* sym) {
        command.append(sym, SDL_strlen(sym));
    }

    void clear_command() {
        command.clear();
        command = "";
    }

    const std::vector<std::string>& get_buffer() const {
        return buffer;
    }
    const std::string& get_command() const {
        return command;
    }
};