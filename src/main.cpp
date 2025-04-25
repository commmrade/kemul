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
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <sys/poll.h>
#include <sys/types.h>
#include <utf8cpp/utf8.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <utf8cpp/utf8/checked.h>
#include <utf8cpp/utf8/cpp11.h>
#include "Application.hpp"


int main() {
    Application app{"FiraCode-Regular.ttf"};
    app.run();
    SDL_Quit();
    return 0;
}
