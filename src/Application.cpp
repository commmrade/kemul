#include "Application.hpp"
#include "Buffer.hpp"
#include "EventHandler.hpp"
#include "Window.hpp"
#include <SDL_clipboard.h>
#include <cstring>
#include <fcntl.h>
#include <memory>
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
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "ANSIParser.hpp"

Application::Application(const std::string &font_path, int width, int height) {
    init_sdl();
    init_ttf();
    
    window_ = std::make_unique<Window>(font_path, width, height); // Setting up window before so we can get font size
    auto font_size = window_->get_font_size();

    // Setting up terminal stuff
    setup_pty(false, width / font_size.first - 1);
    set_blocking_mode(false);

    // Init other stuff
    buffer_ = std::make_unique<TermBuffer>(width, height, font_size.first, font_size.second);
    event_handler_ = std::make_unique<EventHandler>(*this);
    parser_ = std::make_unique<AnsiParser>(*this);
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
    // term_attribs.c_lflag &= ~ICANON;
    // term_attribs.c_oflag &= ~ONLCR;

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
            // std::cout << output << std::endl;
            window_->set_should_render(true);
        }

        window_->process();
        window_->draw(*buffer_.get());
    }
}

void Application::on_textinput_event(const char* sym) {
    write(master_fd_, sym, SDL_strlen(sym));
    // std::cout << sym << std::endl;;
    window_->set_should_render(true);
}
void Application::on_enter_pressed_event() {
    write(master_fd_, "\n", 1);
    window_->set_should_render(true);
}

void Application::on_ctrl_z_pressed() {
    const char sigsusp = 0x1A;
    write(master_fd_, &sigsusp, 1);
}

void Application::on_ctrl_c_pressed() {
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

void Application::on_quit_event() {
    is_running_ = false;
}
void Application::on_backspace_pressed_event() {
    const char backspace = 0x7F;
    write(master_fd_, &backspace, 1);
}
void Application::on_ctrl_h_pressed() {
    const char backspace = 0x08;
    write(master_fd_, &backspace, 1);
}

void Application::on_ctrl_d_pressed() {
    const char eof = 0x04;
    write(master_fd_, &eof, 1);
}

void Application::on_ctrl_l_pressed() { // Clearing screen
    const char clear = 0x0C;
    write(master_fd_, &clear, 1);
}

void Application::on_ctrl_r_pressed() {
    const char rev_find = 0x12;
    write(master_fd_, &rev_find, 1);
}

void Application::on_ctrl_a_pressed() {
    const char cursor_to_the_back = 0x01;
    write(master_fd_, &cursor_to_the_back, 1);
}

void Application::on_ctrl_e_pressed() {
    const char cursor_to_the_end = 0x05;
    write(master_fd_, &cursor_to_the_end, 1);
}

void Application::on_erase_event() {
    buffer_->erase_last_symbol();
    window_->set_should_render(true);
}
void Application::on_scroll_event(Sint32 scroll_dir) {
    window_->scroll(scroll_dir, buffer_->get_cursor_pos(), buffer_->get_max_y());
    window_->set_should_render(true);
}

void Application::on_set_cursor(int row, int col) {
    buffer_->set_cursor_position(row, col);
}

void Application::on_move_cursor(int row, int col) {
    buffer_->move_cursor_pos_relative(row, col);
}


void Application::on_add_cells(std::vector<Cell> cells) {
    buffer_->add_cells(std::move(cells));
}

void Application::on_paste_event(std::string content) {
    on_textinput_event(content.c_str());
}
void Application::on_reset_cursor(bool x_dir, bool y_dir) {
    buffer_->reset_cursor(x_dir, y_dir);
}

void Application::on_clear_requested(bool remove) {
    if (remove) {
        buffer_->clear_all();
        window_->set_scroll_offset(0);
    } else {
        auto [x, y] = buffer_->get_cursor_pos();
        buffer_->set_cursor_position(buffer_->get_max_y() + 1, x);
        window_->set_scroll_offset(buffer_->get_max_y());
    }
    
    window_->set_should_render(true);
}

void Application::on_change_window_title(const std::string& win_title) {
    window_->set_window_title(win_title);
}
void Application::on_erase_in_line(int mode) {
    buffer_->erase_in_line(mode);
}

void Application::on_insert_chars(int n) {
    buffer_->insert_chars(n);
}
void Application::on_delete_chars(int n) {
    buffer_->delete_chars(n);
}

void Application::on_selection(int start_x, int start_y, int end_x, int end_y) {
    buffer_->set_selection(start_x, start_y, end_x, end_y, window_->get_scroll_offset());
    window_->set_should_render(true);
}
void Application::on_remove_selection() {
    buffer_->remove_selection();
}

void Application::on_copy_selection() {
    auto text = buffer_->get_selected_text();
    SDL_SetClipboardText(text.c_str());
}

void Application::on_window_resized() {
    window_->resize();
    auto win_size = window_->get_window_size();
    auto font_size = window_->get_font_size();
    buffer_->resize(win_size, font_size);

    // Update winsize somewhere
    winsize wins;
    wins.ws_col = win_size.first / font_size.first - 1;
    wins.ws_row = 10;
    ioctl(master_fd_, TIOCSWINSZ, &wins);
    window_->set_should_render(true);
}