#include <thread>
#include "box2d/box2d.h"

#include "ComponentManager.h"
#include "ActorManager.h"
#include "InputManager.h"
#include "TextManager.h"
#include "AudioManager.h"
#include "Camera.h"
#include "SceneManager.h"
#include "Rigidbody.h"
#include "ParticleSystem.h"
#include "Physics.h"
#include "EventManager.h"
#include "FluidSystem.h"
#include "Helper.h"

lua_State* G_LuaState = nullptr;

namespace {
    void DebugLog(const std::string& message) {
        std::cout << message << '\n';
    }

    Actor* ActorFind(const std::string& name) {
        if (!ActorManager::instance) return nullptr;
        
        for (Actor* actor : ActorManager::instance->actors) {
            if (actor->name == name && !actor->is_destroyed) {
                return actor;
            }
        }

        for (Actor* actor : ActorManager::instance->actors_to_add) {
            if (actor->name == name && !actor->is_destroyed) return actor;
        }

        return nullptr;
    }

    luabridge::LuaRef ActorFindAll(const std::string& name) {
        luabridge::LuaRef table = luabridge::newTable(G_LuaState);
        if (!ActorManager::instance) return table;

        int index = 1;
        for (Actor* actor : ActorManager::instance->actors) {
            if (actor->name == name && !actor->is_destroyed) {
                table[index++] = actor;
            }
        }

        for (Actor* actor : ActorManager::instance->actors_to_add) {
            if (actor->name == name && !actor->is_destroyed) {
                table[index++] = actor;
            }
        }

        return table;
    }

    void AppQuit() {
        exit(0);
    }

