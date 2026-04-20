#ifndef FLUIDSYSTEM_H
#define FLUIDSYSTEM_H

#include "Rigidbody.h"
#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include <algorithm>

class Actor;

class FluidSystem {
public:
    std::string key = "";
    std::string type = "FluidSystem";
    bool enabled = true;
    Actor* actor = nullptr;
    bool has_started = false;
    float x = 0.0f;
    float y = 0.0f;
    std::string image = "";
    int sorting_order = 9999;
    std::vector<Rigidbody*> rbs;

    std::vector<float> Vx, Vy;
    std::vector<float> Vx0, Vy0;

    std::vector<float> density;
    std::vector<float> density0;

    float dt = 0.016f;
    float diffusion = 0.0f;
    float viscosity = 0.0f;
    float wave_timer = 0.0f;

    FluidSystem() {};
    FluidSystem(const FluidSystem&);
    void OnStart();
    void OnUpdate();
    void OnLateUpdate();
    void OnDestroy();

    void AddSource(std::vector<float>& x, const std::vector<float>& s);
    void SetBounds(int b, std::vector<float>& x);
    void LinSolve(int b, std::vector<float>& x, const std::vector<float>& x0, float a, float c);
    
    void Advect(int, std::vector<float>&, std::vector<float>&, std::vector<float>&, std::vector<float>&);
    void Project(std::vector<float>& u, std::vector<float>& v, std::vector<float>& p, std::vector<float>& div);

    bool GetGridPosAtWorldPos(float target_x, float target_y, int& out_grid_x, int& out_grid_y);
    void InteractWithRigidbody(Rigidbody* rb);
    void RegisterRB(Rigidbody* rb);
    void ApplyWhirlpool(float target_world_x, float target_world_y, float rotation_speed, float suction_power, float world_radius);

    inline int IX(int x, int y) {
        x = std::max(0, std::min(x, grid_width - 1));
        y = std::max(0, std::min(y, grid_height - 1));
        return x + (y * grid_width);
    }

private:
    int grid_width = 200;
    int grid_height = 200;
    float world_width = 1000.0f;
    float world_height = 1000.0f;
    int pixels_per_cell = -1;
    std::vector<Uint32> pixel_buffer;
};

#endif