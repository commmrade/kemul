#include "Application.hpp"
#include "Buffer.hpp"
#include "EventHandler.hpp"
#include "Window.hpp"
#include <SDL_clipboard.h>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <print>
#include <pty.h>
#include <stdexcept>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "ANSIParser.hpp"


Application::Application(const std::string &font_path) {
    init_sdl();
    init_ttf();
    auto appdata_dir = std::filesystem::path(std::getenv("HOME")) / ".local/share/kemul";
    auto config_path = appdata_dir / "config.cock";
    auto config_ = Config{config_path};

    window_ = std::make_unique<Window>(config_.font_path, config_.font_ptsize, config_.default_window_width, config_.default_window_height); // Setting up window before so we can get font size
    auto font_size = window_->get_font_size();

    // Setting up terminal stuff
    setup_pty(false, config_.default_window_width / font_size.first - 1);
    set_blocking_mode(false);

    // Init other stuff
    // buffer_ = std::make_unique<TermBuffer>(config_.default_window_width, config_.default_window_height, font_size.first, font_size.second);
    event_handler_ = std::make_unique<EventHandler>(*this);
    parser_ = std::make_unique<AnsiParser>(*this);


    event_handler_->subscribe<SDL_TextInputEvent>(SDL_TEXTINPUT, [this](const SDL_TextInputEvent& e) {
        on_textinput_event(e);
    });
    event_handler_->subscribe<SDL_KeyboardEvent>(SDL_KEYDOWN,   [this](const SDL_KeyboardEvent& e) { on_keys_pressed(e); });
    event_handler_->subscribe<SDL_MouseWheelEvent>(SDL_MOUSEWHEEL,[this](const SDL_MouseWheelEvent& e) {
        Sint32 scroll_dir = e.y;
        window_->scroll(scroll_dir);
    });
    event_handler_->subscribe<SDL_Event>(SDL_QUIT, [this](const SDL_Event& e) {
        on_quit_event(e);
    });
    event_handler_->subscribe<SDL_MouseMotionEvent>(SDL_MOUSEMOTION,[this](const SDL_MouseMotionEvent& e) {
        on_selection(e);
    });
    event_handler_->subscribe<SDL_Event>(SDL_MOUSEBUTTONDOWN,[this](const SDL_Event& e) {
        on_remove_selection(e);
    });
    event_handler_->subscribe<SDL_MouseButtonEvent>(SDL_MOUSEBUTTONUP,[this](const SDL_MouseButtonEvent& e) {
        reset_selection(e);
    });
    event_handler_->subscribe<SDL_WindowEvent>(SDL_WINDOWEVENT,[this](const SDL_WindowEvent& e) {
        window_event(e);
    });
}
Application::~Application() {
    close(master_fd_);
    close(slave_fd_);
}


void Application::init_sdl() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        throw std::runtime_error(SDL_GetError());
    }
}

void Application::init_ttf() {
    if (TTF_Init() < 0) {
        throw std::runtime_error(TTF_GetError());
    }
}

void Application::setup_pty(bool echo, int cols) {
    char slave_name[128];
    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = 10;
    int slave_id = forkpty(&master_fd_, slave_name, NULL, &ws);

    if (slave_id < 0) {
        throw std::runtime_error("Could not fork properly");
    } else if (slave_id == 0) {
        execl("/bin/bash", "-", NULL);
        throw std::runtime_error("Execl didn't work");
    }

    slave_fd_ = open(slave_name, O_RDONLY);
    if (slave_fd_ < 0) {
        throw std::runtime_error("Failed to open slave pty");
    }

    termios term_attribs;
    if (tcgetattr(slave_fd_, &term_attribs) != 0) {
        throw std::runtime_error("Failed to get terminal attributes");
    }
    term_attribs.c_lflag |= IUTF8;

    if (tcsetattr(slave_fd_, TCSANOW, &term_attribs) != 0) {
        throw std::runtime_error("Failed to set terminal attributes");
    }

    fds_[0].fd = master_fd_;
    fds_[0].events |= POLLIN;
}

void Application::set_blocking_mode(bool enabled) {
    int flags = fcntl(master_fd_, F_GETFL);
    if (!enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    fcntl(master_fd_, F_SETFL, flags);
}


void Application::run() {
    // Maybe some additional setup step
    loop();
}
void Application::loop() {
    std::string dirty_buffer;
    while (is_running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            event_handler_->handle_event(event);
        }

        int poll_status = poll(fds_, 1, 0);
        if (poll_status < 0) {
            throw std::runtime_error("Some kinda poll error");
        }

        if (fds_[0].revents & POLLIN) {
            char buf[1024];
            ssize_t rd_size;

            std::string output{};
            output.reserve(1024);
            while ((rd_size = read(master_fd_, buf, sizeof(buf))) > 0) {
                output.append(buf, rd_size);
            }
            parser_->parse(output);
            window_->set_should_render(true);
        }

        // window_->draw(*buffer_.get());
        window_->draw();
    }
}

