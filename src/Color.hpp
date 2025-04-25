#pragma once
#include <SDL2/SDL_pixels.h>
bool inline operator==(const SDL_Color& lhs, const SDL_Color& rhs) {
    if (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a) {
        return true;
    }
    return false;
}