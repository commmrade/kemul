#pragma once
#include <SDL2/SDL_keycode.h>
#include <SDL_keyboard.h>
#include <SDL_stdinc.h>
#include <memory>
#include <string>
#include <sys/poll.h>
#include <vector>
#include "Window.hpp"
#include "Buffer.hpp"
#include "Config.hpp"


class AnsiParser;
class EventHandler;
class Application {
private:
    // Pty stuff
    int master_fd_;
    int slave_fd_;
    pollfd fds_[1];

    // Settings stuff
    // bool echo_enabled_{false};
    // bool blocking_enabled_{false};

    bool is_running_{true};

    // Members
    std::unique_ptr<Window> window_;
    std::unique_ptr<TermBuffer> buffer_;
    std::unique_ptr<EventHandler> event_handler_;
    std::unique_ptr<AnsiParser> parser_;

    // Config stuff
    Config config_;

    // Mouse selection stuff
    int mouse_x{-1}, mouse_y{-1};
    int mouse_start_x{-1};
    int mouse_start_y{-1};
    int mouse_end_x{-1};
    int mouse_end_y{-1};
public:
    explicit Application(const std::string &font_path);
    ~Application();
    
    void run();

    // Keypress events and others (like window resize)
    void on_textinput_event(const SDL_Event& event);
    void on_keys_pressed(const SDL_Event& event);
    void on_scroll_event(const SDL_Event& event);
    void on_quit_event(const SDL_Event& event);

    void on_selection(const SDL_Event& event);
    void on_remove_selection(const SDL_Event& event);
    void reset_selection(const SDL_Event& event);

    void window_event(const SDL_Event& event);

    void erase_character();
    void send_newline();
    void paste_text(const char* text);
    void on_arrowkey_pressed(SDL_Keycode sym);
    void on_backspace();
    void send_sigint();
    void send_sigsusp();
    void send_eof();
    void clear_text();
    void reverse_find();
    void cursor_to_back(); // CUrsor to the back
    void cursor_to_end(); // Cursor to the end
    void on_window_resized();
    
    
    void copy_selected_text();

    // Parser events
    void on_erase_event();
    
    void on_add_cells(std::vector<Cell>&& cells);
    void on_set_cursor(int row, int col);
    void on_move_cursor(int row, int col);
    void on_reset_cursor(bool x_dir, bool y_dir);
    void on_erase_in_line(int mode);
    void on_clear_requested(bool remove);
    void on_change_window_title(const std::string& win_title);
    void on_insert_chars(int n);
    void on_delete_chars(int n);

private:
    void init_sdl();
    void init_ttf();
    void setup_pty(bool echo, int cols);
    void loop();
    void load_config();
    void set_blocking_mode(bool enabled);
};
