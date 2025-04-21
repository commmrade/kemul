#include "Application.hpp"
#include "Buffer.hpp"
#include "EventHandler.hpp"
#include "Window.hpp"
#include <algorithm>
#include <fcntl.h>
#include <memory>
#include <pty.h>
#include <sstream>
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
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include "ANSIParser.hpp"

Application::Application(const std::string &font_path, int width, int height) {
    setup_pty();

    init_sdl();
    init_ttf();

    
    set_blocking_mode(false);

    window_ = std::make_unique<Window>(font_path);

    int font_w, font_h;
    TTF_SizeText(window_->font_, " ", &font_w, &font_h);
    buffer_ = std::make_unique<TermBuffer>(width, height, font_w, font_h);
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

void Application::setup_pty() {

    char slave_name[128];
    int slave_id = forkpty(&master_fd_, slave_name, NULL, NULL);
    slave_fd_ = open(slave_name, O_RDONLY);
    set_echo_mode(false);

    if (slave_id < 0) {
        throw std::runtime_error("Could not fork properly");
    } else if (slave_id == 0) {
        execl("/bin/bash", "-", NULL);
        throw std::runtime_error("Execl didn't work");
    }

    fds_[0].fd = master_fd_;
    fds_[0].events |= POLLIN;
}

void Application::set_echo_mode(bool enabled) {
    termios term_attribs;
    tcgetattr(slave_fd_, &term_attribs);
    if (enabled) {
        term_attribs.c_lflag |= (ECHO | ICANON);
    } else {
        term_attribs.c_lflag &= ~(ECHO | ICANON);
    }
    tcsetattr(slave_fd_, TCSANOW, &term_attribs);
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

            // std::string output{};
            // output.reserve(1024);
            // while ((rd_size = read(master_fd_, buf, sizeof(buf))) > 0) {
            //     output.append(buf, rd_size);
            // }
            // std::istringstream ss{std::move(output)};
            // std::string line;
            // while (std::getline(ss, line, '\n')) {
            //     parser_->parse(line);
            // }
            

            // ==== OR

            while (true) {
                ssize_t rd_size = read(master_fd_, buf, sizeof(buf));
                if (rd_size <= 0) break;
            
                dirty_buffer.append(buf, rd_size);
            
                size_t pos = 0;
                while ((pos = dirty_buffer.find('\n')) != std::string::npos) {
                    std::string line = dirty_buffer.substr(0, pos);
                    parser_->parse(line);
                    dirty_buffer.erase(0, pos + 1);
                }
            }
            if (!dirty_buffer.empty()) {
                parser_->parse(dirty_buffer);
                dirty_buffer.clear();
            }

            window_->set_should_render(true);
        }

        window_->process();
        window_->draw(*buffer_.get());
    }
}

void Application::on_textinput_event(const char* sym) {
    write(master_fd_, sym, SDL_strlen(sym));
    buffer_->add_str_command(sym);
    window_->set_should_render(true);
}
void Application::on_enter_pressed_event() {
    write(master_fd_, "\n", 1);
    // buffer_->add_str(buffer_->get_command());
    buffer_->clear_command();
    window_->set_should_render(true);
}
void Application::on_quit_event() {
    is_running_ = false;
}
void Application::on_keydown_event(SDL_Keycode key) {
    
}
void Application::on_scroll_event(Sint32 scroll_dir) {
    std::cout << scroll_dir << std::endl;
    window_->scroll(scroll_dir);
    window_->set_should_render(true);
}


void Application::on_set_cells(std::vector<Cell> cells) {
    buffer_->push_cells(std::move(cells));
}
void Application::on_move_cursor(int row, int col) {
    buffer_->set_cursor(row, col);
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

void Application::on_clear_requested() {
    buffer_->clear_all();
    window_->set_should_render(true);
}

void Application::on_change_window_title(const std::string& win_title) {
    window_->set_window_title(win_title);
}