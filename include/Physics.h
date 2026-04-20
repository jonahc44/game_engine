#include "PhysicsManger.h"

class Physics {
public:
    static luabridge::LuaRef Raycast(b2Vec2 pos, b2Vec2 dir, float dist) {
        luabridge::LuaRef nil_ref(G_LuaState);

        if (dist <= 0.0f || !PhysicsManager::world) {
            return nil_ref; 
        }

        dir.Normalize();
        b2Vec2 end_pos = pos + (dist * dir);

        RaycastClosestCallback callback;
        PhysicsManager::world->RayCast(&callback, pos, end_pos);

        if (callback.hit) {
            return luabridge::LuaRef(G_LuaState, callback.result);
        }

        return nil_ref;
    }

    static luabridge::LuaRef RaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
        luabridge::LuaRef table = luabridge::newTable(G_LuaState);
        
        if (dist <= 0.0f || !PhysicsManager::world) {
            return table; 
        }

        dir.Normalize();
        b2Vec2 end_pos = pos + (dist * dir);

        RaycastAllCallback callback;
        PhysicsManager::world->RayCast(&callback, pos, end_pos);

        if (callback.hits.empty()) {
            return table;
        }

        std::sort(callback.hits.begin(), callback.hits.end(), 
            [](const RaycastAllCallback::HitData& a, const RaycastAllCallback::HitData& b) {
                return a.fraction < b.fraction;
            });

        int index = 1;
        for (const auto& h : callback.hits) {
            table[index++] = h.result;
        }

        return table;
    }
};