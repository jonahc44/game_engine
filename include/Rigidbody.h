#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "box2d/box2d.h"
#include <string>
#include <vector>

class Actor;

class Rigidbody {
public:
    b2Body* body;
    std::string key = "";
    std::string type = "Rigidbody";
    bool enabled = true;
    Actor* actor = nullptr;
    bool has_started = false;
    float x = 0.0f;
    float y = 0.0f;
    std::string body_type = "dynamic";
    bool precise = true;
    float gravity_scale = 1.0f;
    float density = 1.0f;
    float angular_friction = 0.3f;
    float rotation = 0.0f;
    bool has_collider = true;
    bool has_trigger = true;
    std::string collider_type = "box";
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;
    std::string trigger_type = "box";
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;
    std::vector<b2Vec2> fluid_projection_points;
    
    Rigidbody() {};
    Rigidbody(const Rigidbody& other);
    ~Rigidbody();

    void OnStart();
    b2Vec2 GetPosition();
    float GetRotation();
    b2Vec2 GetVelocity();
    float GetAngularVelocity();
    float GetGravityScale();
    b2Vec2 GetUpDirection();
    b2Vec2 GetRightDirection();
    void AddForce(b2Vec2);
    void SetVelocity(b2Vec2);
    void SetPosition(b2Vec2);
    void SetRotation(float);
    void SetAngularVelocity(float);
    void SetGravityScale(float);
    void SetUpDirection(b2Vec2);
    void SetRightDirection(b2Vec2);
    void AddProjectionPoint(b2Vec2, float);
    void OnDestroy();
};

#endif