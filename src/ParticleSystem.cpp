#include "ParticleSystem.h"
#include "ImageManager.h"
#include "glm/glm.hpp"
#include <algorithm>

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

template <typename T>
static void Relinearize(std::vector<T>& v, int head, int count, int new_capacity) {
    std::vector<T> out(new_capacity);
    int old_capacity = static_cast<int>(v.size());
    for (int i = 0; i < count; ++i) {
        int src = head + i;
        if (src >= old_capacity) src -= old_capacity;
        out[i] = v[src];
    }
    v.swap(out);
}

void ParticleSystem::Resize(int new_capacity) {
    Relinearize(p_x, head, count, new_capacity);
    Relinearize(p_y, head, count, new_capacity);
    Relinearize(x_vel, head, count, new_capacity);
    Relinearize(y_vel, head, count, new_capacity);
    Relinearize(angular_speeds, head, count, new_capacity);
    Relinearize(rotations, head, count, new_capacity);
    Relinearize(initial_scales, head, count, new_capacity);
    Relinearize(scales, head, count, new_capacity);
    Relinearize(start_frames, head, count, new_capacity);
    Relinearize(color_r, head, count, new_capacity);
    Relinearize(color_g, head, count, new_capacity);
    Relinearize(color_b, head, count, new_capacity);
    Relinearize(color_a, head, count, new_capacity);
    head = 0;
    capacity = new_capacity;
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

        // Only reached if Burst() is called from Lua beyond the size computed in OnStart.
        if (count == capacity) Resize(std::max({capacity * 2, burst_quantity, 1}));

        int p_idx = head + count;
        if (p_idx >= capacity) p_idx -= capacity;
        count++;

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

    // Most bursts that can be alive at once, so steady-state emission never grows the buffers.
    int max_bursts_alive = (duration_frames + frames_between_bursts - 1) / frames_between_bursts;
    int max_particles = max_bursts_alive * burst_quantity;
    if (max_particles > capacity) Resize(max_particles);
}

void ParticleSystem::Simulate(int begin, int end) {
    const float gx = gravity_scale_x;
    const float gy = gravity_scale_y;
    const float drag = drag_factor;
    const float angular_drag = angular_drag_factor;

    float* __restrict px = p_x.data();
    float* __restrict py = p_y.data();
    float* __restrict vx = x_vel.data();
    float* __restrict vy = y_vel.data();
    float* __restrict av = angular_speeds.data();
    float* __restrict rot = rotations.data();

    for (int i = begin; i < end; ++i) {
        vx[i] = (vx[i] + gx) * drag;
        vy[i] = (vy[i] + gy) * drag;
        av[i] *= angular_drag;
        px[i] += vx[i];
        py[i] += vy[i];
        rot[i] += av[i];
    }

    // Lifetime-based properties: the per-system checks are hoisted out of the per-particle loop.
    const int frame = local_frame_number;
    const float duration = static_cast<float>(duration_frames);
    const int* __restrict start = start_frames.data();

    if (!std::isnan(end_scale)) {
        const float* __restrict init = initial_scales.data();
        float* __restrict sc = scales.data();
        for (int i = begin; i < end; ++i) {
            sc[i] = glm::mix(init[i], end_scale, static_cast<float>(frame - start[i]) / duration);
        }
    }

    auto lerp_channel = [&](std::vector<int16_t>& channel, int16_t from, int16_t to) {
        if (to < 0) return;
        int16_t* __restrict c = channel.data();
        for (int i = begin; i < end; ++i) {
            c[i] = glm::mix(from, to, static_cast<float>(frame - start[i]) / duration);
        }
    };
    lerp_channel(color_r, start_color_r, end_color_r);
    lerp_channel(color_g, start_color_g, end_color_g);
    lerp_channel(color_b, start_color_b, end_color_b);
    lerp_channel(color_a, start_color_a, end_color_a);
}

void ParticleSystem::Submit(int begin, int end) {
    for (int i = begin; i < end; ++i) {
        ImageManager::DrawEx(image, p_x[i], p_y[i], rotations[i], scales[i], scales[i], 0.5f, 0.5f, color_r[i], color_g[i], color_b[i], color_a[i], sorting_order);
    }
}

void ParticleSystem::OnUpdate() {
    // Retire expired particles before emitting, so a full ring frees space for this frame's burst.
    while (count > 0 && local_frame_number - start_frames[head] >= duration_frames) {
        if (++head == capacity) head = 0;
        --count;
    }

    if (local_frame_number % frames_between_bursts == 0 && emission_allowed) {
        Burst();
    }

    active_particles = count;

    // The live range may wrap around the end of the ring; process it as two contiguous spans.
    int first_span = std::min(count, capacity - head);
    Simulate(head, head + first_span);
    Simulate(0, count - first_span);
    Submit(head, head + first_span);
    Submit(0, count - first_span);

    local_frame_number++;
}

void ParticleSystem::Stop() {
    emission_allowed = false;
}

void ParticleSystem::Play() {
    emission_allowed = true;
}