#pragma once

#include "ComponentManager.h"
#include "SceneManager.h"
#include "TextManager.h"
#include "SDL_render.h"
#include "SDL_video.h"
#include "Camera.h"
#include "PhysicsManger.h"

#include "rapidjson/document.h"
#include <cstdint>
#include <set>
#include <sys/types.h>

class Engine {
    private:
        ComponentManager component_manager;
        SceneManager scene_manager;
        TextManager text_manager;
        Camera camera;
        // PhysicsManager physics_manager;

        bool debug_mode;
        bool victory;
        bool game_over;
        glm::vec2 cam_res;
        // glm::vec2 cam_midpoint;
        std::string frame_buffer;
        rapidjson::Document game_doc;
        rapidjson::Document cam_doc;
        int x_resolution, y_resolution;
        int prev_score = 0;
        
        float player_movement_speed = 0.02f;
        bool x_actor_flip_on_movement = false;
        std::vector<Actor*> trigger_npcs;
        std::vector<SDL_Texture*> dialogue_textures;
        bool initial_render = true;
        uint8_t color_r, color_g, color_b = 255;

    public:
        const uint8_t DEFAULT_CAMERA_WIDTH = 13;
        const uint8_t DEFAULT_CAMERA_HEIGHT = 9;
        const uint16_t DEFAULT_WINDOW_WIDTH = 640;
        const uint16_t DEFAULT_WINDOW_HEIGHT = 360;
        const uint8_t HP_COOLDOWN = 180;
        const uint8_t DMG_ANIMATION_COOLDOWN = 30;
        int last_hp_down = -180;
        std::set<int> rewarded_npc_indices;
        SDL_Window* window;
        SDL_Renderer* renderer;

        Engine(bool debug_mode = false);
        void GameLoop();
        void PlayerMove(const int& idx);
        void ActorsMove();
        bool CheckCollision(const glm::vec2 pos, Actor* curr_actor);
        void CheckTrigger();

        void ProcessDialogue();
        void RenderDialogue();
        void CommandCheck(const std::string& input, const int i);
        void ScoreUp(Actor* npc);
        void take_damage(Actor* npc);
        bool SceneTransitionCheck(const std::string& input);
        void Render();
        void RenderHud();
        void PrintPlayerState();
};