void Application::on_textinput_event(const SDL_TextInputEvent& event) {
    const auto* text = event.text;
    write(master_fd_, text, SDL_strlen(text));
}
void Application::on_keys_pressed(const SDL_KeyboardEvent& event) {
    Uint16 mods = event.keysym.mod;
    SDL_Keycode keys = event.keysym.sym;

    if (keys == SDLK_RETURN) {
        send_newline();
    } else if (keys == SDLK_v && (mods & KMOD_CTRL) && (mods & KMOD_LSHIFT)) {
        char* clipboard_text = SDL_GetClipboardText();
        paste_text(clipboard_text);
        delete clipboard_text;
    } else if (keys == SDLK_c && (mods & KMOD_CTRL) && (mods & KMOD_LSHIFT)) {
        copy_selected_text();
    } else if (keys == SDLK_l && (mods & KMOD_CTRL)) {
        clear_text();
    } else if (keys == SDLK_BACKSPACE) {
        erase_character();
    } else if (keys == SDLK_LEFT || keys == SDLK_RIGHT || keys == SDLK_UP || keys == SDLK_DOWN) {
        on_arrowkey_pressed(keys);
    } else if (keys == SDLK_h && mods & KMOD_CTRL) {
        on_backspace();
    } else if (keys == SDLK_c && mods & KMOD_CTRL) {
        send_sigint();
    } else if (keys == SDLK_z && mods & KMOD_CTRL) {
        send_sigsusp();
    } else if (keys == SDLK_d && mods & KMOD_CTRL) {
        send_eof();
    } else if (keys == SDLK_r && mods & KMOD_CTRL) {
        reverse_find();
    } else if (keys == SDLK_a && mods & KMOD_CTRL) {
        cursor_to_back();
    } else if (keys == SDLK_e && mods & KMOD_CTRL) {
        cursor_to_end();
    }
}

void Application::on_scroll_event(const SDL_MouseWheelEvent& event) {
    Sint32 scroll_dir = event.y;
    // window_->scroll(scroll_dir, buffer_->get_cursor_pos(), buffer_->get_max_y());
    window_->scroll(scroll_dir);
}

void Application::on_quit_event([[maybe_unused]] const SDL_Event& event) {
    is_running_ = false;
}

// Select
void Application::on_selection(const SDL_MouseMotionEvent& event) {
    mouse_x = event.x;
    mouse_y = event.y;

    if (mouse_start_x != -1) {
        mouse_end_x = mouse_x;
        mouse_end_y = mouse_y;

        if (mouse_start_x <= 0 || mouse_start_y <= 0 || mouse_end_x <= 0 || mouse_end_y <= 0) return;
        window_->set_selection(mouse_start_x, mouse_start_y, mouse_end_x, mouse_end_y);
    }

}
void Application::on_remove_selection(const SDL_Event& event) {
    std::println("Cleared");
    window_->remove_selection();

    mouse_start_x = mouse_x;
    mouse_start_y = mouse_y;
}

void Application::reset_selection(const SDL_MouseButtonEvent& event) {
    mouse_start_x = -1;
    mouse_start_y = -1;
    mouse_end_x = -1;
    mouse_end_y = -1;
}


void Application::send_newline() {
    write(master_fd_, "\n", 1);
}

void Application::send_sigsusp() {
    const char sigsusp = 0x1A;
    write(master_fd_, &sigsusp, 1);
}

void Application::send_sigint() {
    const char sigint = 0x03;
    write(master_fd_, &sigint, 1); // SIGINT send
}

void Application::on_arrowkey_pressed(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_UP: {
            write(master_fd_, "\033[A", 3);
            break;
        }
        case SDLK_DOWN: {
            write(master_fd_, "\033[B", 3);
            break;
        }
        case SDLK_RIGHT: {
            write(master_fd_, "\033[C", 3);
            break;
        }
        case SDLK_LEFT: {
            write(master_fd_, "\033[D", 3);
            break;
        }
    }
}


void Application::erase_character() {
    const char backspace = 0x7F;
    write(master_fd_, &backspace, 1);
}
void Application::on_backspace() {
    const char backspace = 0x08;
    write(master_fd_, &backspace, 1);
}

void Application::send_eof() {
    const char eof = 0x04;
    write(master_fd_, &eof, 1);
}

void Application::clear_text() { // Clearing screen
    const char clear = 0x0C;
    write(master_fd_, &clear, 1);
}

void Application::reverse_find() {
    const char rev_find = 0x12;
    write(master_fd_, &rev_find, 1);
}

void Application::cursor_to_back() {
    const char cursor_to_the_back = 0x01;
    write(master_fd_, &cursor_to_the_back, 1);
}

void Application::cursor_to_end() {
    const char cursor_to_the_end = 0x05;
    write(master_fd_, &cursor_to_the_end, 1);
}

void Application::on_erase_event() {
    window_->erase_at_end();
}


void Application::on_set_cursor(int row, int col) {
    window_->set_cursor(row, col);
}

void Application::on_move_cursor(int row, int col) {
    window_->move_cursor(row, col);
}


void Application::on_add_cells(std::vector<Cell>&& cells) {
    window_->add_cells(std::move(cells));
}

void Application::paste_text(const char* text) {
    write(master_fd_, text, SDL_strlen(text));
}
void Application::on_reset_cursor(bool x_dir, bool y_dir) {
    window_->reset_cursor(x_dir, y_dir);
}

void Application::window_event(const SDL_WindowEvent& event) {
    if (event.event == SDL_WINDOWEVENT_RESIZED) {
        on_window_resized();
    }
}


void Application::on_clear_requested(bool remove) {
    window_->clear_buf(remove);
}

void Application::on_change_window_title(const std::string& win_title) {
    window_->set_window_title(win_title);
}
void Application::on_erase_in_line(int mode) {
    window_->erase_in_line(mode);
}

void Application::on_insert_chars(int n) {
    window_->insert_chars(n);
}
void Application::on_delete_chars(int n) {
    window_->delete_chars(n);
}

void Application::copy_selected_text() {
    auto text = window_->get_selected_text();
    SDL_SetClipboardText(text.c_str());
}

void Application::on_window_resized() {
    window_->resize();
    auto win_size = window_->get_window_size();
    auto font_size = window_->get_font_size();

    winsize wins;
    wins.ws_col = win_size.first / font_size.first;
    wins.ws_row = 20;
    ioctl(master_fd_, TIOCSWINSZ, &wins);
}
