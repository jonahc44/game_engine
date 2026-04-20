#ifndef TEXTMANAGER_H
#define TEXTMANAGER_H

#include "SDL2_ttf/SDL_ttf.h"
#include "SDL_render.h"
#include <filesystem>
#include <vector>
#include <unordered_map>

struct TextDrawRequest {
    std::string text;
    int x, y;
    std::string font_name;
    int size;
    int r, g, b, a;
};

class TextManager {
    private:
        const std::filesystem::path font_path = "resources/fonts/";
        static std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts;
        static std::vector<TextDrawRequest> draw_requests;

    public:
        TTF_Font* LoadFont(TextDrawRequest req);
        SDL_Texture* LoadTexture(SDL_Renderer* renderer, std::string txt);
        void RenderAll(SDL_Renderer* renderer);
        static void Draw(const std::string& str_content, float x, float y, const std::string& font_name, float font_size, float r, float g, float b, float a);
};

#endif