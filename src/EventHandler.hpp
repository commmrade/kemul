#pragma once
#include <SDL_events.h>
#include <iostream>
#include "Application.hpp"

class EventHandler {
private:
    Application& application;
    
public:
    explicit EventHandler(Application& application);

    // Mouse selection variables
    int mouse_x{-1}, mouse_y{-1};
    int mouse_start_x{-1};
    int mouse_start_y{-1};
    int mouse_end_x{-1};
    int mouse_end_y{-1};

    void handle_event(SDL_Event& event);
};