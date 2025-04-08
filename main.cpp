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
// #include <SDL_blendmode.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <SDL2/SDL.h>
#include <sstream>
#include <stdexcept>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <unordered_map>
#include <utf8cpp/utf8.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <utf8cpp/utf8/checked.h>
#include <utf8cpp/utf8/cpp11.h>
#include <utility>
#include <vector>

const int PTSIZE = 14;

void init_sdl() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        throw std::runtime_error(SDL_GetError());
    }

    if (TTF_Init() < 0) {
        throw std::runtime_error(TTF_GetError());
    }
}

void cache_glyph_to_atlas(SDL_Texture* atlas, uint32_t codepoint, SDL_Renderer* renderer, int& atlas_x, int& atlas_y, std::unordered_map<uint32_t, SDL_Rect>& cache_glyphs, TTF_Font* font, int atlas_width) {
    std::string utf8_char = utf8::utf32to8(std::u32string{codepoint});
    SDL_Surface* glyph_surf = TTF_RenderUTF8_Blended(font, utf8_char.c_str(), SDL_Color{200, 200, 200, 255});
    if (!glyph_surf) {
        std::cerr << "Glyph surface is null\n";
        return;
    }

    SDL_Texture* glyph_texture = SDL_CreateTextureFromSurface(renderer, glyph_surf);
    if (!glyph_texture) {
        std::cerr << "Glyph texture is null\n";
        return;
    }
    SDL_SetTextureBlendMode(glyph_texture, SDL_BLENDMODE_BLEND);

    SDL_Rect dest_rect = {atlas_x, atlas_y, glyph_surf->w, glyph_surf->h};
    SDL_SetRenderTarget(renderer, atlas);
    SDL_RenderCopy(renderer, glyph_texture, NULL, &dest_rect);
    SDL_SetRenderTarget(renderer, NULL);

    atlas_x += glyph_surf->w;
    if (atlas_x > atlas_width) {
        atlas_y += TTF_FontHeight(font);
        atlas_x = 0;
    }
    cache_glyphs[codepoint] = dest_rect;

    SDL_DestroyTexture(glyph_texture);
    SDL_FreeSurface(glyph_surf);
}

int main() {

    int master_fd;
    char slave_name[512];
    int child_id = forkpty(&master_fd, slave_name, NULL, NULL);


    {
        int slave_fd = open(slave_name, O_RDONLY);
        termios term_attribs;
        tcgetattr(slave_fd, &term_attribs);
        term_attribs.c_lflag &= ~ECHO;
        tcsetattr(slave_fd, TCSANOW, &term_attribs);
        close(slave_fd);
    }

    if (child_id < 0) {
        throw std::runtime_error("Forkpty error");
    } else if (child_id == 0) {
        execl("/bin/bash", "-", nullptr);
        throw std::runtime_error("Could not execl");
    }

    pollfd fds[1] = {master_fd};
    fds[0].events = POLLIN;

    int flags = fcntl(master_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(master_fd, F_SETFL, flags);

    init_sdl();

    TTF_Font* font = TTF_OpenFont("Roboto-Regular.ttf", PTSIZE);
    if (!font) {
        throw std::runtime_error(TTF_GetError());
    }
    SDL_Window* window = SDL_CreateWindow("Kemul", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 900, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        throw std::runtime_error(SDL_GetError());
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        throw std::runtime_error(SDL_GetError());
    }

    std::vector<std::string> buffer{};
    std::string current_command{};
    int scroll_offset{0};

    // Caching part ==========================
    SDL_RendererInfo render_info;
    SDL_GetRendererInfo(renderer, &render_info);

    SDL_Texture* glyph_atlas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, render_info.max_texture_width, render_info.max_texture_height);
    std::unordered_map<uint32_t, SDL_Rect> cache_glyphs;

    int atlas_x {0};
    int atlas_y {0};

    // Caching part ==========================

    bool is_running{true};
    bool should_render{true};
    while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            switch(event.type) {
                case SDL_QUIT: {
                    is_running = false;
                    break;
                }
                case SDL_TEXTINPUT: {
                    write(master_fd, event.text.text, SDL_strlen(event.text.text));
                    current_command.append(event.text.text, SDL_strlen(event.text.text));
                    should_render = true;
                    break;
                }
                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym == SDLK_RETURN) {
                        write(master_fd, "\n", 1);
                        buffer.back() += current_command;
                        current_command.clear();
                        should_render = true;
                    } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                        current_command.clear();
                        buffer.clear();
                        current_command = "";
                        should_render = true;
                    }
                    break;
                }
                case SDL_MOUSEWHEEL: {
                    if (event.wheel.y < 0) {
                        scroll_offset += 2;
                    } else if (event.wheel.y > 0) {
                        if (scroll_offset > 0) {
                            scroll_offset -= 2;
                        }
                    }
                    should_render = true;
                    break;
                }
            }
        }

        int poll_status = poll(fds, 1, 0);
        if (poll_status < 0) {
            throw std::runtime_error("Some kinda poll error");
        }

        if (fds[0].revents & POLLIN) {
            char buf[256];
            ssize_t rd_size;

            std::string output{};
            output.reserve(256);
            while ((rd_size = read(master_fd, buf, sizeof(buf))) > 0) {
                output.append(buf, rd_size);
            }

            std::istringstream ss{std::move(output)};
            std::string line;
            while (std::getline(ss, line)) {
                buffer.push_back(line);
            }
            should_render = true;
        }

        if (!should_render) {
            SDL_Delay(16);
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        int x {10};
        int y{10};

        for (auto i = scroll_offset; i < buffer.size(); ++i) {
            std::vector<uint32_t> codepoints;
            utf8::utf8to32(buffer[i].cbegin(), buffer[i].cend(), std::back_inserter(codepoints));
            
            for (auto codepoint : codepoints) {
                if (cache_glyphs.find(codepoint) == cache_glyphs.end()) {
                    cache_glyph_to_atlas(glyph_atlas, codepoint, renderer, atlas_x, atlas_y, cache_glyphs, font, render_info.max_texture_width);
                }

                SDL_Rect src = cache_glyphs[codepoint];
                SDL_Rect glyph_rect{x, y, src.w, src.h};
                SDL_RenderCopy(renderer, glyph_atlas, &src, &glyph_rect);

                x += src.w;
                if (x >= 900 - 50) {
                    x = 10;
                    y += src.h;
                }
            }

            if (i != buffer.size() - 1) {
                y += TTF_FontHeight(font);
                x = 10;
            } else {

            }

        }

        std::vector<uint32_t> codepoints;
        
        utf8::utf8to32(current_command.cbegin(), current_command.cend(), std::back_inserter(codepoints));
        for (auto codepoint : codepoints) {

            if (cache_glyphs.find(codepoint) == cache_glyphs.end()) {
                cache_glyph_to_atlas(glyph_atlas, codepoint, renderer, atlas_x, atlas_y, cache_glyphs, font, render_info.max_texture_width);
            }


            SDL_Rect src = cache_glyphs[codepoint];
            SDL_Rect glyph_rect{x, y, src.w, src.h};
            SDL_RenderCopy(renderer, glyph_atlas, &src, &glyph_rect);

            x += src.w;
            if (x >= 900 - 50) {
                x = 10;
                y += src.h;
            }
        }

        SDL_RenderPresent(renderer);

        should_render = false;
    }

    SDL_DestroyTexture(glyph_atlas);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