    void AppSleep(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    int AppGetFrame() {
        return Helper::GetFrameNumber();
    }

    void AppOpenURL(const std::string& url) {
        std::string command;
        
        #if defined(_WIN32)
            command = "start " + url;
        #elif defined(__APPLE__)
            command = "open " + url;
        #else
            command = "xdg-open " + url;
        #endif
        
        std::system(command.c_str());
    }

    Actor* GlobalInstantiate(const std::string& template_name) {
        return ActorManager::instance->Instantiate(template_name);
    }

    void GlobalDestroy(Actor* actor) {
        if (!actor) return;
        actor->is_destroyed = true;
        for (auto& [key, comp_ptr] : actor->components) {
            (*comp_ptr.table)["enabled"] = false;
            actor->components_to_remove.push_back(key);
        }
    }
}

ComponentManager::ComponentManager() {
    instance = this;
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    G_LuaState = lua_state;

    // Set namespaces & functions
    luabridge::getGlobalNamespace(lua_state)
        .beginNamespace("Debug")
            .addFunction("Log", &DebugLog)
        .endNamespace()
        .beginClass<Actor>("Actor")
            .addFunction("GetName", &Actor::GetName)
            .addFunction("GetID", &Actor::GetID)
            .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
            .addFunction("GetComponent", &Actor::GetComponent)
            .addFunction("GetComponents", &Actor::GetComponents)
            .addFunction("AddComponent", &Actor::AddComponent)
            .addFunction("RemoveComponent", &Actor::RemoveComponent)
            .addStaticFunction("Find", &ActorFind)
            .addStaticFunction("FindAll", &ActorFindAll)
            .addStaticFunction("Instantiate", &GlobalInstantiate)
            .addStaticFunction("Destroy", &GlobalDestroy)
        .endClass()
        .beginClass<b2Vec2>("Vector2")
            .addConstructor<void(*)(float, float)>()
            .addProperty("x", &b2Vec2::x)
            .addProperty("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::operator_add)
            .addFunction("__sub", &b2Vec2::operator_sub)
            .addFunction("__mul", &b2Vec2::operator_mul)
            .addStaticFunction("Distance", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Distance))
            .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
        .endClass()
        .beginClass<Rigidbody>("Rigidbody")
            .addConstructor<void (*) (void)>()
            .addProperty("key", &Rigidbody::key)
            .addProperty("type", &Rigidbody::type)
            .addProperty("enabled", &Rigidbody::enabled)
            .addProperty("actor", &Rigidbody::actor)
            .addProperty("has_started", &Rigidbody::has_started)
            .addProperty("x", &Rigidbody::x)
            .addProperty("y", &Rigidbody::y)
            .addProperty("body_type", &Rigidbody::body_type)
            .addProperty("precise", &Rigidbody::precise)
            .addProperty("gravity_scale", &Rigidbody::gravity_scale)
            .addProperty("density", &Rigidbody::density)
            .addProperty("angular_friction", &Rigidbody::angular_friction)
            .addProperty("rotation", &Rigidbody::rotation)
            .addProperty("has_collider", &Rigidbody::has_collider)
            .addProperty("has_trigger", &Rigidbody::has_trigger)
            .addProperty("collider_type", &Rigidbody::collider_type)
            .addProperty("width", &Rigidbody::width)
            .addProperty("height", &Rigidbody::height)
            .addProperty("radius", &Rigidbody::radius)
            .addProperty("friction", &Rigidbody::friction)
            .addProperty("bounciness", &Rigidbody::bounciness)
            .addProperty("trigger_type", &Rigidbody::trigger_type)
            .addProperty("trigger_width", &Rigidbody::trigger_width)
            .addProperty("trigger_height", &Rigidbody::trigger_height)
            .addProperty("trigger_radius", &Rigidbody::trigger_radius)

            .addFunction("GetPosition", &Rigidbody::GetPosition)
            .addFunction("GetRotation", &Rigidbody::GetRotation)
            .addFunction("GetVelocity", &Rigidbody::GetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
            .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
            .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
            .addFunction("OnStart", &Rigidbody::OnStart)
            .addFunction("AddForce", &Rigidbody::AddForce)
            .addFunction("SetVelocity", &Rigidbody::SetVelocity)
            .addFunction("SetPosition", &Rigidbody::SetPosition)
            .addFunction("SetRotation", &Rigidbody::SetRotation)
            .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
            .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("OnDestroy", &Rigidbody::OnDestroy)
        .endClass()
        .beginClass<ParticleSystem>("ParticleSystem")
            .addProperty("key", &ParticleSystem::key)
            .addProperty("type", &ParticleSystem::type)
            .addProperty("enabled", &ParticleSystem::enabled)
            .addProperty("actor", &ParticleSystem::actor)
            .addProperty("has_started", &ParticleSystem::has_started)
            .addProperty("x", &ParticleSystem::x)
            .addProperty("y", &ParticleSystem::y)
            .addProperty("frames_between_bursts", &ParticleSystem::frames_between_bursts)
            .addProperty("burst_quantity", &ParticleSystem::burst_quantity)
            .addProperty("start_scale_min", &ParticleSystem::start_scale_min)
            .addProperty("start_scale_max", &ParticleSystem::start_scale_max)
            .addProperty("rotation_min", &ParticleSystem::rotation_min)
            .addProperty("rotation_max", &ParticleSystem::rotation_max)
            .addProperty("start_color_r", &ParticleSystem::start_color_r)
            .addProperty("start_color_g", &ParticleSystem::start_color_g)
            .addProperty("start_color_b", &ParticleSystem::start_color_b)
            .addProperty("start_color_a", &ParticleSystem::start_color_a)
            .addProperty("emit_radius_min", &ParticleSystem::emit_radius_min)
            .addProperty("emit_radius_max", &ParticleSystem::emit_radius_max)
            .addProperty("emit_angle_min", &ParticleSystem::emit_angle_min)
            .addProperty("emit_angle_max", &ParticleSystem::emit_angle_max)
            .addProperty("image", &ParticleSystem::image)
            .addProperty("sorting_order", &ParticleSystem::sorting_order)
            .addProperty("duration_frames", &ParticleSystem::duration_frames)
            .addProperty("start_speed_min", &ParticleSystem::start_speed_min)
            .addProperty("start_speed_max", &ParticleSystem::start_speed_max)
            .addProperty("rotation_speed_min", &ParticleSystem::rotation_speed_min)
            .addProperty("rotation_speed_max", &ParticleSystem::rotation_speed_max)
            .addProperty("gravity_scale_x", &ParticleSystem::gravity_scale_x)
            .addProperty("gravity_scale_y", &ParticleSystem::gravity_scale_y)
            .addProperty("drag_factor", &ParticleSystem::drag_factor)
            .addProperty("angular_drag_factor", &ParticleSystem::angular_drag_factor)
            .addProperty("end_scale", &ParticleSystem::end_scale)
            .addProperty("end_color_r", &ParticleSystem::end_color_r)
            .addProperty("end_color_g", &ParticleSystem::end_color_g)
            .addProperty("end_color_b", &ParticleSystem::end_color_b)
            .addProperty("end_color_a", &ParticleSystem::end_color_a)

            .addFunction("OnUpdate", &ParticleSystem::OnUpdate)
            .addFunction("OnStart", &ParticleSystem::OnStart)
            .addFunction("Stop", &ParticleSystem::Stop)
            .addFunction("Play", &ParticleSystem::Play)
            .addFunction("Burst", &ParticleSystem::Burst)

        .endClass()

        .beginClass<FluidSystem>("FluidSystem")
        .addProperty("key", &FluidSystem::key)
            .addProperty("type", &FluidSystem::type)
            .addProperty("enabled", &FluidSystem::enabled)
            .addProperty("actor", &FluidSystem::actor)
            .addProperty("has_started", &FluidSystem::has_started)
            .addProperty("x", &FluidSystem::x)
            .addProperty("y", &FluidSystem::y)

            .addFunction("OnUpdate", &FluidSystem::OnUpdate)
            .addFunction("OnStart", &FluidSystem::OnStart)
            .addFunction("InteractWithRigidbody", &FluidSystem::InteractWithRigidbody)
            .addFunction("ApplyWhirlpool", &FluidSystem::ApplyWhirlpool)
        .endClass()

        .beginClass<Collision>("collision")
            .addProperty("other", &Collision::other)
            .addProperty("point", &Collision::point)
            .addProperty("relative_velocity", &Collision::relative_velocity)
            .addProperty("normal", &Collision::normal)
        .endClass()
        .beginClass<HitResult>("HitResult")
            .addProperty("actor", &HitResult::actor)
            .addProperty("point", &HitResult::point)
            .addProperty("normal", &HitResult::normal)
            .addProperty("is_trigger", &HitResult::is_trigger)
        .endClass()
        .beginNamespace("Physics")
            .addFunction("Raycast", &Physics::Raycast)
            .addFunction("RaycastAll", &Physics::RaycastAll)
        .endNamespace()
        .beginNamespace("Application")
            .addFunction("Quit", &AppQuit)
            .addFunction("Sleep", &AppSleep)
            .addFunction("GetFrame", &AppGetFrame)
            .addFunction("OpenURL", &AppOpenURL)
        .endNamespace()
        .beginClass<glm::vec2>("vec2")
            .addProperty("x", &glm::vec2::x)
            .addProperty("y", &glm::vec2::y)
        .endClass()
        .beginNamespace("Input")
            .addFunction("GetKey", &Input::GetKeyString)
            .addFunction("GetKeyDown", &Input::GetKeyDownString)
            .addFunction("GetKeyUp", &Input::GetKeyUpString)
            
            .addFunction("GetMousePosition", &Input::GetMousePosition)
            .addFunction("GetMouseButton", &Input::GetMouseButton)
            .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
            .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
            
            .addFunction("HideCursor", &Input::HideCursor)
            .addFunction("ShowCursor", &Input::ShowCursor)
        .endNamespace()
        .beginNamespace("Text")
            .addFunction("Draw", &TextManager::Draw)
        .endNamespace()
        .beginNamespace("Audio")
            .addFunction("Play", &AudioManager::Play)
            .addFunction("Halt", &AudioManager::Halt)
            .addFunction("SetVolume", &AudioManager::SetVolume)
        .endNamespace()
        .beginNamespace("Image")
            .addFunction("DrawUI", &ImageManager::DrawUI)
            .addFunction("DrawUIEx", &ImageManager::DrawUIEx)
            .addFunction("Draw", &ImageManager::Draw)
            .addFunction("DrawEx", &ImageManager::DrawEx)
            .addFunction("DrawPixel", &ImageManager::DrawPixel)
            .addFunction("GetWindowSize", &ImageManager::GetWindowSize)
        .endNamespace()
        .beginNamespace("Camera")
            .addFunction("SetPosition", &Camera::SetPosition)
            .addFunction("GetPositionX", &Camera::GetPositionX)
            .addFunction("GetPositionY", &Camera::GetPositionY)
            .addFunction("SetZoom", &Camera::SetZoom)
            .addFunction("GetZoom", &Camera::GetZoom)
        .endNamespace()
        .beginNamespace("Scene")
            .addFunction("Load", &SceneManager::Load)
            .addFunction("GetCurrent", &SceneManager::GetCurrent)
            .addFunction("DontDestroy", &SceneManager::DontDestroy)
        .endNamespace()
        .beginNamespace("Event")
            .addFunction("Publish", &EventManager::Publish)
            .addFunction("Subscribe", &EventManager::Subscribe)
            .addFunction("Unsubscribe", &EventManager::Unsubscribe)
        .endNamespace();

    // Load all components
    if (std::filesystem::exists(comp_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(comp_path)) {
            std::string filepath = entry.path().string();
            std::string component_name = entry.path().stem().string();

            if (luaL_dofile(lua_state, filepath.c_str()) != LUA_OK) {
                std::cout << "problem with lua file " << component_name;
                exit(0);
            }

            luabridge::LuaRef parent_table = luabridge::getGlobal(lua_state, component_name.c_str());

            if (parent_table.isTable()) {
                luabridge::LuaRef instance_table = luabridge::newTable(lua_state);
                EstablishInheritance(instance_table, parent_table);
            }
        }
    }
}

luabridge::LuaRef ComponentManager::CreateComponent(const std::string& component_name) {
    luabridge::LuaRef parent_table = luabridge::getGlobal(lua_state, component_name.c_str());

    if (!parent_table.isTable()) {
        std::cout << "error: failed to locate component " << component_name;
        exit(0);
    }

    luabridge::LuaRef instance_table = luabridge::newTable(lua_state);
    EstablishInheritance(instance_table, parent_table);
    
    return instance_table; 
}

void ComponentManager::ModifyComponent(luabridge::LuaRef& component, rapidjson::Value& json_comp) {
    for (auto it = json_comp.MemberBegin(); it != json_comp.MemberEnd(); ++it) {
        std::string prop_name = it->name.GetString();
        
        if (prop_name == "type") continue; 

        if (it->value.IsString()) {
            component[prop_name] = it->value.GetString();
        } else if (it->value.IsInt()) {
            component[prop_name] = it->value.GetInt();
        } else if (it->value.IsNumber()) {
            component[prop_name] = it->value.GetFloat();
        } else if (it->value.IsBool()) {
            component[prop_name] = it->value.GetBool();
        }
    }
}

void ComponentManager::EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table) {
    luabridge::LuaRef new_metatable = luabridge::newTable(lua_state);
    new_metatable["__index"] = parent_table;

    instance_table.push(lua_state);
    new_metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1);
}