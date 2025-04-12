#include "GlyphCache.hpp"
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_ttf.h>
#include <optional>
#include <stdexcept>
#include <utf8cpp/utf8/cpp17.h>
#include <iostream>

GlyphCache::GlyphCache(SDL_Renderer* renderer, TTF_Font* font, int max_width, int max_height) : font_(font), max_width_(max_width), max_height_(max_height) {
    atlas_texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, max_width, max_height);
}
GlyphCache::~GlyphCache() {
    SDL_DestroyTexture(atlas_texture_);
}

void GlyphCache::add_glyph(SDL_Renderer* renderer, uint32_t codepoint) {
    std::string utf8_char = utf8::utf32to8(std::u32string{codepoint});
    SDL_Surface* glyph_surf = TTF_RenderUTF8_Blended(font_, utf8_char.c_str(), SDL_Color{255, 255, 255, 255});
    if (!glyph_surf) {
        std::cerr << "Glyph surface is null\n";
        std::cout << "char '" << utf8_char << "'" << std::endl;
        return;
    }
    
    SDL_Texture* glyph_texture = SDL_CreateTextureFromSurface(renderer, glyph_surf);
    if (!glyph_texture) {
        std::cerr << "Glyph texture is null\n";
        return;
    }
    if (atlas_x_ + glyph_surf->w > max_width_ || atlas_y_ + glyph_surf->h > max_height_) {
        SDL_DestroyTexture(glyph_texture);
        SDL_FreeSurface(glyph_surf);
        throw std::runtime_error("Should impl multi-texture atlas");
    }

    SDL_SetTextureBlendMode(glyph_texture, SDL_BLENDMODE_BLEND);

    SDL_Rect dest_rect = {atlas_x_, atlas_y_, glyph_surf->w, glyph_surf->h};
    SDL_SetRenderTarget(renderer, atlas_texture_);
    SDL_RenderCopy(renderer, glyph_texture, NULL, &dest_rect);
    SDL_SetRenderTarget(renderer, NULL);

    atlas_x_ += glyph_surf->w;
    if (atlas_x_ > max_width_) {
        atlas_y_ += TTF_FontHeight(font_);
        atlas_x_ = 0;
    }
    glyph_positions_[codepoint] = dest_rect;

    SDL_DestroyTexture(glyph_texture);
    SDL_FreeSurface(glyph_surf);
}

bool GlyphCache::glyph_exists(uint32_t codepoint) {
    return glyph_positions_.find(codepoint) != glyph_positions_.end();
}

std::optional<SDL_Rect> GlyphCache::get_glyph_pos(uint32_t codepoint) {
    if (auto iter = glyph_positions_.find(codepoint); iter != glyph_positions_.end()) {
        return {iter->second};
    }
    return std::nullopt;
}

SDL_Rect GlyphCache::get_or_create_glyph_pos(SDL_Renderer* renderer, uint32_t codepoint) {
    if (auto iter = glyph_positions_.find(codepoint); iter != glyph_positions_.end()) {
        return iter->second;
    }
    add_glyph(renderer, codepoint);
    return glyph_positions_.find(codepoint)->second;
}