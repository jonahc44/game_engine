#include "ImageManager.h"
#include "Helper.h"
#include "SDL2_image/SDL_image.h"
#include "SDL_render.h"
#include "glm/glm.hpp"
#include <filesystem>
#include <iostream>
#include <cstring>

namespace fs = std::filesystem;

SDL_Texture* ImageManager::LoadTexture(const std::string& img_name) {
    if (texture_cache.find(img_name) != texture_cache.end()) {
        return texture_cache[img_name];
    }

    fs::path image_path = image_dir / (img_name + ".png");

    if (!fs::exists(image_path)) {
        std::cout << "error: missing image " << img_name;
        exit(0);
    }

    SDL_Texture* tex = IMG_LoadTexture(ImageManager::renderer, image_path.string().c_str());
    if (!tex) {
        std::cout << "error: missing image " << img_name << '\n';
        exit(0);
    }

    texture_cache[img_name] = tex;
    return tex;
}

void ImageManager::CreateStreamingTexture(const std::string& texture_name, int grid_width, int grid_height) {
    if (texture_cache.find(texture_name) != texture_cache.end()) return;

    SDL_Texture* tex = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        grid_width, 
        grid_height
    );

    if (!tex) {
        std::cout << "error: failed to create streaming texture " << texture_name << '\n';
        exit(0);
    }

    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    texture_cache[texture_name] = tex;
}

void ImageManager::UpdateStreamingTexture(const std::string& texture_name, const std::vector<Uint32>& pixel_buffer, int grid_width, int grid_height) {
    if (texture_cache.find(texture_name) == texture_cache.end()) {
        std::cout << "error: attempted to update non-existent streaming texture " << texture_name << '\n';
        return;
    }

    SDL_Texture* tex = texture_cache[texture_name];
    void* pixels;
    int pitch;

    SDL_LockTexture(tex, NULL, &pixels, &pitch);

    for (int y = 0; y < grid_height; ++y) {
        Uint8* dst = static_cast<Uint8*>(pixels) + (y * pitch);
        const Uint32* src = pixel_buffer.data() + (y * grid_width);
        memcpy(dst, src, grid_width * sizeof(Uint32));
    }
    SDL_UnlockTexture(tex);
}

void ImageManager::DrawUI(const std::string& image_name, float x, float y) {
    DrawUIEx(image_name, x, y, 255.0f, 255.0f, 255.0f, 255.0f, 0.0f);
}

void ImageManager::DrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order) {
    ui_requests.push_back({image_name, static_cast<int>(x), static_cast<int>(y), 0, 1.0f, 1.0f, 0.0f, 0.0f, 
                           static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a), static_cast<int>(sorting_order)});
}

void ImageManager::Draw(const std::string& image_name, float x, float y) {
    DrawEx(image_name, x, y, 0.0f, 1.0f, 1.0f, 0.5f, 0.5f, 255.0f, 255.0f, 255.0f, 255.0f, 0.0f);
}

void ImageManager::DrawEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x, float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a, float sorting_order) {
    scene_requests.push_back({image_name, x, y, static_cast<int>(rotation_degrees), 
                              scale_x, scale_y, pivot_x, pivot_y, 
                              static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a), static_cast<int>(sorting_order)});
}

void ImageManager::DrawPixel(float x, float y, float r, float g, float b, float a) {
    pixel_requests.push_back({static_cast<int>(x), static_cast<int>(y), 
                              static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), static_cast<int>(a)});
}

void ImageManager::RenderScene(float cam_pos_x, float cam_pos_y, int cam_w, int cam_h, float zoom_factor) {
    SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

    std::stable_sort(scene_requests.begin(), scene_requests.end(), [](const ImageDrawRequest& a, const ImageDrawRequest& b) {
        return a.sorting_order < b.sorting_order;
    });

    for (const auto& req : scene_requests) {
        SDL_Texture* tex = LoadTexture(req.image_name);
        float tex_w_f, tex_h_f;
        Helper::SDL_QueryTexture(tex, &tex_w_f, &tex_h_f);
        int tex_w = static_cast<int>(tex_w_f);
        int tex_h = static_cast<int>(tex_h_f);

        SDL_SetTextureColorMod(tex, req.r, req.g, req.b);
        SDL_SetTextureAlphaMod(tex, req.a);

        float abs_scale_x = glm::abs(req.scale_x);
        float abs_scale_y = glm::abs(req.scale_y);

        float scaled_w = tex_w * abs_scale_x;
        float scaled_h = tex_h * abs_scale_y;

        float pivot_x = req.pivot_x * scaled_w;
        float pivot_y = req.pivot_y * scaled_h;

        SDL_RendererFlip flip = SDL_FLIP_NONE;
        if (req.scale_x < 0) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
        if (req.scale_y < 0) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);

        glm::vec2 final_rendering_position = glm::vec2(req.x, req.y) - glm::vec2(cam_pos_x, cam_pos_y);

        float final_x = final_rendering_position.x * 100.0f + (cam_w * 0.5f * (1.0f / zoom_factor)) - pivot_x;
        float final_y = final_rendering_position.y * 100.0f + (cam_h * 0.5f * (1.0f / zoom_factor)) - pivot_y;

        SDL_FRect dstrect = {
            final_x,
            final_y,
            scaled_w,
            scaled_h
        };

        SDL_FPoint center_point = { 
            pivot_x, 
            pivot_y 
        };

        Helper::SDL_RenderCopyEx(0, "", renderer, tex, NULL, &dstrect, req.rotation, &center_point, flip);

        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    scene_requests.clear();
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void ImageManager::RenderUi() {
    std::stable_sort(ui_requests.begin(), ui_requests.end(), [](const UIDrawRequest& a, const UIDrawRequest& b) {
        return a.sorting_order < b.sorting_order;
    });

    for (const auto& req : ui_requests) {
        SDL_Texture* tex = LoadTexture(req.image_name);
        float tex_w, tex_h;
        Helper::SDL_QueryTexture(tex, &tex_w, &tex_h);

        SDL_SetTextureColorMod(tex, req.r, req.g, req.b);
        SDL_SetTextureAlphaMod(tex, req.a);

        SDL_FRect dstrect = { static_cast<float>(req.x), static_cast<float>(req.y), static_cast<float>(tex_w), static_cast<float>(tex_h) };
        
        Helper::SDL_RenderCopyEx(0, "", renderer, tex, NULL, &dstrect, 0.0f, NULL, SDL_FLIP_NONE);

        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }
    ui_requests.clear();
}

void ImageManager::RenderPixels() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (const auto& req : pixel_requests) {
        SDL_SetRenderDrawColor(renderer, req.r, req.g, req.b, req.a);
        SDL_RenderDrawPoint(renderer, req.x, req.y);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    pixel_requests.clear();
}

void ImageManager::CreateDefaultParticle() {
    if (texture_cache.find("") != texture_cache.end()) return;

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
    Uint32 white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
    SDL_FillRect(surface, NULL, white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    texture_cache[""] = texture;
}

glm::vec2 ImageManager::GetWindowSize() {
    int x = 0;
    int y = 0;
    SDL_RenderGetLogicalSize(renderer, &x, &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

// void ImageManager::CreateDefaultFluid(uint width, uint height) {
//     if (texture_cache.find("") != texture_cache.end()) return;

//     SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
//     Uint32 blue = SDL_MapRGBA(surface->format, 0, 0, 255, 255);
//     SDL_FillRect(surface, NULL, blue);
//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
//     SDL_FreeSurface(surface);
//     texture_cache[""] = texture;
// }