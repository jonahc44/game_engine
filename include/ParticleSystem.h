#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "Helper.h"

class Actor;

// struct Particle {
//     float x;
//     float y;
//     float x_vel;
//     float y_vel;
//     float angular_speed;
//     float rotation;
//     float initial_scale;
//     float scale;
//     int start_frame;
//     int16_t color_r;
//     int16_t color_g;
//     int16_t color_b;
//     int16_t color_a;
//     bool active = false;
//     // std::string image;
//     // int sorting_order;
// };

class ParticleSystem {
public:
    std::string key = "";
    std::string type = "ParticleSystem";
    bool enabled = true;
    Actor* actor = nullptr;
    bool has_started = false;
    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;
    float x = 0.0f;
    float y = 0.0f;
    int frames_between_bursts = 1;
    int local_frame_number = 0;
    int burst_quantity = 1;
    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    int16_t start_color_r = 255;
    int16_t start_color_g = 255;
    int16_t start_color_b = 255;
    int16_t start_color_a = 255;
    std::string image = "";
    int sorting_order = 9999;
    int duration_frames = 300;
    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;
    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;
    float end_scale = std::numeric_limits<float>::quiet_NaN();
    int16_t end_color_r = -1;
    int16_t end_color_g = -1;
    int16_t end_color_b = -1;
    int16_t end_color_a = -1;
    bool emission_allowed = true;
    int active_particles = 0;

    RandomEngine emit_angle_distribution;
    RandomEngine emit_radius_distribution;
    RandomEngine rotation_distribution;
    RandomEngine scale_distribution;
    RandomEngine speed_distribution;
    RandomEngine rotation_speed_distribution;

    // std::vector<Particle> particles;
    std::vector<float> p_x;
    std::vector<float> p_y;
    std::vector<float> x_vel;
    std::vector<float> y_vel;
    std::vector<float> angular_speeds;
    std::vector<float> rotations;
    std::vector<float> initial_scales;
    std::vector<float> scales;
    std::vector<int> start_frames;
    std::vector<int16_t> color_r;
    std::vector<int16_t> color_g;
    std::vector<int16_t> color_b;
    std::vector<int16_t> color_a;
    std::vector<bool> active;
    std::queue<int> free_list;

    ParticleSystem() {};
    ParticleSystem(const ParticleSystem&);
    void OnStart();
    void OnUpdate();
    void Stop();
    void Play();
    void Burst();
};  

#endif