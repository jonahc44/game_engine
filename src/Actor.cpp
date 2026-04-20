#include "Actor.h"
#include "ComponentManager.h"
#include <algorithm>

luabridge::LuaRef Actor::AddComponent(const std::string& type_name) {
    static int runtime_comp_count = 0;
    std::string key = "r" + std::to_string(runtime_comp_count++);
    luabridge::LuaRef new_comp = ComponentManager::instance->CreateComponent(type_name);

    if (type_name == "Rigidbody") {
        Rigidbody* new_rb = new Rigidbody();
        new_comp = luabridge::LuaRef(ComponentManager::instance->lua_state, new_rb);
    }

    
    new_comp["key"] = key;
    new_comp["type"] = type_name;
    new_comp["actor"] = this;
    new_comp["enabled"] = true;
    new_comp["has_started"] = false;

    ComponentWrapper wrapper;
    wrapper.table = std::make_shared<luabridge::LuaRef>(new_comp);
    wrapper.has_update = new_comp["OnUpdate"].isFunction();
    wrapper.has_late_update = new_comp["OnLateUpdate"].isFunction();
    wrapper.has_started = !new_comp["OnStart"].isFunction();
    wrapper.has_on_destroy = new_comp["OnDestroy"].isFunction();

    components_to_add[key] = wrapper;
    return new_comp;
}

void Actor::RemoveComponent(luabridge::LuaRef component_ref) {
    component_ref["enabled"] = false;

    if (component_ref["key"].isString()) {
        components_to_remove.push_back(component_ref["key"].cast<std::string>());
    }
}

luabridge::LuaRef Actor::GetComponentByKey(const std::string& key) {
    if (std::find(components_to_remove.begin(), components_to_remove.end(), key) != components_to_remove.end()) {
        return luabridge::LuaRef(lua_state);
    }

    if (components.count(key) > 0) {
        return *(components[key].table);
    }

    return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef Actor::GetComponent(const std::string& type_name) {
    for (auto const& [key, wrapper] : components) {
        if (std::find(components_to_remove.begin(), components_to_remove.end(), key) != components_to_remove.end()) continue;

        luabridge::LuaRef type_ref = (*wrapper.table)["type"];
        if (type_ref.isString() && type_ref.cast<std::string>() == type_name) {
            return *wrapper.table;
        }
    }
    return luabridge::LuaRef(lua_state);
}

luabridge::LuaRef Actor::GetComponents(const std::string& type_name) {
    luabridge::LuaRef table = luabridge::newTable(lua_state);
    int index = 1;
    
    for (auto const& [key, wrapper] : components) {
        luabridge::LuaRef type_ref = (*wrapper.table)["type"];
        if (type_ref.isString() && type_ref.cast<std::string>() == type_name) {
            table[index++] = *wrapper.table;
        }
    }
    return table;
}

// void Actor::Destroy() {
//     for (auto& [key, comp_ptr] : components) {
//         (*comp_ptr.table)["enabled"] = false;
//     }
//     is_destroyed = true;
// }