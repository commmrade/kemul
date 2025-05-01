#pragma once
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

class GlyphCache {
private:
    SDL_Texture* atlas_texture_;

    int max_width_;
    int max_height_;

    int atlas_x_{0};
    int atlas_y_{0};

    std::unordered_map<uint32_t, SDL_Rect> glyph_positions_;
public:
    explicit GlyphCache(SDL_Renderer* renderer, TTF_Font* font, std::pair<int, int> max_dimensions);
    ~GlyphCache();

    void add_glyph(SDL_Renderer* renderer, TTF_Font* font, uint32_t codepoint);

    bool glyph_exists(uint32_t codepoint);
    std::optional<SDL_Rect> get_glyph_pos(uint32_t codepoint);
    SDL_Rect get_or_create_glyph_pos(SDL_Renderer* renderer, TTF_Font* font, uint32_t codepoint);
    SDL_Texture* const atlas() const { return atlas_texture_; }
};
