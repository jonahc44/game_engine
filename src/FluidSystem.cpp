#include "FluidSystem.h"
#include "ImageManager.h"
#include <iostream>

FluidSystem::FluidSystem(const FluidSystem& other) {
    image = other.image;
    sorting_order = other.sorting_order;
    
    world_width = other.world_width;
    world_height = other.world_height;
    dt = other.dt;
}

void FluidSystem::OnStart() {
    glm::vec2 screen_size = ImageManager::GetWindowSize();
    // std::cout << "FluidSystem Booting. Window Size: " << screen_size.x << " x " << screen_size.y << "\n";

    // if (screen_size.x <= 0) screen_size.x = 1280;
    // if (screen_size.y <= 0) screen_size.y = 720;

    if (pixels_per_cell <= 0) pixels_per_cell = 4;
    grid_width = static_cast<int>(screen_size.x) / pixels_per_cell;
    grid_height = static_cast<int>(screen_size.y) / pixels_per_cell;

    float pixels_per_meter = 100.0f; 
    
    world_width = screen_size.x / pixels_per_meter; 
    world_height = screen_size.y / pixels_per_meter;

    int total_cells = grid_width * grid_height;

    pixel_buffer.resize(total_cells, 0);
    Vx.resize(total_cells, 0.0f);
    Vy.resize(total_cells, 0.0f);
    Vx0.resize(total_cells, 0.0f);
    Vy0.resize(total_cells, 0.0f);
    density.resize(total_cells, 0.0f);
    density0.resize(total_cells, 0.0f);

    ImageManager::CreateStreamingTexture("water_sim", grid_width, grid_height);
}
void FluidSystem::OnUpdate() {
    // wave_timer += dt;
    // float wave_interval = 3.0f;
    // float wave_duration = 0.5f;

    // if (wave_timer >= wave_interval) {
        
    //     int wave_thickness = 10;
        
    //     int start_y = static_cast<int>(grid_height * 0.2f);
    //     int end_y = static_cast<int>(grid_height * 0.8f);
        
    //     for (int y = start_y; y < end_y; ++y) {
    //         for (int x = 1; x <= wave_thickness; ++x) {
    //             Vx0[IX(x, y)] += 150.0f; 
    //             density0[IX(x, y)] += 200.0f; 
    //         }
    //     }

    //     if (wave_timer >= wave_interval + wave_duration) {
    //         wave_timer = 0.0f;
    //     }
    // }

    AddSource(density, density0);
    AddSource(Vx, Vx0);
    AddSource(Vy, Vy0);

    std::swap(Vx0, Vx);
    std::swap(Vy0, Vy);
    Advect(1, Vx, Vx0, Vx0, Vy0);
    Advect(2, Vy, Vy0, Vx0, Vy0);
    Project(Vx, Vy, Vx0, Vy0); 
    
    std::swap(density0, density);
    Advect(0, density, density0, Vx, Vy);

    float velocity_drag = 0.5f;
    for(int i = 0; i < density.size(); i++) {
        density[i] *= 0.98f; 

        Vx[i] *= (1.0f - velocity_drag * dt);
        Vy[i] *= (1.0f - velocity_drag * dt);
    }

    for (int y = 0; y < grid_height; ++y) {
        for (int x = 0; x < grid_width; ++x) {
            int index = y * grid_width + x;
            float d = density[index];
            
            d = std::max(0.0f, std::min(d, 1.0f));
            
            Uint8 r, g, b_col;
            
            if (d < 0.5f) {
                float t = d * 2.0f;
                r = static_cast<Uint8>(30 + (40 * t));
                g = static_cast<Uint8>(100 + (80 * t));
                b_col = static_cast<Uint8>(200 + (20 * t));
            } else {
                float t = (d - 0.5f) * 2.0f;
                r = static_cast<Uint8>(70 + (185 * t));
                g = static_cast<Uint8>(180 + (75 * t));
                b_col = static_cast<Uint8>(220 + (35 * t));
            }
            
            Uint8 a = 255; 
            pixel_buffer[index] = (r << 24) | (g << 16) | (b_col << 8) | a;
        }
    }

    ImageManager::UpdateStreamingTexture("water_sim", pixel_buffer, grid_width, grid_height);    
    ImageManager::DrawEx("water_sim", x, y, 0.0f, static_cast<float>(pixels_per_cell), static_cast<float>(pixels_per_cell), 0.5f, 0.5f, 255, 255, 255, 255, -10.0f);

    std::fill(density0.begin(), density0.end(), 0.0f);
    std::fill(Vx0.begin(), Vx0.end(), 0.0f);
    std::fill(Vy0.begin(), Vy0.end(), 0.0f);
}

void FluidSystem::AddSource(std::vector<float>& x, const std::vector<float>& s) {
    for (size_t i = 0; i < x.size(); i++) {
        x[i] += dt * s[i];
    }
}

