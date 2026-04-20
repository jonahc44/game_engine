#include "glm/glm.hpp"
#include <memory>
#include <string>
#include <unordered_set>
#include <optional>
#include "SDL_rect.h"
#include "SDL_render.h"
#include "lua.hpp"
#include "LuaBridge.h"

#include "Rigidbody.h"

struct ComponentWrapper {
    std::shared_ptr<luabridge::LuaRef> table;
    bool has_started = false;
    bool has_update = false;
    bool has_late_update = false;
	bool has_on_destroy = false;
};

struct Collision {
	Actor* other;
	b2Vec2 point;
	b2Vec2 relative_velocity;
	b2Vec2 normal = { -999.0f,-999.0f };
};

class Actor
{
public:
	std::string name;
	std::map<std::string, ComponentWrapper> components;
	std::vector<std::string> components_to_remove;
    std::unordered_map<std::string, ComponentWrapper> components_to_add;
	int hp = -1;
	int score = -1;
	glm::vec2 position;
	glm::vec2 scale;
	glm::vec2 view_pivot_offset;
	float rotation;
	glm::vec2 velocity;
	std::string nearby_str;
	std::string contact_str;
	SDL_Texture* nearby_dialogue;
	SDL_Texture* contact_dialogue;
	uint32_t uid;
	int next_on_tile = -1;
	SDL_FRect rect;
	SDL_Texture* texture_front = nullptr;
	SDL_Texture* texture_back = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Texture* damage_texture = nullptr;
	SDL_Texture* texture_to_render = nullptr;
	SDL_FPoint center;
	SDL_RendererFlip flip_flags = SDL_FLIP_NONE;
	std::optional<int> render_order;
	bool prev_x_neg = false;
	bool prev_y_neg = false;
	bool movement_bounce = false;
	float bounce_y;
	float box_collider_width = 0.0f;
	float box_collider_height = 0.0f;
	float box_trigger_width = 0.0f;
	float box_trigger_height = 0.0f;
	float collide_left, collide_right, collide_top, collide_bottom = -1.0f;
	float trigger_left, trigger_right, trigger_top, trigger_bottom = -1.0f;
	bool collider = false;
	bool trigger = false;
	bool nearby_sfx_played = false;
	std::unordered_set<Actor*> colliding_actors;
	int last_dmg_animation = -30;
	bool is_destroyed = false;
	bool dont_destroy = false;
	lua_State* lua_state = nullptr;
	Rigidbody rb;

	Actor(std::string name, glm::vec2 position, glm::vec2 scale,
		float rotation, glm::vec2 initial_velocity, std::string nearby_str, std::string contact_str,
		SDL_Texture* nearby_dialogue, SDL_Texture* contact_dialogue, uint32_t uid)
		: name(name), position(position), scale(scale), 
		rotation(rotation), velocity(initial_velocity), nearby_str(nearby_str), contact_str(contact_str), 
		nearby_dialogue(nearby_dialogue), contact_dialogue(contact_dialogue), uid(uid) {
	}

	Actor() {}

    luabridge::LuaRef AddComponent(const std::string& type_name);
    void RemoveComponent(luabridge::LuaRef component_ref);
	void Destroy();

	std::string GetName() const { return name; }
    int GetID() const { return uid; }

	luabridge::LuaRef GetComponentByKey(const std::string& key);
	luabridge::LuaRef GetComponent(const std::string& type_name);
	luabridge::LuaRef GetComponents(const std::string& type_name);
};