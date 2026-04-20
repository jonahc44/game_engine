#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "lua.hpp"
#include "LuaBridge.h"

struct EventData {
    std::string type;
    luabridge::LuaRef component;
    luabridge::LuaRef function;

    EventData(std::string t, luabridge::LuaRef c, luabridge::LuaRef f) 
        : type(t), component(c), function(f) {}
};

class EventManager {
public:
    static std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> subscriptions;
    
    static std::vector<EventData> pending_subscriptions;
    static std::vector<EventData> pending_unsubscriptions;

    static void Publish(std::string event_type, luabridge::LuaRef event_object);
    static void Subscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function);
    static void Unsubscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function);

    static void ProcessSubscriptions();
    static void Clear();
};

#endif