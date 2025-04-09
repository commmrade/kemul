#pragma once
#include <SDL_keycode.h>
#include <cstdint>
#include <memory>
#include <sys/poll.h>

#include "Window.hpp"
#include "Buffer.hpp"


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


    // Parser events
private:
    void init_sdl();
    void init_ttf();

    void setup_pty();

    void loop();
};