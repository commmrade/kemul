#pragma once
#include <SDL2/SDL_keycode.h>
#include <cstdint>
#include <memory>
#include <sys/poll.h>
#include <vector>
#include "Window.hpp"
#include "Buffer.hpp"

class AnsiParser;
class EventHandler;
class Application {
private:
    // Pty stuff
    int master_fd_;
    int slave_fd_;
    pollfd fds_[1];

    // Settings stuff
    bool echo_enabled_{false};
    bool blocking_enabled_{false};

    bool is_running_{true};


    // Members
    std::unique_ptr<Window> window_;
    std::unique_ptr<TermBuffer> buffer_;
    std::unique_ptr<EventHandler> event_handler_;
    std::unique_ptr<AnsiParser> parser_;
public:
    explicit Application(const std::string &font_path);
    ~Application();


    void set_echo_mode(bool enabled);
    void set_blocking_mode(bool enabled);

    void run();


    // Events
    void on_quit_event();
    void on_textinput_event(const char* sym);
    void on_enter_pressed_event();
    void on_keydown_event(SDL_Keycode key);
    void on_scroll_event(Sint32 scroll_dir);
    void on_paste_event(std::string content);

    void on_set_cells(std::vector<Cell> cells);
    void on_add_cells(std::vector<Cell> cells);
    void on_move_cursor(int row, int col);
    void on_reset_cursor(bool x_dir, bool y_dir);
    void on_clear_requested();

    // Parser events
private:
    void init_sdl();
    void init_ttf();

    void setup_pty();

    void loop();
};
