#include "ParticleSystem.h"
#include "ImageManager.h"
#include "glm/glm.hpp"
#include <chrono>

ParticleSystem::ParticleSystem(const ParticleSystem& other) {
    emit_angle_min = other.emit_angle_min;
    emit_angle_max = other.emit_angle_max;
    emit_radius_min = other.emit_radius_min;
    emit_radius_max = other.emit_radius_max;

    frames_between_bursts = other.frames_between_bursts;
    burst_quantity = other.burst_quantity;
    duration_frames = other.duration_frames;
    
    start_scale_min = other.start_scale_min;
    start_scale_max = other.start_scale_max;
    rotation_min = other.rotation_min;
    rotation_max = other.rotation_max;
    
    start_speed_min = other.start_speed_min;
    start_speed_max = other.start_speed_max;
    rotation_speed_min = other.rotation_speed_min;
    rotation_speed_max = other.rotation_speed_max;
    
    gravity_scale_x = other.gravity_scale_x;
    gravity_scale_y = other.gravity_scale_y;
    drag_factor = other.drag_factor;
    angular_drag_factor = other.angular_drag_factor;
    
    start_color_r = other.start_color_r;
    start_color_g = other.start_color_g;
    start_color_b = other.start_color_b;
    start_color_a = other.start_color_a;
    
    end_scale = other.end_scale;
    end_color_r = other.end_color_r;
    end_color_g = other.end_color_g;
    end_color_b = other.end_color_b;
    end_color_a = other.end_color_a;
    
    image = other.image;
    sorting_order = other.sorting_order;
}

void ParticleSystem::Burst() {
    for (int i = 0; i < burst_quantity; i++) {
        float angle_radians = glm::radians(emit_angle_distribution.Sample());
        float radius = emit_radius_distribution.Sample();
        float rotation = rotation_distribution.Sample();
        float scale = scale_distribution.Sample();

        float cos_angle = glm::cos(angle_radians);
        float sin_angle = glm::sin(angle_radians);

        float speed = speed_distribution.Sample();
        float rotation_speed = rotation_speed_distribution.Sample();

        int p_idx;
        if (!free_list.empty()) {
            p_idx = free_list.front();
            free_list.pop();
        } else {
            p_idx = active.size();
            active.push_back(false);
            p_x.push_back(0.0f);
            p_y.push_back(0.0f);
            x_vel.push_back(0.0f);
            y_vel.push_back(0.0f);
            angular_speeds.push_back(0.0f);
            rotations.push_back(0.0f);
            initial_scales.push_back(0.0f);
            scales.push_back(0.0f);
            start_frames.push_back(0);
            color_r.push_back(0);
            color_g.push_back(0);
            color_b.push_back(0);
            color_a.push_back(0);
        }

        active[p_idx] = true; 
        p_x[p_idx] = this->x + (cos_angle * radius);
        p_y[p_idx] = this->y + (sin_angle * radius);
        x_vel[p_idx] = cos_angle * speed;
        y_vel[p_idx] = sin_angle * speed;
        angular_speeds[p_idx] = rotation_speed;
        rotations[p_idx] = rotation;
        initial_scales[p_idx] = scale;
        scales[p_idx] = scale;
        start_frames[p_idx] = local_frame_number;
        color_r[p_idx] = start_color_r;
        color_g[p_idx] = start_color_g;
        color_b[p_idx] = start_color_b;
        color_a[p_idx] = start_color_a;
    }
}

void ParticleSystem::OnStart() {
    if (image == "") ImageManager::CreateDefaultParticle();

    emit_angle_distribution = RandomEngine(emit_angle_min, emit_angle_max, 298);
    emit_radius_distribution = RandomEngine(emit_radius_min, emit_radius_max, 404);

    rotation_distribution = RandomEngine(rotation_min, rotation_max, 440);
    scale_distribution = RandomEngine(start_scale_min, start_scale_max, 494);

    speed_distribution = RandomEngine(start_speed_min, start_speed_max, 498);
    rotation_speed_distribution = RandomEngine(rotation_speed_min, rotation_speed_max, 305);

    if (frames_between_bursts < 1) frames_between_bursts = 1;
    if (burst_quantity < 1) burst_quantity = 1;
    if (duration_frames < 1) duration_frames = 1;

    int max_particles = glm::ceil(duration_frames / frames_between_bursts) * burst_quantity;
    p_x.reserve(max_particles);
    p_y.reserve(max_particles);
    x_vel.reserve(max_particles);
    y_vel.reserve(max_particles);
    angular_speeds.reserve(max_particles);
    rotations.reserve(max_particles);
    initial_scales.reserve(max_particles);
    scales.reserve(max_particles);
    start_frames.reserve(max_particles);
    color_r.reserve(max_particles);
    color_g.reserve(max_particles);
    color_b.reserve(max_particles);
    color_a.reserve(max_particles);
    active.reserve(max_particles);
}

void ParticleSystem::OnUpdate() {
    if (local_frame_number % frames_between_bursts == 0 && emission_allowed) {
        Burst();
    }

    for (int i = 0; i < active.size(); ++i) {
        if (!active[i]) continue;

        active_particles++;

        int frames_particle_been_alive = local_frame_number - start_frames[i];
        
        if (frames_particle_been_alive >= duration_frames) {
            active[i] = false;
            free_list.push(i);
            continue;
        } 

        x_vel[i] += gravity_scale_x;
        y_vel[i] += gravity_scale_y;

        x_vel[i] *= drag_factor;
        y_vel[i] *= drag_factor;
        angular_speeds[i] *= angular_drag_factor;

        p_x[i] += x_vel[i];
        p_y[i] += y_vel[i];
        rotations[i] += angular_speeds[i];

        float lifetime_progress = static_cast<float>(frames_particle_been_alive) / duration_frames;

        if (!std::isnan(end_scale)) {
            scales[i] = glm::mix(initial_scales[i], end_scale, lifetime_progress);
        }

        if (end_color_r >= 0) {
            color_r[i] = glm::mix(start_color_r, end_color_r, lifetime_progress);
        }

        if (end_color_g >= 0) {
            color_g[i] = glm::mix(start_color_g, end_color_g, lifetime_progress);
        }

        if (end_color_b >= 0) {
            color_b[i] = glm::mix(start_color_b, end_color_b, lifetime_progress);
        }

        if (end_color_a >= 0) {
            color_a[i] = glm::mix(start_color_a, end_color_a, lifetime_progress);
        }

        ImageManager::DrawEx(image, p_x[i], p_y[i], rotations[i], scales[i], scales[i], 0.5f, 0.5f, color_r[i], color_g[i], color_b[i], color_a[i], sorting_order);
    }

    local_frame_number++;
}

void ParticleSystem::Stop() {
    emission_allowed = false;
}

void ParticleSystem::Play() {
    emission_allowed = true;
}