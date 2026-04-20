#ifndef PHYSICS_MANAGER_H
#define PHYSICS_MANAGER_H

#include "ContactListener.h"
#include "ActorManager.h"
#include "box2d/box2d.h"
#include <algorithm>

extern lua_State* G_LuaState;

struct HitResult {
    Actor* actor = nullptr;
    b2Vec2 point = {0.0f, 0.0f};
    b2Vec2 normal = {0.0f, 0.0f};
    bool is_trigger = false;
};

class RaycastClosestCallback : public b2RayCastCallback {
public:
    bool hit = false;
    HitResult result;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
        
        if (!actor) return -1.0f; 

        if (fixture->GetFilterData().categoryBits == 0x0004) {
            return -1.0f;
        }

        if (fraction == 0.0f) {
            return -1.0f;
        }

        hit = true;
        result.actor = actor;
        result.point = point;
        result.normal = normal;
        result.is_trigger = fixture->IsSensor();

        return fraction;
    }
};

class RaycastAllCallback : public b2RayCastCallback {
public:
    struct HitData {
        HitResult result;
        float fraction;
    };
    std::vector<HitData> hits;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
        
        if (!actor) return -1.0f;
        if (fixture->GetFilterData().categoryBits == 0x0004) return -1.0f;
        if (fraction == 0.0f) return -1.0f;

        HitResult res;
        res.actor = actor;
        res.point = point;
        res.normal = normal;
        res.is_trigger = fixture->IsSensor();

        hits.push_back({res, fraction});
        return 1.0f;
    }
};

class PhysicsManager {
public:
    static inline b2World* world = nullptr;
    static inline bool hasRigidbodyExisted = false;
    static inline ContactListener contact_listener;

    static void InitializeWorld() {
        if (!world && hasRigidbodyExisted) {
            b2Vec2 gravity(0.0f, 9.8f);
            world = new b2World(gravity);
            world->SetContactListener(&contact_listener);
        }
    }

    static void Step() {
        if (world) {
            world->Step(1.0f / 60.0f, 8, 3);
        }
    }

    static void CleanUp() {
        if (world) {
            delete world;
            world = nullptr;
        }
    }
};

// b2World* PhysicsManager::world = nullptr;
// bool PhysicsManager::hasRigidbodyExisted = false;
#endif