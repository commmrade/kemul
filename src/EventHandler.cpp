#include "EventHandler.hpp"
#include <SDL_clipboard.h>
#include <SDL_keycode.h>

EventHandler::EventHandler(Application& application) : application(application) {
    
}

void EventHandler::handle_event(SDL_Event& event) {
    switch (event.type) {
        case SDL_TEXTINPUT: {
            application.on_textinput_event(event.text.text);
            break;
        }
        case SDL_KEYDOWN: {
            if (event.key.keysym.sym == SDLK_RETURN) {
                application.on_enter_pressed_event();
            } else if (event.key.keysym.sym == SDLK_v && (event.key.keysym.mod & KMOD_CTRL) && (event.key.keysym.mod & KMOD_LSHIFT)) {
                char* clipboard_text = SDL_GetClipboardText();
                application.on_paste_event(clipboard_text);
                delete clipboard_text;
            } else if (event.key.keysym.sym == SDLK_l && (event.key.keysym.mod & KMOD_CTRL)) {
                application.on_clear_requested();
            } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                application.on_backspace_pressed_event();
            } else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
                application.on_arrowkey_pressed(event.key.keysym.sym);
            } else {
                // application.on_keydown_event(event.key.keysym.sym);
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            application.on_scroll_event(event.wheel.y);
            break;
        }
        case SDL_QUIT: {
            application.on_quit_event();
            break;
        }
    }
}

