#include "ActorManager.h"
#include "AudioManager.h"
#include "EngineUtils.h"
#include "ImageManager.h"
#include "ParticleSystem.h"
#include "FluidSystem.h"
#include "SDL_render.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

ActorManager* ActorManager::instance = nullptr;

Actor* ActorManager::LoadActor(
    rapidjson::GenericValue<rapidjson::UTF8<>>& actor_obj, 
    AudioManager& audio_manager,
    ComponentManager& component_manager,
    bool temp_load=false
) {

    Actor* new_actor = new Actor(
        "",
        glm::vec2(0, 0),
        glm::vec2(1.0f, 1.0f),
        0.0f,
        glm::vec2(0, 0),
        "",
        "",
        nullptr,
        nullptr,
        temp_load ? -1 : max_uid++
    );

    new_actor->lua_state = component_manager.lua_state;

    if (!temp_load && actor_obj.HasMember("template") && actor_obj["template"].IsString()) {
        std::string template_name = actor_obj["template"].GetString();

        if (template_map.count(template_name) == 0) {
            std::cout << "error: template " << template_name << " is missing";
            exit(0);
        }

        int uid = new_actor->uid;
        *new_actor = *template_map[template_name];
        new_actor->uid = uid;

        new_actor->components.clear();

        for (auto& [key, tmpl_wrapper] : template_map[template_name]->components) {
            luabridge::LuaRef comp_type = (*tmpl_wrapper.table)["type"];
            std::string type_str = comp_type.isString() ? comp_type.cast<std::string>() : "";
            
            luabridge::LuaRef new_comp(component_manager.lua_state);

            if (type_str == "Rigidbody") {
                Rigidbody* parent_component = tmpl_wrapper.table->cast<Rigidbody*>();
                Rigidbody* new_rb = new Rigidbody(*parent_component);
                new_comp = luabridge::LuaRef(component_manager.lua_state, new_rb);
            } else if (type_str == "ParticleSystem") {
                ParticleSystem* parent_component = tmpl_wrapper.table->cast<ParticleSystem*>();
                ParticleSystem* new_ps = new ParticleSystem(*parent_component);
                new_comp = luabridge::LuaRef(component_manager.lua_state, new_ps);
            } else if (type_str == "FluidSystem") {
                FluidSystem* parent_component = tmpl_wrapper.table->cast<FluidSystem*>();
                FluidSystem* new_fs = new FluidSystem(*parent_component);
                new_comp = luabridge::LuaRef(component_manager.lua_state, new_fs);
            } else {
                new_comp = luabridge::newTable(component_manager.lua_state);
                component_manager.EstablishInheritance(new_comp, *tmpl_wrapper.table);
            }

            new_comp["key"] = key; 
            if (comp_type.isString()) new_comp["type"] = comp_type;
            
            new_comp["enabled"] = true;
            new_comp["has_started"] = false;
            new_comp["actor"] = new_actor; 
            
            ComponentWrapper wrapper;
            wrapper.table = std::make_shared<luabridge::LuaRef>(new_comp);
            wrapper.has_update = new_comp["OnUpdate"].isFunction();
            wrapper.has_late_update = new_comp["OnLateUpdate"].isFunction();
            wrapper.has_started = !new_comp["OnStart"].isFunction();
            wrapper.has_on_destroy = new_comp["OnDestroy"].isFunction();

            new_actor->components[key] = wrapper;
        }
    }

    if (actor_obj.HasMember("name") && actor_obj["name"].IsString()) {
        new_actor->name = actor_obj["name"].GetString();
    }

    if (actor_obj.HasMember("components")) {
        auto curr_comps = actor_obj["components"].GetObject();
        
        for (auto& comp : curr_comps) {
            std::string key = comp.name.GetString();
            auto& comp_obj = comp.value;

            if (comp_obj.IsObject()) {
                luabridge::LuaRef target_comp = luabridge::newTable(component_manager.lua_state);

                if (new_actor->components.count(key) > 0) {
                    target_comp = *new_actor->components[key].table;
                } else {
                    std::string type = comp_obj["type"].GetString();

                    if (type == "Rigidbody") {
                        Rigidbody* new_rb = new Rigidbody();
                        target_comp = luabridge::LuaRef(component_manager.lua_state, new_rb);
                    } else if (type == "ParticleSystem") {
                        ParticleSystem* new_ps = new ParticleSystem();
                        target_comp = luabridge::LuaRef(component_manager.lua_state, new_ps);
                    } else if (type == "FluidSystem") {
                        FluidSystem* new_fs = new FluidSystem();
                        target_comp = luabridge::LuaRef(component_manager.lua_state, new_fs);
                    } else {
                        target_comp = component_manager.CreateComponent(type);
                    }

                    target_comp["key"] = key;
                    target_comp["type"] = type;
                    target_comp["enabled"] = true;
                    target_comp["has_started"] = false;

                    ComponentWrapper wrapper;
                    wrapper.table = std::make_shared<luabridge::LuaRef>(target_comp);
                    wrapper.has_update = target_comp["OnUpdate"].isFunction();
                    wrapper.has_late_update = target_comp["OnLateUpdate"].isFunction();
                    wrapper.has_started = !target_comp["OnStart"].isFunction();
                    wrapper.has_on_destroy = target_comp["OnDestroy"].isFunction();
                    
                    new_actor->components[key] = wrapper;
                }

                component_manager.ModifyComponent(target_comp, comp_obj);
                target_comp["actor"] = new_actor;
            }
        }
    }

    return new_actor;
}

