#ifndef ACTOR_H
#define ACTOR_H

#include <cstdint>
#include <string>
#include <vector>
#include "SDL_render.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "lua.hpp"
#include "LuaBridge.h"

#include "AudioManager.h"
#include "ImageManager.h"
#include "TextManager.h"
#include "ComponentManager.h"
#include <Actor.h>

class ActorManager
{
    public:
        uint32_t max_uid = 1;
        std::map<std::string, Actor*> template_map;
        std::vector<Actor*> actors;
		std::vector<Actor*> actors_to_add;
		static ActorManager* instance;

		ActorManager(AudioManager& audio_manager, ComponentManager& component_manager);
        Actor* LoadActor(rapidjson::GenericValue<rapidjson::UTF8<>>& actor_obj, AudioManager& audio_manager, ComponentManager& component_manager, bool temp_load);
		Actor* Instantiate(const std::string& template_name);
		void FlipHorizontal(Actor* actor);
		void FlipVertical(Actor* actor);
		void EndFrame();
};

#endif