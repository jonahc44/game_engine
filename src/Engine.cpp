#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <string>

#include "EngineUtils.h"
#include "Helper.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "lua.hpp"
#include "LuaBridge.h"
#include "glm/fwd.hpp"
#include "ActorManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "EventManager.h"
#include "Engine.h"

namespace fs = std::filesystem;

Engine::Engine(bool debug_mode) :
    debug_mode(debug_mode),
    victory(false),
    game_over(false) {
    
    // Check for resources directory then resources/game.config on launch
    fs::path base_path = "resources";
    if (!fs::exists(base_path) || !fs::is_directory(base_path)) {
        std::cout << "error: resources/ missing";
        exit(0);
    }

    fs::path game_config_path = base_path / "game.config";
    if (!fs::exists(game_config_path) || fs::is_directory(game_config_path)) {
        std::cout << "error: resources/game.config missing";
        exit(0);
    }

    ReadJsonFile(game_config_path.string(), game_doc);

    // Initial scene check
    if (!game_doc.HasMember("initial_scene") || !game_doc["initial_scene"].IsString()) {
        std:: cout << "error: initial_scene unspecified";
        exit(0);
    }

    if (game_doc.HasMember("player_movement_speed")) {
        player_movement_speed = game_doc["player_movement_speed"].GetFloat();
    }

    // Camera and SDL2 window/renderer setup
    const char* game_title = "";
    if (game_doc.HasMember("game_title") && game_doc["game_title"].IsString()) {
        game_title = game_doc["game_title"].GetString();
    }

    fs::path cam_config_path = base_path / "rendering.config";

    if (fs::exists(cam_config_path) && !fs::is_directory(cam_config_path)) {
        ReadJsonFile(cam_config_path.string(), cam_doc);
        
        if (cam_doc.HasMember("x_resolution") && cam_doc["x_resolution"].IsInt()) {
            cam_res.x = x_resolution = cam_doc["x_resolution"].GetInt();
        } else {
            cam_res.x = DEFAULT_CAMERA_WIDTH;
            x_resolution = DEFAULT_WINDOW_WIDTH;
        }

        if (cam_doc.HasMember("y_resolution") && cam_doc["y_resolution"].IsInt()) {
            cam_res.y = y_resolution = cam_doc["y_resolution"].GetInt();
        } else {
            cam_res.y = DEFAULT_CAMERA_HEIGHT;
            y_resolution = DEFAULT_WINDOW_HEIGHT;
        }

        if (cam_doc.HasMember("cam_offset_x")) {
            camera.cam_offset.x = cam_doc["cam_offset_x"].GetFloat() * 100.0f;
        }

        // Background colors
        if (cam_doc.HasMember("clear_color_r") && cam_doc["clear_color_r"].IsInt()) {
            color_r = cam_doc["clear_color_r"].GetInt();
        }

        if (cam_doc.HasMember("clear_color_g") && cam_doc["clear_color_g"].IsInt()) {
            color_g = cam_doc["clear_color_g"].GetInt();
        }

        if (cam_doc.HasMember("clear_color_b") && cam_doc["clear_color_b"].IsInt()) {
            color_b = cam_doc["clear_color_b"].GetInt();
        }

        if (cam_doc.HasMember("zoom_factor")) {
            camera.zoom_factor = cam_doc["zoom_factor"].GetFloat();
        }

        if (cam_doc.HasMember("cam_ease_factor")) {
            camera.cam_ease_factor = cam_doc["cam_ease_factor"].GetFloat();
        }
    } else {
        cam_res.x = DEFAULT_CAMERA_WIDTH;
        cam_res.y = DEFAULT_CAMERA_HEIGHT;
        x_resolution = DEFAULT_WINDOW_WIDTH;
        y_resolution = DEFAULT_WINDOW_HEIGHT;
    }

    window = Helper::SDL_CreateWindow(
        game_title, 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        x_resolution, 
        y_resolution, 
        SDL_WINDOW_SHOWN
    );

    if (window == nullptr) {
        std::cout << "Window could not be created! SDL error: " << SDL_GetError() << '\n';
        SDL_Quit();
        exit(0);
    }

    renderer = Helper::SDL_CreateRenderer(
        window, 
        -1, 
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    ImageManager::Init(renderer);

    // SDL_GetRendererOutputSize(renderer, &x_resolution, &y_resolution);
    SDL_RenderSetLogicalSize(renderer, x_resolution, y_resolution);

    // Initial render
    SDL_SetRenderDrawColor(renderer, color_r, color_g, color_b, 255);
    SDL_RenderClear(renderer);

    // Load intro
    scene_manager.Init(renderer, component_manager);
    std::string scene = game_doc["initial_scene"].GetString();
    scene_manager.current_scene = scene;
    scene_manager.Load(scene, game_doc, component_manager);
}

bool Engine::SceneTransitionCheck(const std::string& input) {
    const std::string phrase = "proceed to";
    size_t pos = input.find(phrase);
    if (pos == std::string::npos) return false;

    pos += phrase.length();
    while (pos < input.size() && std::isspace(input[pos])) {
        ++pos;
    }

    if (pos == input.size()) return false;

    size_t end_pos = pos;
    while (end_pos < input.size() && !std::isspace(input[end_pos])) {
        ++end_pos;
    }

    std::string scene = input.substr(pos, end_pos - pos);
    SDL_RenderClear(renderer);
    scene_manager.Load(scene, game_doc,component_manager);
    rewarded_npc_indices.clear();

    Render();
    return true;
}

void Engine::Render() {
    static std::vector<Actor*> render_queue; 
    render_queue = scene_manager.actor_manager->actors;

    std::sort(render_queue.begin(), render_queue.end(), [](const Actor* a, const Actor* b) {
        float sort_val_a = a->render_order.has_value() ? static_cast<float>(a->render_order.value()) : a->position.y;
        float sort_val_b = b->render_order.has_value() ? static_cast<float>(b->render_order.value()) : b->position.y;

        if (glm::abs(sort_val_a - sort_val_b) > 0.0001f) {
            return sort_val_a < sort_val_b;
        }

        return a->uid < b->uid;
    });

    ImageManager::RenderScene(camera.cam_pos.x, camera.cam_pos.y, x_resolution, y_resolution, camera.zoom_factor);
    ImageManager::RenderUi();
    text_manager.RenderAll(renderer);
    ImageManager::RenderPixels();
}

void Engine::RenderHud() {
    float tex_w = 0;
    float tex_h = 0;
    Helper::SDL_QueryTexture(scene_manager.hp_texture, &tex_w, &tex_h);
    SDL_FRect rect = {
        5.0f,
        25.0f,
        tex_w,
        tex_h
    };

    Helper::SDL_QueryTexture(scene_manager.score_texture, &tex_w, &tex_h);
    rect = {
        5.0f,
        5.0f,
        tex_w,
        tex_h
    };

    Helper::SDL_RenderCopy(renderer, scene_manager.score_texture, NULL, &rect);
}

bool Engine::CheckCollision(const glm::vec2 pos, Actor* curr_actor) {
    if (!curr_actor->collider) return false;

    bool collision_detected = false;

    float curr_left = pos.x + curr_actor->collide_left;
    float curr_right = pos.x + curr_actor->collide_right;
    float curr_top = pos.y + curr_actor->collide_top;
    float curr_bottom = pos.y + curr_actor->collide_bottom;

    for (Actor* actor : scene_manager.actor_manager->actors) {
        if (actor == curr_actor || !actor->collider) {
            continue;
        }

        float world_left = actor->position.x + actor->collide_left;
        float world_right = actor->position.x + actor->collide_right;
        float world_top = actor->position.y + actor->collide_top;
        float world_bottom = actor->position.y + actor->collide_bottom;

        if (curr_left < world_right && curr_right > world_left) {
            if (curr_top < world_bottom && curr_bottom > world_top) {
                collision_detected = true;

                curr_actor->colliding_actors.insert(actor);
                actor->colliding_actors.insert(curr_actor);
            }
        }
    }

    return collision_detected;
}

void Engine::RenderDialogue() {
    for (size_t i = 0; i < dialogue_textures.size(); i++) {
        SDL_Texture* texture = dialogue_textures[i];
        float tex_w = 0;
        float tex_h = 0;

        if (texture != nullptr) {
            Helper::SDL_QueryTexture(texture, &tex_w, &tex_h);
        } else {
            continue;
        }

        SDL_FRect rect = {
            25.0f,
            (float)y_resolution - 50.0f - (dialogue_textures.size() - 1 - i) * 50,
            tex_w,
            tex_h
        };
        
        Helper::SDL_RenderCopy(renderer, texture, NULL, &rect);
    }

    dialogue_textures.clear();
}

void Engine::ActorsMove() {
    int frame = Helper::GetFrameNumber();

    for (uint32_t i = 0; i < (uint32_t)scene_manager.actor_manager->actors.size(); i++) {
        Actor* actor = scene_manager.actor_manager->actors[i];
        // if (actor == p) {
        //     PlayerMove(i);
        //     continue;
        // }

        if (actor->velocity.x != 0 || actor->velocity.y != 0) {
            // Animation
            if (x_actor_flip_on_movement) {
                if ((actor->velocity.x < 0.0f && !actor->prev_x_neg) || (actor->velocity.x > 0.0f && actor->prev_x_neg)) {
                    scene_manager.actor_manager->FlipHorizontal(actor);
                }
            }

            if (actor->texture_back != nullptr) {
                if ((actor->velocity.y < 0.0f && !actor->prev_y_neg) || (actor->velocity.y > 0.0f && actor->prev_y_neg)) {
                    scene_manager.actor_manager->FlipVertical(actor);
                }
            }

            glm::vec2 pos = actor->position + actor->velocity;

            if (actor->movement_bounce) {
                actor->bounce_y = -glm::abs(sin(frame*0.15f) * 10.0f);
            }

            if (!actor->colliding_actors.empty()) {
                actor->velocity *= glm::vec2(-1, -1);
            } else if (!CheckCollision(pos, actor)) {
                scene_manager.UpdateActorPosition(actor, pos, i);
            } else {
                actor->velocity *= glm::vec2(-1, -1);
            }
        } else {
            if (actor->movement_bounce) actor->bounce_y = 0.0f;

            glm::vec2 pos = actor->position + actor->velocity;
            CheckCollision(pos, actor);
        }
    }
}

void Engine::GameLoop() {
    int into_idx = 0;
    bool in_intro = scene_manager.in_intro;
    bool at_end = false;
    Input::Init();
    bool should_quit = false;

    while (true) {
        if (!scene_manager.next_scene.empty()) {
            scene_manager.current_scene = scene_manager.next_scene;
            std::string scene_to_load = scene_manager.next_scene;
            scene_manager.next_scene = "";

            auto& actors = scene_manager.actor_manager->actors;
            actors.erase(std::remove_if(actors.begin(), actors.end(), [](Actor* a) {
                if (!a->dont_destroy) {
                    delete a;
                    return true;
                }
                return false;
            }), actors.end());

            EventManager::Clear();
            scene_manager.Load(scene_to_load, game_doc, component_manager); 

            std::sort(scene_manager.actor_manager->actors.begin(), scene_manager.actor_manager->actors.end(), [](const Actor* a, const Actor* b) {
                return a->uid < b->uid;
            });
        }

        SDL_Event next_event;
        SDL_Scancode PlayerMove = SDL_SCANCODE_UNKNOWN;
        SDL_SetRenderDrawColor(renderer, color_r, color_g, color_b, 255);

        while (Helper::SDL_PollEvent(&next_event)) {
            if (next_event.type == SDL_QUIT) {
                should_quit = true;
            }

            Input::ProcessEvent(next_event);
        }

        size_t actors_this_frame = scene_manager.actor_manager->actors.size();
        for (size_t i = 0; i < actors_this_frame; ++i) {
            Actor* actor = scene_manager.actor_manager->actors[i];
            if (actor->is_destroyed) continue;

            for (auto& [key, wrapper] : actor->components) {
                if (wrapper.has_started) continue;
                luabridge::LuaRef has_started = (*wrapper.table)["has_started"];
                if (has_started.isBool() && has_started.cast<bool>()) continue;
                
                luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
                if (enabled.isBool() && !enabled.cast<bool>()) continue;

                try {
                    (*wrapper.table)["OnStart"](*wrapper.table);
                } catch (const luabridge::LuaException& e) {
                    ReportError(actor->name, e);
                }
                
                (*wrapper.table)["has_started"] = true;
                wrapper.has_started = true;
            }
        }

        actors_this_frame = scene_manager.actor_manager->actors.size(); 
        for (size_t i = 0; i < actors_this_frame; ++i) {
            Actor* actor = scene_manager.actor_manager->actors[i];
            if (actor->is_destroyed) continue;
            for (auto& [key, wrapper] : actor->components) {
                if (!wrapper.has_update) continue;
                luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
                if (enabled.isBool() && !enabled.cast<bool>()) continue;

                try {
                    (*wrapper.table)["OnUpdate"](*wrapper.table);
                } catch (const luabridge::LuaException& e) {
                    ReportError(actor->name, e);
                }
            }
        }

        actors_this_frame = scene_manager.actor_manager->actors.size();
        for (size_t i = 0; i < actors_this_frame; ++i) {
            Actor* actor = scene_manager.actor_manager->actors[i];
            if (actor->is_destroyed) continue;
            for (auto& [key, wrapper] : actor->components) {
                if (!wrapper.has_late_update) continue;
                luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
                if (enabled.isBool() && !enabled.cast<bool>()) continue;

                try {
                    (*wrapper.table)["OnLateUpdate"](*wrapper.table);
                } catch (const luabridge::LuaException& e) {
                    ReportError(actor->name, e);
                }
            }
        }

        actors_this_frame = scene_manager.actor_manager->actors.size();
        for (size_t i = 0; i < actors_this_frame; ++i) {
            Actor* actor = scene_manager.actor_manager->actors[i];

            for (auto& [key, wrapper] : actor->components) {
                if (!wrapper.has_on_destroy) continue;
                bool is_component_removed = std::find(actor->components_to_remove.begin(), actor->components_to_remove.end(), key) != actor->components_to_remove.end();
                if (is_component_removed) {
                    try {
                        (*wrapper.table)["OnDestroy"](*wrapper.table);
                    } catch (const luabridge::LuaException& e) {
                        ReportError(actor->name, e);
                    }
                }
            }
        }

        EventManager::ProcessSubscriptions();
        SDL_RenderClear(renderer);
        PhysicsManager::Step();
        Render();        
        Helper::SDL_RenderPresent(renderer);
        Input::LateUpdate();

        // Component cleanup
        for (Actor* actor : scene_manager.actor_manager->actors) {
            for (const std::string& key : actor->components_to_remove) {
                auto& wrapper = actor->components[key];
                luabridge::LuaRef type_ref = (*wrapper.table)["type"];
                
                if (type_ref.isString() && type_ref.cast<std::string>() == "Rigidbody") {
                    Rigidbody* rb_to_delete = wrapper.table->cast<Rigidbody*>();
                    delete rb_to_delete;
                }

                actor->components.erase(key);
            }
            actor->components_to_remove.clear();

            for (auto& [key, wrapper] : actor->components_to_add) {
                actor->components[key] = wrapper;
            }
            actor->components_to_add.clear();
        }

        auto& actors_to_add = scene_manager.actor_manager->actors_to_add;
        for (Actor* new_actor : actors_to_add) {
            scene_manager.actor_manager->actors.push_back(new_actor);
        }
        actors_to_add.clear();

        auto& actors = scene_manager.actor_manager->actors;
        actors.erase(std::remove_if(actors.begin(), actors.end(), [](Actor* a) {
            if (a->is_destroyed) {
                delete a;
                return true;
            }
            return false;
        }), actors.end());

        scene_manager.actor_manager->EndFrame();

        if (should_quit) {
            SDL_Quit();
            exit(0);
        }
    }
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    bool debug_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "debug") {
            debug_mode = true;
            break;
        }
    }

    Engine game_engine(debug_mode);
    game_engine.GameLoop();

    return 0;
}