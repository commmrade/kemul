#pragma once
#include "EventHandler.hpp"

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
            } else {
                application.on_keydown_event(event.key.keysym.sym);
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