ActorManager::ActorManager(AudioManager& audio_manager, ComponentManager& component_manager) {
    instance = this;
    fs::path path = "resources/actor_templates";

    if (!fs::exists(path) || !fs::is_directory(path)) return;

    for (const auto& t_file : fs::directory_iterator(path)) {
        rapidjson::Document doc;
        ReadJsonFile(t_file.path().string(), doc);

        std::string name = t_file.path().stem().string();
        template_map[name] = LoadActor(doc, audio_manager, component_manager, true);
    }
}

Actor* ActorManager::Instantiate(const std::string& template_name) {
    if (template_map.count(template_name) == 0) return nullptr;
    
    Actor* new_actor = new Actor("", glm::vec2(0,0), glm::vec2(1,1), 0, glm::vec2(0,0), "", "", nullptr, nullptr, max_uid++);
    *new_actor = *template_map[template_name];
    new_actor->uid = max_uid - 1;
    
    new_actor->components.clear();
    for (auto& [key, tmpl_wrapper] : template_map[template_name]->components) {
        luabridge::LuaRef comp_type = (*tmpl_wrapper.table)["type"];
        std::string type_str = comp_type.isString() ? comp_type.cast<std::string>() : "";
        luabridge::LuaRef new_comp(ComponentManager::instance->lua_state);

        if (type_str == "Rigidbody") {
            Rigidbody* parent_component = tmpl_wrapper.table->cast<Rigidbody*>();
            Rigidbody* new_rb = new Rigidbody(*parent_component);
            new_comp = luabridge::LuaRef(ComponentManager::instance->lua_state, new_rb);
        } else if (type_str == "ParticleSystem") {
            ParticleSystem* parent_component = tmpl_wrapper.table->cast<ParticleSystem*>();
            ParticleSystem* new_ps = new ParticleSystem(*parent_component);
            new_comp = luabridge::LuaRef(ComponentManager::instance->lua_state, new_ps);
        } else if (type_str == "FluidSystem") {
            FluidSystem* parent_component = tmpl_wrapper.table->cast<FluidSystem*>();
            FluidSystem* new_fs = new FluidSystem(*parent_component);
            new_comp = luabridge::LuaRef(ComponentManager::instance->lua_state, new_fs);
        } else {
            new_comp = luabridge::newTable(ComponentManager::instance->lua_state);
            ComponentManager::instance->EstablishInheritance(new_comp, *tmpl_wrapper.table);
        }

        new_comp["key"] = key; 
        new_comp["actor"] = new_actor; 
        new_comp["enabled"] = true;
        new_comp["has_started"] = false;
        
        ComponentWrapper wrapper;
        wrapper.table = std::make_shared<luabridge::LuaRef>(new_comp);
        wrapper.has_update = new_comp["OnUpdate"].isFunction();
        wrapper.has_late_update = new_comp["OnLateUpdate"].isFunction();
        wrapper.has_started = !new_comp["OnStart"].isFunction(); 
        wrapper.has_on_destroy = new_comp["OnDestroy"].isFunction();
        
        new_actor->components[key] = wrapper;
    }
    
    actors_to_add.push_back(new_actor);
    return new_actor;
}
	 
void ActorManager::FlipHorizontal(Actor* actor) {
    if (actor->flip_flags & SDL_FLIP_HORIZONTAL) {
        actor->flip_flags = static_cast<SDL_RendererFlip>(actor->flip_flags & ~SDL_FLIP_HORIZONTAL);
    } else {
        actor->flip_flags = static_cast<SDL_RendererFlip>(actor->flip_flags | SDL_FLIP_HORIZONTAL);
    }
    actor->prev_x_neg = !actor->prev_x_neg;
}

void ActorManager::FlipVertical(Actor* actor) {
    if (actor->prev_y_neg) {
        actor->texture = actor->texture_front;
    } else {
        actor->texture = actor->texture_back;
    }
    actor->prev_y_neg = !actor->prev_y_neg;
}

void ActorManager::EndFrame() {
    for (Actor* actor : actors) {
        actor->colliding_actors.clear();
    }
}