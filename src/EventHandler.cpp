#include "EventHandler.hpp"
#include <SDL_clipboard.h>
#include <SDL_events.h>
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
            } else if (event.key.keysym.sym == SDLK_c && (event.key.keysym.mod & KMOD_CTRL) && (event.key.keysym.mod & KMOD_LSHIFT)) {
                application.on_copy_selection();
            } else if (event.key.keysym.sym == SDLK_l && (event.key.keysym.mod & KMOD_CTRL)) {
                application.on_ctrl_l_pressed();
            } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                application.on_backspace_pressed_event();
            } else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
                application.on_arrowkey_pressed(event.key.keysym.sym);
            } else if (event.key.keysym.sym == SDLK_h && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_h_pressed();
            } else if (event.key.keysym.sym == SDLK_c && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_c_pressed();
            } else if (event.key.keysym.sym == SDLK_z && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_z_pressed();
            } else if (event.key.keysym.sym == SDLK_d && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_d_pressed();
            } else if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_r_pressed();
            } else if (event.key.keysym.sym == SDLK_a && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_a_pressed();
            } else if (event.key.keysym.sym == SDLK_e && event.key.keysym.mod & KMOD_CTRL) {
                application.on_ctrl_e_pressed();
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
        case SDL_MOUSEMOTION: {
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;

            if (mouse_start_x != -1) {
                mouse_end_x = mouse_x;
                mouse_end_y = mouse_y;

                if (mouse_start_x < 0 || mouse_start_y < 0 || mouse_end_x < 0 || mouse_end_y < 0) return;
                application.on_remove_selection();
                application.on_selection(mouse_start_x, mouse_start_y, mouse_end_x, mouse_end_y);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            mouse_start_x = mouse_x;
            mouse_start_y = mouse_y;
            application.on_remove_selection();
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            mouse_start_x = -1;
            mouse_start_y = -1;
            mouse_end_x = -1;
            mouse_end_y = -1;
            break;
        }
    }
}

