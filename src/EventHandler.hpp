#pragma once
#include <SDL_events.h>
#include "Application.hpp"
#include <functional>
#include <unordered_map>


using SlotType = std::function<void(const SDL_Event&)>;
class Dispatcher {
private:
    std::unordered_multimap<SDL_EventType, SlotType> observers;
public:
    void subscribe(SDL_EventType event_type, SlotType&& function) {
        observers.emplace(event_type, std::move(function));
    }
    void post(SDL_EventType event_type, SDL_Event& event) {
        auto range = observers.equal_range(event_type);
        for (auto it = range.first; it != range.second; ++it) {
            it->second(event);
        }
    }
};

class EventHandler {
private:
    Dispatcher m_dispatcher;
public:
    explicit EventHandler(Application& application);

    // Mouse selection variables
    void handle_event(SDL_Event& event);
    void subscribe(SDL_EventType event_type, SlotType&& function) {
        m_dispatcher.subscribe(event_type, std::move(function));
    }
};