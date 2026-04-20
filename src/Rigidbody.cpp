#include "Rigidbody.h"
#include "PhysicsManger.h"
#include "glm/glm.hpp"
#include <iostream>

void Rigidbody::OnStart() {
    const uint16 COLLIDER_CATEGORY = 0x0001;
    const uint16 TRIGGER_CATEGORY  = 0x0002;
    const uint16 PHANTOM_CATEGORY  = 0x0004;

    PhysicsManager::hasRigidbodyExisted = true;        
    PhysicsManager::InitializeWorld();

    b2BodyDef bodyDef;

    if (body_type == "dynamic") {
        bodyDef.type = b2_dynamicBody;
    } else if (body_type == "static") {
        bodyDef.type = b2_staticBody;
    } else if (body_type == "kinematic") {
        bodyDef.type = b2_kinematicBody;
    }

    bodyDef.position = { x, y};
    bodyDef.angle = rotation * (b2_pi / 180.0f);
    bodyDef.bullet = precise;
    bodyDef.angularDamping = angular_friction;
    bodyDef.gravityScale = gravity_scale;

    body = PhysicsManager::world->CreateBody(&bodyDef);

    b2PolygonShape poly_shape;
    b2CircleShape circle_shape;
    b2Shape* my_shape = nullptr;

    if (has_collider) {
        if (collider_type == "box"){
            poly_shape.SetAsBox(width / 2.0f, height / 2.0f);
            my_shape = &poly_shape;
        }
        else if (collider_type == "circle") {
            circle_shape.m_radius = radius;
            my_shape = &circle_shape;
        }

        b2FixtureDef collider_def;
        collider_def.shape = my_shape;
        collider_def.density = density;
        collider_def.isSensor = false;
        collider_def.friction = friction;
        collider_def.restitution = bounciness;

        collider_def.filter.categoryBits = COLLIDER_CATEGORY;
        collider_def.filter.maskBits = COLLIDER_CATEGORY;
        collider_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);
        body->CreateFixture(&collider_def);
    }

    if (has_trigger) {
        if (trigger_type == "box"){
            poly_shape.SetAsBox(trigger_width / 2.0f, trigger_height / 2.0f);
            my_shape = &poly_shape;
        }
        else if (trigger_type == "circle") {
            circle_shape.m_radius = trigger_radius;
            my_shape = &circle_shape;
        }
        
        b2FixtureDef trigger_def;
        trigger_def.shape = my_shape;
        trigger_def.density = density;
        trigger_def.isSensor = true;

        trigger_def.filter.categoryBits = TRIGGER_CATEGORY;
        trigger_def.filter.maskBits = TRIGGER_CATEGORY;
        trigger_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);
        body->CreateFixture(&trigger_def);
    }

    if (!has_collider && !has_trigger) {
        b2FixtureDef phantom_def;
        poly_shape.SetAsBox(width / 2.0f, height / 2.0f);
        my_shape = &poly_shape;
        phantom_def.shape = my_shape;
        phantom_def.density = density;
        phantom_def.isSensor = true;

        phantom_def.filter.categoryBits = PHANTOM_CATEGORY;
        phantom_def.filter.maskBits = 0x0000;
        phantom_def.userData.pointer = reinterpret_cast<uintptr_t>(this->actor);

        body->CreateFixture(&phantom_def);
    }
}

Rigidbody::Rigidbody(const Rigidbody& other) {
    x = other.x;
    y = other.y;
    body_type = other.body_type;
    precise = other.precise;
    gravity_scale = other.gravity_scale;
    density = other.density;
    angular_friction = other.angular_friction;
    rotation = other.rotation;
    has_collider = other.has_collider;
    has_trigger = other.has_trigger;
    collider_type = other.collider_type;
    width = other.width;
    height = other.height;
    radius = other.radius;
    friction = other.friction;
    bounciness = other.bounciness;
    trigger_type = other.trigger_type;
    trigger_width = other.trigger_width;
    trigger_height = other.trigger_height;
    trigger_radius = other.trigger_radius;
    fluid_projection_points = other.fluid_projection_points;

    body = nullptr; 
}

Rigidbody::~Rigidbody() {
    if (PhysicsManager::world && body) {
        PhysicsManager::world->DestroyBody(body);
    }
}

b2Vec2 Rigidbody::GetPosition() {
    if (body == nullptr) return b2Vec2(x, y);
    return body->GetPosition();
}

float Rigidbody::GetRotation() {
    float radians = body->GetAngle();
    return radians * (180.0f / b2_pi);
}

b2Vec2 Rigidbody::GetVelocity() {
    return body->GetLinearVelocity();
}

float Rigidbody::GetAngularVelocity() {
    float radians = body->GetAngularVelocity();
    return radians * (180.0f / b2_pi); 
    return 0.0f;
}

float Rigidbody::GetGravityScale() {
    return body->GetGravityScale();
}

b2Vec2 Rigidbody::GetUpDirection() {
    float angle = body->GetAngle();
    b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
    result.Normalize();
    return result;
}

b2Vec2 Rigidbody::GetRightDirection() {
    float angle = body->GetAngle();
    b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
    result.Normalize();
    return result;
}

void Rigidbody::AddForce(b2Vec2 force) {
    body->ApplyForceToCenter(force, true);
}

void Rigidbody::SetVelocity(b2Vec2 vel) {
    body->SetLinearVelocity(vel);
}

void Rigidbody::SetPosition(b2Vec2 pos) {
    if (body == nullptr) {
        x = pos.x;
        y = pos.y;
    } else {
        body->SetTransform(pos, body->GetAngle());
    }
}

void Rigidbody::SetRotation(float degrees) {
    float radians = degrees * (b2_pi / 180.0f);
    body->SetTransform(body->GetPosition(), radians);
}

void Rigidbody::SetAngularVelocity(float degrees) {
    float radians = degrees * (b2_pi / 180.0f);
    body->SetAngularVelocity(radians);
}

void Rigidbody::SetGravityScale(float scale) {
    body->SetGravityScale(scale);
}

void Rigidbody::SetUpDirection(b2Vec2 dir) {
    dir.Normalize();
    float new_angle = glm::atan(dir.x, -dir.y);
    body->SetTransform(body->GetPosition(), new_angle);
}

void Rigidbody::SetRightDirection(b2Vec2 dir) {
    dir.Normalize();
    float new_angle = glm::atan(dir.x, -dir.y) - (b2_pi / 2.0f);
    body->SetTransform(body->GetPosition(), new_angle);
}

void Rigidbody::AddProjectionPoint(b2Vec2 pos, float angle) {
    
}

void Rigidbody::OnDestroy() {
    PhysicsManager::world->DestroyBody(body);
    body = nullptr;
}