void FluidSystem::SetBounds(int b, std::vector<float>& x) {
    for (int i = 1; i < grid_width - 1; i++) {
        x[IX(i, 0)] = (b == 2) ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, grid_height - 1)] = (b == 2) ? -x[IX(i, grid_height - 2)] : x[IX(i, grid_height - 2)];
    }

    for (int j = 1; j < grid_height - 1; j++) {
        x[IX(0, j)] = (b == 1) ? -x[IX(1, j)] : x[IX(1, j)];
        x[IX(grid_width - 1, j)] = (b == 1) ? -x[IX(grid_width - 2, j)] : x[IX(grid_width - 2, j)];
    }

    x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, grid_height - 1)] = 0.5f * (x[IX(1, grid_height - 1)] + x[IX(0, grid_height - 2)]);
    x[IX(grid_width - 1, 0)] = 0.5f * (x[IX(grid_width - 2, 0)] + x[IX(grid_width - 1, 1)]);
    x[IX(grid_width - 1, grid_height - 1)] = 0.5f * (x[IX(grid_width - 2, grid_height - 1)] + x[IX(grid_width - 1, grid_height - 2)]);
}

void FluidSystem::Advect(int b, std::vector<float>& d, std::vector<float>& d0, std::vector<float>& u, std::vector<float>& v) {
    // Calculate the physical size of a single grid cell
    float h_x = world_width / static_cast<float>(grid_width);
    float h_y = world_height / static_cast<float>(grid_height);
    
    float dt0_x = dt / h_x;
    float dt0_y = dt / h_y;

    for (int j = 1; j < grid_height - 1; j++) {
        for (int i = 1; i < grid_width - 1; i++) {
            float x = i - dt0_x * u[IX(i, j)];
            float y = j - dt0_y * v[IX(i, j)];

            if (x < 0.5f) x = 0.5f;
            if (x > grid_width - 1.5f) x = grid_width - 1.5f;
            int i0 = static_cast<int>(x);
            int i1 = i0 + 1;

            if (y < 0.5f) y = 0.5f;
            if (y > grid_height - 1.5f) y = grid_height - 1.5f;
            int j0 = static_cast<int>(y);
            int j1 = j0 + 1;

            float s1 = x - i0;
            float s0 = 1.0f - s1;
            float t1 = y - j0;
            float t0 = 1.0f - t1;

            d[IX(i, j)] =
                s0 * (t0 * d0[IX(i0, j0)] + t1 * d0[IX(i0, j1)]) +
                s1 * (t0 * d0[IX(i1, j0)] + t1 * d0[IX(i1, j1)]);
        }
    }
    SetBounds(b, d);
}

void FluidSystem::LinSolve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c) {
    float cRecip = 1.0f / c;
    
    for (int k = 0; k < 20; k++) {
        for (int j = 1; j < grid_height - 1; j++) {
            for (int i = 1; i < grid_width - 1; i++) {
                x[IX(i, j)] =
                    (x0[IX(i, j)] +
                        a * (x[IX(i + 1, j)] + x[IX(i - 1, j)] +
                             x[IX(i, j + 1)] + x[IX(i, j - 1)])) * cRecip;
            }
        }
        SetBounds(b, x);
    }
}

void FluidSystem::Project(std::vector<float>& u, std::vector<float>& v, std::vector<float>& p, std::vector<float>& div) {
    float h_x = world_width / static_cast<float>(grid_width);
    float h_y = world_height / static_cast<float>(grid_height);

    for (int j = 1; j < grid_height - 1; j++) {
        for (int i = 1; i < grid_width - 1; i++) {
            // Divergence scaled by physical cell size
            div[IX(i, j)] = -0.5f * (
                h_x * (u[IX(i + 1, j)] - u[IX(i - 1, j)]) +
                h_y * (v[IX(i, j + 1)] - v[IX(i, j - 1)])
            );
            p[IX(i, j)] = 0.0f;
        }
    }
    SetBounds(0, div);
    SetBounds(0, p);

    LinSolve(0, p, div, 1, 4);

    for (int j = 1; j < grid_height - 1; j++) {
        for (int i = 1; i < grid_width - 1; i++) {
            // Pressure gradient scaled by physical cell size
            u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h_x;
            v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h_y;
        }
    }
    SetBounds(1, u);
    SetBounds(2, v);
}

bool FluidSystem::GetGridPosAtWorldPos(float target_x, float target_y, int& out_grid_x, int& out_grid_y) {
    float bounds_left = x - (world_width / 2.0f);
    float bounds_top = y - (world_height / 2.0f);

    float local_x = target_x - bounds_left;
    float local_y = target_y - bounds_top;

    if (local_x < 0 || local_x >= world_width || local_y < 0 || local_y >= world_height) {
        return false; 
    }

    out_grid_x = static_cast<int>((local_x / world_width) * grid_width);
    out_grid_y = static_cast<int>((local_y / world_height) * grid_height);

    // std::cout << "grid x: " << out_grid_x << ", grid y: " << out_grid_y << std::endl;

    return true;
}

