#include "GlyphCache.hpp"
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_ttf.h>
#include <optional>
#include <stdexcept>
#include <utf8cpp/utf8/cpp17.h>
#include <iostream>
#include <utility>

GlyphCache::GlyphCache(SDL_Renderer* renderer, TTF_Font* font, std::pair<int, int> max_dimensions) : max_width_(max_dimensions.first), max_height_(max_dimensions.second) {
    reset_atlas(renderer);
}
GlyphCache::~GlyphCache() {
    SDL_DestroyTexture(atlas_texture_);
}

SDL_Rect GlyphCache::add_glyph(SDL_Renderer* renderer, TTF_Font* font, uint32_t codepoint) {
    std::string utf8_char = utf8::utf32to8(std::u32string{codepoint});
    SDL_Surface* glyph_surf = TTF_RenderUTF8_Blended(font, utf8_char.c_str(), SDL_Color{255, 255, 255, 255});
    if (!glyph_surf) {
        std::cerr << "Glyph surface is null\n";
        return{};
    }
    
    SDL_Texture* glyph_texture = SDL_CreateTextureFromSurface(renderer, glyph_surf);
    if (!glyph_texture) {
        std::cerr << "Glyph texture is null\n";
        SDL_FreeSurface(glyph_surf);
        return{};
    }

    SDL_SetTextureBlendMode(glyph_texture, SDL_BLENDMODE_NONE);

    if (atlas_x_ + glyph_surf->w > max_width_) {
        atlas_y_ += TTF_FontHeight(font);
        atlas_x_ = 0;
    }

    if (atlas_y_ + TTF_FontHeight(font) > max_height_) {
        reset_atlas(renderer);
    }

    SDL_Rect dest_rect = {atlas_x_, atlas_y_, glyph_surf->w, glyph_surf->h};
    SDL_SetRenderTarget(renderer, atlas_texture_);
    SDL_RenderCopy(renderer, glyph_texture, NULL, &dest_rect);
    SDL_SetRenderTarget(renderer, NULL);
    glyph_positions_[codepoint] = dest_rect;

    atlas_x_ += glyph_surf->w;

    SDL_DestroyTexture(glyph_texture);
    SDL_FreeSurface(glyph_surf);
    return dest_rect;
}

void GlyphCache::reset_atlas(SDL_Renderer* renderer) {
    SDL_DestroyTexture(atlas_texture_);
    atlas_texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, max_width_, max_height_);
    SDL_SetTextureBlendMode(atlas_texture_, SDL_BLENDMODE_BLEND); // So there is no black rectangle around glyphs which makes drawing BG color impossible
    glyph_positions_.clear();
    atlas_x_ = 0;
    atlas_y_ = 0;
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

SDL_Rect GlyphCache::get_or_create_glyph_pos(SDL_Renderer* renderer, TTF_Font* font, uint32_t codepoint) {
    if (auto iter = glyph_positions_.find(codepoint); iter != glyph_positions_.end()) {
        return iter->second;
    }
    auto rect = add_glyph(renderer, font, codepoint);
    return rect;
}