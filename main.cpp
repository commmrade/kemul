#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <pty.h>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <termios.h>
#include <iterator>
#include <unistd.h>
#include <fcntl.h>
#include <SDL2/SDL.h>
#include <utf8cpp/utf8/checked.h>
#include <vector>
#include <utf8cpp/utf8.h>
#include <SDL2/SDL_ttf.h>
#include <sstream>

int main() {
    int master_fd;
    char slave_name[512];
    int fork_id = forkpty(&master_fd, slave_name, NULL, NULL);
    std::cout << slave_name << std::endl;

    {
        int slave_fd = open(slave_name, O_RDONLY);
        termios term_attribs;
        tcgetattr(slave_fd, &term_attribs);
        term_attribs.c_lflag &= ~ECHO;
        tcsetattr(slave_fd, TCSANOW, &term_attribs);
        close(slave_fd);
    }

    if (fork_id == 0) {
        execl("/bin/bash", "-", nullptr);
        throw "fork failed";
    }

    pollfd fds[1] = {master_fd};
    fds[0].events = POLLIN;

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        throw std::runtime_error("SDL failed to init");
    }
    if (TTF_Init() < 0) {
        throw std::runtime_error("TTF Failed to init");
    }

    SDL_Window* window = SDL_CreateWindow("Just a title", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window) {
        throw std::runtime_error("Error creating window");
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error("Could not create a renderer");
    }
    bool is_running = true;
    bool should_render = true;

    TTF_Font* textFont = TTF_OpenFont("/usr/share/fonts/TTF/FiraCodeNerdFontMono-Regular.ttf", 15);

    std::vector<std::string> commands;
    std::string command;

    while (is_running) {
        SDL_Event event;

        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_TEXTINPUT: {
                    command += event.text.text;
                    write(master_fd, event.text.text, 4);
                    should_render = true;
                    break;
                }
                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        write(master_fd, "\n", 1);
                        commands.push_back(command);
                        command.clear();
                        should_render = true;
                    }
                    if (event.key.keysym.sym == SDLK_BACKSPACE) {
                        command.clear();
                        commands.clear();
                        should_render = true;
                    }
                    break;
                }
                case SDL_QUIT: {
                    is_running = false;
                    break;
                }
            }
        }

        int ret = poll(fds, 1, 0);
        if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                char buf[8096];
                int rd_size = read(master_fd, buf, sizeof(buf));
                std::string output(buf, rd_size);
                std::istringstream ss{std::move(output)};

                std::string line;
                while (std::getline(ss, line)) {
                    commands.push_back(std::move(line));
                }

                std::cout.write(buf, rd_size);
                should_render = true;
            }
        }

        if (!should_render) {
            SDL_Delay(16);
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int x = 50, y = 50;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);

        for (const auto& line : commands) {
            std::vector<uint32_t> codepoints;
            utf8::utf8to32(line.begin(), line.end(), std::back_inserter(codepoints));
            for (auto codepoint : codepoints) {
                std::string utf8_char = utf8::utf32to8(std::u32string{codepoint});
                SDL_Surface* text_surface = TTF_RenderUTF8_Blended(textFont, utf8_char.c_str(), SDL_Color{100, 100, 100, 255});
                if (!text_surface) {
                    throw std::runtime_error(TTF_GetError());
                }
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
                if (!texture) {
                    SDL_FreeSurface(text_surface);
                    throw std::runtime_error(SDL_GetError());
                }
                SDL_Rect rect;
                rect.x = x;
                x += text_surface->w;
                rect.y = y;
                if (rect.x > width - 50) {
                    y += text_surface->h;
                    x = 50;
                }
                rect.w = text_surface->w;
                rect.h = text_surface->h;
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_FreeSurface(text_surface);
                SDL_DestroyTexture(texture);
            }
            y += TTF_FontHeight(textFont);
            x = 50;
        }
        std::vector<uint32_t> codepoints;
        utf8::utf8to32(command.begin(), command.end(), std::back_inserter(codepoints));
        for (auto codepoint : codepoints) {
            std::string utf8_char = utf8::utf32to8(std::u32string{codepoint});
            SDL_Surface* text_surface = TTF_RenderUTF8_Blended(textFont, utf8_char.c_str(), SDL_Color{100, 100, 100, 255});
            if (!text_surface) {
                throw std::runtime_error(TTF_GetError());
            }
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if (!texture) {
                SDL_FreeSurface(text_surface);
                throw std::runtime_error(SDL_GetError());
            }
            SDL_Rect rect;
            rect.x = x;
            x += text_surface->w;
            rect.y = y;
            if (rect.x > width - 50) {
                y += text_surface->h;
                x = 50;
            }
            rect.w = text_surface->w;
            rect.h = text_surface->h;
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(text_surface);
            SDL_DestroyTexture(texture);
        }

        SDL_RenderPresent(renderer);
        should_render = false;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
