#include "ContactListener.h"
#include "Actor.h"
#include "EngineUtils.h"

void ContactListener::BeginContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetUserData().pointer);
    Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB) return;

    b2Vec2 relative_vel = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();

    bool is_sensor = fixtureA->IsSensor() || fixtureB->IsSensor();
    std::string func_name = is_sensor ? "OnTriggerEnter" : "OnCollisionEnter";

    b2Vec2 point(-999.0f, -999.0f);
    b2Vec2 normal(-999.0f, -999.0f);

    if (!is_sensor) {
        b2WorldManifold worldManifold;
        contact->GetWorldManifold(&worldManifold);
        point = worldManifold.points[0];
        normal = worldManifold.normal;
    }

    Collision collA = { actorB, point, relative_vel, normal };
    Collision collB = { actorA, point, relative_vel, normal };

    std::vector<std::string> keysA;
    for (const auto& pair : actorA->components) {
        keysA.push_back(pair.first);
    }
    std::sort(keysA.begin(), keysA.end());

    for (const std::string& key : keysA) {
        auto& wrapper = actorA->components[key];
        luabridge::LuaRef func = (*wrapper.table)[func_name];
        luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
        
        if (func.isFunction() && enabled.isBool() && enabled.cast<bool>()) {
            try {
                func(*wrapper.table, collA); 
            } catch (const luabridge::LuaException& e) {
                ReportError(actorA->name, e);
            }
        }
    }

    std::vector<std::string> keysB;
    for (const auto& pair : actorB->components) {
        keysB.push_back(pair.first);
    }
    std::sort(keysB.begin(), keysB.end());

    for (const std::string& key : keysB) {
        auto& wrapper = actorB->components[key];
        luabridge::LuaRef func = (*wrapper.table)[func_name];
        luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
        
        if (func.isFunction() && enabled.isBool() && enabled.cast<bool>()) {
            try {
                func(*wrapper.table, collB);
            } catch (const luabridge::LuaException& e) {
                ReportError(actorB->name, e);
            }
        }
    }
}

void ContactListener::EndContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetUserData().pointer);
    Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB) return;

    b2Vec2 point(-999.0f, -999.0f);
    b2Vec2 normal(-999.0f, -999.0f);

    b2Vec2 relative_vel = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();

    Collision collA = { actorB, point, relative_vel, normal };
    Collision collB = { actorA, point, relative_vel, normal };

    bool is_sensor = fixtureA->IsSensor() || fixtureB->IsSensor();
    std::string func_name = is_sensor ? "OnTriggerExit" : "OnCollisionExit";

    std::vector<std::string> keysA;
    for (const auto& pair : actorA->components) {
        keysA.push_back(pair.first);
    }
    std::sort(keysA.begin(), keysA.end());

    for (const std::string& key : keysA) {
        auto& wrapper = actorA->components[key];
        luabridge::LuaRef func = (*wrapper.table)[func_name];
        luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
        if (func.isFunction() && enabled.isBool() && enabled.cast<bool>()) {
            try { func(*wrapper.table, collA); } 
            catch (const luabridge::LuaException& e) { ReportError(actorA->name, e); }
        }
    }

    std::vector<std::string> keysB;
    for (const auto& pair : actorB->components) {
        keysB.push_back(pair.first);
    }
    std::sort(keysB.begin(), keysB.end());

    for (const std::string& key : keysB) {
        auto& wrapper = actorB->components[key];
        luabridge::LuaRef func = (*wrapper.table)[func_name];
        luabridge::LuaRef enabled = (*wrapper.table)["enabled"];
        if (func.isFunction() && enabled.isBool() && enabled.cast<bool>()) {
            try { func(*wrapper.table, collB); } 
            catch (const luabridge::LuaException& e) { ReportError(actorB->name, e); }
        }
    }
}