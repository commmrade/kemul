#include "EventHandler.hpp"
#include <SDL_clipboard.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_video.h>

EventHandler::EventHandler(Application& application) {}

void EventHandler::handle_event(SDL_Event& event) {
    switch (event.type) {
        case SDL_TEXTINPUT: {
            m_dispatcher.post(SDL_TEXTINPUT, event);
            break;
        }
        case SDL_KEYDOWN: {
            m_dispatcher.post(SDL_KEYDOWN, event);
            break;
        }
        case SDL_MOUSEWHEEL: {
            m_dispatcher.post(SDL_MOUSEWHEEL, event);
            break;
        }
        case SDL_QUIT: {
            m_dispatcher.post(SDL_QUIT, event);
            break;
        }
        case SDL_MOUSEMOTION: {
            m_dispatcher.post(SDL_MOUSEMOTION, event);
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            m_dispatcher.post(SDL_MOUSEBUTTONDOWN, event);
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            m_dispatcher.post(SDL_MOUSEBUTTONUP, event);
            break;
        }
        case SDL_WINDOWEVENT: {
            m_dispatcher.post(SDL_WINDOWEVENT, event);
            // COnnect app to this
            break;
        }
    }
}

