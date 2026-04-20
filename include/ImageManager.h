#ifndef IMAGE_H
#define IMAGE_H

#include "SDL2_image/SDL_image.h"
#include "SDL.h"
#include "SDL_render.h"
#include "glm/glm.hpp"
#include <filesystem>
#include <vector>
#include <unordered_map>

struct UIDrawRequest {
    std::string image_name;
    int x, y;
    int rotation;
    float scale_x, scale_y;
    float pivot_x, pivot_y;
    int r, g, b, a;
    int sorting_order;
};

struct ImageDrawRequest {
    std::string image_name;
    float x, y;
    int rotation;
    float scale_x, scale_y;
    float pivot_x, pivot_y;
    int r, g, b, a;
    int sorting_order;
};

struct PixelDrawRequest {
    int x, y;
    int r, g, b, a;
};

struct FluidDrawRequest {
    std::string image_name;
    std::vector<Uint32> pixel_buffer;
};

class ImageManager {
    private:
        static inline SDL_Renderer* renderer = nullptr;
        static inline const std::filesystem::path image_dir = "resources/images/";
        static inline std::vector<ImageDrawRequest> scene_requests;
        static inline std::vector<UIDrawRequest> ui_requests;
        static inline std::vector<PixelDrawRequest> pixel_requests;
        static inline std::vector<FluidDrawRequest> fluid_requests;
        static inline std::unordered_map<std::string, SDL_Texture*> texture_cache;
        static SDL_Texture* LoadTexture(const std::string& img_name);

    public:
        static void Init(SDL_Renderer* rec_renderer) { renderer = rec_renderer; }
        void load();
        static void CreateStreamingTexture(const std::string& texture_name, int grid_width, int grid_height);
        static void UpdateStreamingTexture(const std::string& texture_name, const std::vector<Uint32>& pixel_buffer, int grid_width, int grid_height);
        static void Draw(const std::string& image_name, float x, float y);
        static void DrawEx(const std::string& image_name, float x, float y, float rotation, float scale_x, float scale_y, float pivot_x, float pivot_y, 
            float r, float g, float b, float a, float sorting_order);
        static void DrawUI(const std::string& image_name, float x, float y);
        static void DrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a, float sorting_order);
        static void DrawPixel(float x, float y, float r, float g, float b, float a);
        static void RenderScene(float cam_pos_x, float cam_pos_y, int cam_w, int cam_h, float zoom_factor);
        static void RenderUi();
        static void RenderPixels();
        static void RenderFluids();
        static void CreateDefaultParticle();
        static void CreateDefaultFluid(uint width, uint height);
        static glm::vec2 GetWindowSize();
};

#endif