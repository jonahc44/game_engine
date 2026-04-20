#include "EventManager.h"
#include "EngineUtils.h"
#include <algorithm>
#include <iostream>

std::unordered_map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> EventManager::subscriptions;
std::vector<EventData> EventManager::pending_subscriptions;
std::vector<EventData> EventManager::pending_unsubscriptions;

void EventManager::Publish(std::string event_type, luabridge::LuaRef event_object) {
    if (subscriptions.count(event_type) == 0) return;

    for (auto& pair : subscriptions[event_type]) {
        pair.second(pair.first, event_object);
        // try {
        //     pair.second(pair.first, event_object); 
        // } catch (const luabridge::LuaException& e) {
        //     ReportError();
        // }
    }
}

void EventManager::Subscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    pending_subscriptions.push_back(EventData(event_type, component, function));
}

void EventManager::Unsubscribe(std::string event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    pending_unsubscriptions.push_back(EventData(event_type, component, function));
}

void EventManager::ProcessSubscriptions() {
    for (const auto& request : pending_unsubscriptions) {
        auto& subs = subscriptions[request.type];
        
        subs.erase(std::remove_if(subs.begin(), subs.end(), 
            [&](const std::pair<luabridge::LuaRef, luabridge::LuaRef>& pair) {
                return pair.first == request.component && pair.second == request.function;
            }), 
        subs.end());
    }
    pending_unsubscriptions.clear();

    for (const auto& request : pending_subscriptions) {
        subscriptions[request.type].push_back({request.component, request.function});
    }
    pending_subscriptions.clear();
}

void EventManager::Clear() {
    subscriptions.clear();
    pending_subscriptions.clear();
    pending_unsubscriptions.clear();
}