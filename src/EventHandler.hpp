#pragma once
#include <SDL_events.h>
#include <iostream>
#include "Application.hpp"

class EventHandler {
private:
    Application& application;
    
public:
    explicit EventHandler(Application& application);


    void handle_event(SDL_Event& event);
};