#ifndef SCENE_H
#define SCENE_H

#include <string>
#include <unordered_map>
#include <memory>
#include "SDL_render.h"
#include "rapidjson/document.h"
#include "ActorManager.h"
#include "ImageManager.h"
#include "TextManager.h"
#include "AudioManager.h"
#include "ComponentManager.h"

class SceneManager
{
	private:
		rapidjson::Document scene_doc;

    public:
		// Actor* player = nullptr;
		std::unordered_map<uint64_t, int> actor_map;
		int intro_idx = 0;
    	std::unique_ptr<ActorManager> actor_manager;
        // std::unique_ptr<TextManager> text_manager;
		std::unique_ptr<AudioManager> audio_manager;

		bool in_intro = false;
		SDL_Renderer* renderer;
		SDL_Texture* hp_texture;
		SDL_Texture* score_texture;

		void Init(SDL_Renderer* renderer, ComponentManager& component_manger);
        void Load(std::string& scene, rapidjson::Document& game_doc, ComponentManager& component_manger);
		void SceneTransition(std::string& scene);
		void GenerateActorMaps();
		void UpdateActorPosition(Actor* actor, const glm::vec2& new_pos, const int& idx);
		uint64_t PackCoords(long long x, long long y);
		bool NextSlide();
		void RenderIntro();
		void RenderGameOverBad(bool);
		void RenderGameOverGood(bool);
		void StartGameplay();
		void CleanupIntro();

		static inline std::string current_scene = "";
    	static inline std::string next_scene = "";
		static void Load(const std::string& scene_name);
		static std::string GetCurrent();
		static void DontDestroy(Actor* actor);
};

#endif