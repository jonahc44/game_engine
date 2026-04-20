#ifndef COMPONENTMANAGER_H
#define COMPONENTMANAGER_H

#include "lua.hpp"
#include "LuaBridge.h"
#include "rapidjson/document.h"
#include <filesystem>
#include <vector>

class ComponentManager {
    private:
        const std::filesystem::path comp_path = "resources/component_types/";
        // std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> components;

    public:
        static inline ComponentManager* instance = nullptr;
        lua_State* lua_state;
        std::vector<std::shared_ptr<luabridge::LuaRef>> components;

        ComponentManager();
        luabridge::LuaRef CreateComponent(const std::string&);
        void ModifyComponent(luabridge::LuaRef&, rapidjson::Value& json_comp);
        void EstablishInheritance(luabridge::LuaRef& intance_table, luabridge::LuaRef& parent_table);

        ComponentManager(const ComponentManager&) = delete;
        ComponentManager& operator=(const ComponentManager&) = delete;
};

#endif