void FluidSystem::InteractWithRigidbody(Rigidbody* rb) {
    if (!rb) return;

    b2Vec2 pos = rb->GetPosition();
    b2Vec2 vel = rb->GetVelocity();
    float speed = vel.Length();

    float world_radius = 0.5f; 
    if (rb->collider_type == "circle") {
        world_radius = rb->radius;
    } else { 
        world_radius = std::max(rb->width, rb->height) / 2.0f;
    }

    b2Vec2 forward = vel;
    if (speed > 0.0f) forward.Normalize();
    b2Vec2 right(-forward.y, forward.x); 

    float offset_distance = world_radius * 1.2f;
    b2Vec2 stern_center(pos.x - (forward.x * offset_distance), pos.y - (forward.y * offset_distance));

    if (speed > 0.1f) {
        float half_width = world_radius; 

        b2Vec2 emitters[3] = {
            b2Vec2(stern_center.x - right.x * half_width, stern_center.y - right.y * half_width),
            b2Vec2(stern_center.x + right.x * half_width, stern_center.y + right.y * half_width),
            stern_center
        };

        float back_power = 40.0f;
        float side_power = 60.0f; 

        b2Vec2 thrusts[3] = {
            b2Vec2(-forward.x * back_power - right.x * side_power, -forward.y * back_power - right.y * side_power),
            b2Vec2(-forward.x * back_power + right.x * side_power, -forward.y * back_power + right.y * side_power),
            b2Vec2(-forward.x * back_power, -forward.y * back_power)
        };

        float dye_amounts[3] = { 350.0f, 350.0f, 500.0f };

        for (int k = 0; k < 3; k++) {
            int cx, cy;
            if (!GetGridPosAtWorldPos(emitters[k].x, emitters[k].y, cx, cy)) continue;

            int local_radius = 1; 
            for (int i = -local_radius; i <= local_radius; i++) {
                for (int j = -local_radius; j <= local_radius; j++) {
                    int px = cx + i;
                    int py = cy + j;

                    if (px < 1 || px >= grid_width - 1 || py < 1 || py >= grid_height - 1) continue;

                    if (i * i + j * j <= local_radius * local_radius) {
                        float noise = ((rand() % 100) / 100.0f) + 0.2f;

                        Vx0[IX(px, py)] += thrusts[k].x * speed; 
                        Vy0[IX(px, py)] += thrusts[k].y * speed;
                        
                        density0[IX(px, py)] += dye_amounts[k] * noise;
                    }
                }
            }
        }
    }

    int cx, cy;
    if (!GetGridPosAtWorldPos(pos.x, pos.y, cx, cy)) return;
    
    float fluid_vx = Vx[IX(cx, cy)];
    float fluid_vy = Vy[IX(cx, cy)];

    b2Vec2 relative_vel = b2Vec2(fluid_vx, fluid_vy) - vel;
    float drag_coefficient = 1.5f; 
    b2Vec2 drag_force = b2Vec2(relative_vel.x * drag_coefficient, relative_vel.y * drag_coefficient);

    float max_drag_force = 200.0f; 
    if (drag_force.LengthSquared() > max_drag_force * max_drag_force) {
        drag_force.Normalize();
        drag_force *= max_drag_force;
    }

    rb->AddForce(drag_force);
}

void FluidSystem::ApplyWhirlpool(float target_world_x, float target_world_y, float rotation_speed, float suction_power, float world_radius) {
    // std::cout << "Attempting to place whirlpool at (" << target_world_x << ", " << target_world_y << ")\n";
    int cx, cy;
    if (!GetGridPosAtWorldPos(target_world_x, target_world_y, cx, cy)) return;

    float cells_per_unit = static_cast<float>(grid_width) / world_width;
    int radius = std::max(1, static_cast<int>(world_radius * cells_per_unit));

    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            int px = cx + i;
            int py = cy + j;

            if (px < 1 || px >= grid_width - 1 || py < 1 || py >= grid_height - 1) continue;

            float dist_sq = (i * i) + (j * j);
            if (dist_sq <= radius * radius && dist_sq > 0) {
                float dist = std::sqrt(dist_sq);
                
                float falloff = 1.0f - (dist / radius); 
                falloff *= falloff; 

                float nx = i / dist;
                float ny = j / dist;

                float tx = -ny;
                float ty = nx;

                Vx0[IX(px, py)] += (tx * rotation_speed - nx * suction_power) * falloff;
                Vy0[IX(px, py)] += (ty * rotation_speed - ny * suction_power) * falloff;

                density0[IX(px, py)] += 2.0f * falloff;
            }
        }
    }
}