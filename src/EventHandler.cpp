#include "EventHandler.hpp"
#include <SDL_clipboard.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_video.h>

EventHandler::EventHandler(Application& application) : application(application) {
    
}

void EventHandler::handle_event(SDL_Event& event) {
    switch (event.type) {
        case SDL_TEXTINPUT: {
            application.on_textinput_event(event.text.text);
            break;
        }
        case SDL_KEYDOWN: {
            application.on_keys_pressed(event.key.keysym.mod, event.key.keysym.sym);
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
        case SDL_MOUSEMOTION: {
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;

            if (mouse_start_x != -1) {
                mouse_end_x = mouse_x;
                mouse_end_y = mouse_y;

                if (mouse_start_x <= 0 || mouse_start_y <= 0 || mouse_end_x <= 0 || mouse_end_y <= 0) return;
                application.on_selection(mouse_start_x, mouse_start_y, mouse_end_x, mouse_end_y);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            application.on_remove_selection();
            mouse_start_x = mouse_x;
            mouse_start_y = mouse_y;
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            mouse_start_x = -1;
            mouse_start_y = -1;
            mouse_end_x = -1;
            mouse_end_y = -1;
            break;
        }
        case SDL_WINDOWEVENT: {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                application.on_window_resized();
            }
            break;
        }
    }
}

