#pragma once
#include <SDL_events.h>
#include "Application.hpp"
#include <functional>
#include <unordered_map>


using SlotType = std::function<void(const SDL_Event&)>;
class EventHandler {
private:
    std::unordered_multimap<SDL_EventType, SlotType> observers;
public:
    explicit EventHandler(Application& application);

    // Mouse selection variables
    void handle_event(SDL_Event& event);

    template <typename EventType, typename Function>
    void subscribe(SDL_EventType event_type, Function&& function) {
        observers.emplace(event_type, std::move(function));
    }
    void dispatch(SDL_EventType event_type, const SDL_Event& event);
};