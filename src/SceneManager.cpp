#include "SceneManager.h"
#include "ActorManager.h"
#include "AudioManager.h"
#include "EngineUtils.h"
#include "SDL_render.h"
#include "TextManager.h"
#include "glm/fwd.hpp"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

void SceneManager::Init(SDL_Renderer* rec_renderer, ComponentManager& comp_manager) {
    renderer = rec_renderer;
    // text_manager = std::make_unique<TextManager>();
    audio_manager = std::make_unique<AudioManager>();
    actor_manager = std::make_unique<ActorManager>(*audio_manager, comp_manager);
}

void SceneManager::Load(const std::string& scene_name) {
    next_scene = scene_name; 
}

std::string SceneManager::GetCurrent() {
    return current_scene;
}

void SceneManager::DontDestroy(Actor* actor) {
    if (actor) {
        actor->dont_destroy = true;
    }
}

uint64_t SceneManager::PackCoords(long long x, long long y) {
    return (y << 32) | (x & 0xFFFFFFFF);
}

void SceneManager::GenerateActorMaps() {
    actor_map.clear();

    for (size_t i = 0; i < actor_manager->actors.size(); i++) {
        Actor* a = actor_manager->actors[i];
        a->next_on_tile = -1;
        uint64_t idx = PackCoords(a->position.x, a->position.y);
        
        if (actor_map.find(idx) == actor_map.end()) {
            actor_map[idx] = i;
        } else {
            a->next_on_tile = actor_map[idx];
            actor_map[idx] = i;
        }
    }
}

void SceneManager::UpdateActorPosition(Actor* actor, const glm::vec2& new_pos, const int& idx) {
    uint64_t old_pos = PackCoords(actor->position.x, actor->position.y);
    auto it = actor_map.find(old_pos);
    
    if (it != actor_map.end()) {
        int current_idx = it->second;
        int prev_idx = -1;

        while (current_idx != -1) {
            if (current_idx == idx) {
                Actor* current = actor_manager->actors[current_idx];
                
                if (prev_idx == -1) {
                    if (current->next_on_tile == -1) {
                        actor_map.erase(it);
                    } else {
                        it->second = current->next_on_tile;
                    }
                } else {
                    Actor* prev = actor_manager->actors[prev_idx];
                    prev->next_on_tile = current->next_on_tile;
                }
                current->next_on_tile = -1; 
                break;
            }
            prev_idx = current_idx;
            current_idx = actor_manager->actors[current_idx]->next_on_tile;
        }
    }

    actor->position = new_pos;
    uint64_t new_pos_packed = PackCoords(new_pos.x, new_pos.y);
    int old_head = -1;
    
    auto it_new = actor_map.find(new_pos_packed);
    if (it_new != actor_map.end()) {
        old_head = it_new->second;
    }

    actor->next_on_tile = old_head;        
    actor_map[new_pos_packed] = idx;
}


void SceneManager::Load(std::string& scene, rapidjson::Document& game_doc, ComponentManager& component_manger) {
    fs::path scene_path = "resources/scenes/" + scene + ".scene";

    if (!fs::exists(scene_path) || fs::is_directory(scene_path)) {
        std::cout << "error: scene " << scene << " is missing";
        exit(0);
    }

    ReadJsonFile(scene_path.string(), scene_doc);

    if (scene_doc.HasMember("actors") && scene_doc["actors"].IsArray()) {
        long long x_min, y_min, x_max, y_max;

        for (auto& actor_obj : scene_doc["actors"].GetArray()) {
            Actor* new_actor = actor_manager->LoadActor(actor_obj, *audio_manager, component_manger, false);

            actor_manager->actors.push_back(new_actor);

            // Get map bounds
            if (actor_manager->actors.size() == 1) {
                x_min = x_max = new_actor->position.x;
                y_min = y_max = new_actor->position.y;
            } else {
                if (new_actor->position.x < x_min) {
                    x_min = new_actor->position.x;
                } else if (new_actor->position.x > x_max) {
                    x_max = new_actor->position.x;
                }

                if (new_actor->position.y < y_min) {
                    y_min = new_actor->position.y;
                } else if (new_actor->position.y > y_max) {
                    y_max = new_actor->position.y;
                }
            }  
        }
    }

    GenerateActorMaps();

    if (!in_intro) {
        StartGameplay();
    }
}

// bool SceneManager::NextSlide() {
//     intro_idx++;

//     if (image_manager->NextSlide(intro_idx) || text_manager->NextSlide(intro_idx)) {
//         return true; 
//     }
//     return false;
// }

// void SceneManager::RenderIntro() {
//     image_manager->RenderIntro(renderer, intro_idx);
//     text_manager->RenderIntro(renderer, intro_idx);
// }

// void SceneManager::RenderGameOverBad(bool at_end) {
//     if (!at_end) audio_manager->CleanupBgm();
//     image_manager->RenderGameOverBad(renderer);
//     if (!at_end) audio_manager->PlayEnd(true);
// }

// void SceneManager::RenderGameOverGood(bool at_end) {
//     if (!at_end) audio_manager->CleanupBgm();
//     image_manager->RenderGameOverGood(renderer);
//     if (!at_end) audio_manager->PlayEnd(false);
// }

void SceneManager::StartGameplay() {
    audio_manager->PlayBgm();
}

// void SceneManager::CleanupIntro() {
//     image_manager->CleanupIntro();
//     text_manager->CleanupIntro();
//     audio_manager->CleanupBgm();
//     StartGameplay();
// }