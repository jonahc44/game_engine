#include "TextManager.h"
#include "Helper.h"
#include "SDL2_ttf/SDL_ttf.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include <iostream>

namespace fs = std::filesystem;

std::vector<TextDrawRequest> TextManager::draw_requests;
std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> TextManager::fonts;

TTF_Font* TextManager::LoadFont(TextDrawRequest req) {
    if (!TTF_WasInit()) TTF_Init();

    if (fonts[req.font_name].count(req.size) == 0) {
        fs::path font_file = font_path / (req.font_name + ".ttf");

        if (!fs::exists(font_file)) {
            std::cout << "error: font " << req.font_name << " missing";
            exit(0);
        }

        TTF_Font* font = TTF_OpenFont(font_file.string().c_str(), req.size);

        if (font == nullptr) {
            std::cout << "error: font " << req.font_name << " could not be rendered";
            exit(0);
        }

        fonts[req.font_name][req.size] = font;
    }

    return fonts[req.font_name][req.size];
}

void TextManager::RenderAll(SDL_Renderer* renderer) {
    for (const auto& req : draw_requests) {
        TTF_Font* active_font = LoadFont(req);

        SDL_Color color = { static_cast<Uint8>(req.r), static_cast<Uint8>(req.g), static_cast<Uint8>(req.b), static_cast<Uint8>(req.a) };
        SDL_Surface* surface = TTF_RenderText_Solid(active_font, req.text.c_str(), color);
        if (!surface) {
            // std::cout << "TTF ERROR: " << TTF_GetError() << '\n';
            continue;
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!texture) {
            // std::cout << "TEXTURE ERROR: " << SDL_GetError() << '\n';
            continue;
        }

        float tex_w = 0;
        float tex_h = 0;
        Helper::SDL_QueryTexture(texture, &tex_w, &tex_h);

        SDL_FRect dstrect = { static_cast<float>(req.x), static_cast<float>(req.y), static_cast<float>(tex_w), static_cast<float>(tex_h) };
        SDL_FPoint center = { 0.0f, 0.0f }; 
        
        Helper::SDL_RenderCopyEx(-1, "", renderer, texture, NULL, &dstrect, 0.0f, &center, SDL_FLIP_NONE);
        SDL_DestroyTexture(texture);
    }
    
    draw_requests.clear();
}

// SDL_Texture* TextManager::LoadTexture(SDL_Renderer* renderer, std::string txt) {
//     SDL_Surface* surf = TTF_RenderText_Solid(font, txt.c_str(), {255, 255, 255, 255});
//     SDL_Texture* t = SDL_CreateTextureFromSurface(renderer, surf);
//     SDL_FreeSurface(surf);
//     return t;
// }

void TextManager::Draw(const std::string& str_content, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a) {
    TextDrawRequest req;
    req.text = str_content;
    req.x = static_cast<int>(x);
    req.y = static_cast<int>(y);
    req.font_name = font_name;
    req.size = static_cast<int>(font_size);
    req.r = static_cast<int>(r);
    req.g = static_cast<int>(g);
    req.b = static_cast<int>(b);
    req.a = static_cast<int>(a);
    
    draw_requests.push_back(req);
}