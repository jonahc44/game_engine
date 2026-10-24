# game_engine

A 2D game engine written in C++17. Games are built entirely from data files and Lua scripts: scenes and actors are described in JSON, behavior is written as Lua components, and the engine handles rendering, input, audio, physics, particles, and a grid-based fluid simulation.

## Features

- **Actor/component model.** Actors are containers of components. Components are Lua tables (or built-in native components) with optional lifecycle callbacks: `OnStart`, `OnUpdate`, `OnLateUpdate`, `OnDestroy`.
- **Lua scripting.** Uses Lua 5.4 with LuaBridge. Every `.lua` file in `resources/component_types/` becomes a component type, and scripts can reach the engine APIs listed under [Lua API](#lua-api).
- **Scenes and templates.** Scenes are JSON files of actors. Actor templates are reusable prefabs that scenes can reference and that scripts can spawn at runtime with `Actor.Instantiate`. Actors marked with `Scene.DontDestroy` persist across scene loads.
- **Rendering.** Uses the SDL2 renderer, with a camera you can position and zoom, sorted world-space sprites, a screen-space UI layer, per-pixel drawing, and TTF text.
- **Physics.** Box2D rigidbodies (dynamic, static, kinematic) with box or circle colliders and triggers. Lua callbacks fire on collisions and triggers (`OnCollisionEnter/Exit`, `OnTriggerEnter/Exit`), and raycasts are available (`Physics.Raycast`, `Physics.RaycastAll`).
- **Particle system.** The native `ParticleSystem` component has configurable bursts, emission shape, speed, rotation, gravity, drag, and scale and color over lifetime. Particles are stored in a ring buffer.
- **Fluid simulation.** The native `FluidSystem` component is a stable-fluids style grid solver (advect, diffuse, project) rendered to a streaming texture. It can push rigidbodies around and apply whirlpool forces.
- **Input.** Keyboard keys by name, mouse buttons, mouse position, scroll wheel, and cursor show/hide.
- **Audio.** Plays `.wav`/`.ogg` clips on channels through SDL2_mixer, with per-channel volume.
- **Events.** A publish/subscribe event bus for decoupled communication between components.

## Building

The engine uses [Premake 5](https://premake.github.io/) to generate project files for each platform. All third-party code is vendored in `third_party/`: Lua, LuaBridge, Box2D, GLM, RapidJSON, and the SDL2 headers and libraries.

### Linux

Requirements: `clang`, `premake5`, and the SDL2, SDL2_image, and SDL2_mixer development packages. SDL2_ttf is bundled in `third_party/SDL2_ttf/lib` and linked through an rpath.

```sh
# Debian/Ubuntu
sudo apt install clang libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev
# Arch
sudo pacman -S clang sdl2 sdl2_image sdl2_mixer

premake5 gmake
make            # builds box2d, then game_engine
```

The build produces `./game_engine_linux` in the repo root.

### macOS

```sh
premake5 xcode4
open game_engine.xcworkspace
```

Build the `game_engine` target. The SDL2 frameworks in `third_party/*/lib` are linked and found at runtime through an rpath. The executable is `game_engine_osx`.

### Windows

```sh
premake5 vs2022
```

Open `game_engine.sln` and build the x64 Release configuration. The SDL2 DLLs are copied next to `x64/Release/game_engine_win.exe` after each build.

### Editor support

`export-compile-commands.lua` adds a Premake action that generates `compile_commands.json` for clangd and similar tools:

```sh
premake5 export-compile-commands
```

## Running a game

Run the executable from a directory that contains a `resources/` folder. The engine exits with an error message if `resources/` or `resources/game.config` is missing.

```
resources/
├── game.config              # required
├── rendering.config         # optional
├── scenes/                  # <name>.scene
├── actor_templates/         # <name>.template
├── component_types/         # <ComponentName>.lua
├── images/                  # .png
├── audio/                   # .wav or .ogg
└── fonts/                   # .ttf
```

`resources/` is gitignored, so each game keeps its own content outside the engine repo.

### `game.config`

```json
{
  "game_title": "My Game",
  "initial_scene": "level1"
}
```

### `rendering.config` (optional)

```json
{
  "x_resolution": 1280,
  "y_resolution": 720,
  "clear_color_r": 30,
  "clear_color_g": 30,
  "clear_color_b": 40,
  "zoom_factor": 1.0
}
```

### Scenes

`resources/scenes/level1.scene`:

```json
{
  "actors": [
    {
      "name": "Player",
      "template": "Player",
      "components": {
        "1": { "type": "PlayerController", "speed": 6 }
      }
    },
    {
      "name": "Ground",
      "components": {
        "rb": { "type": "Rigidbody", "body_type": "static", "x": 0, "y": 5, "width": 20, "height": 1 }
      }
    }
  ]
}
```

Each key under `components` is the component's key on that actor. Every other field is copied onto the component instance, overriding the defaults from the Lua table or template. Components on an actor run in key order.

### Actor templates

A file in `resources/actor_templates/<name>.template` has the same format as a single actor entry. Scenes can reference it with `"template": "<name>"`, and Lua can spawn it with `Actor.Instantiate("<name>")`.

### Lua components

`resources/component_types/PlayerController.lua`. The global table name must match the file name.

```lua
PlayerController = {
    speed = 5,

    OnStart = function(self)
        self.rb = self.actor:GetComponent("Rigidbody")
    end,

    OnUpdate = function(self)
        local vel = self.rb:GetVelocity()
        if Input.GetKey("left")  then vel.x = -self.speed end
        if Input.GetKey("right") then vel.x =  self.speed end
        self.rb:SetVelocity(vel)

        if Input.GetKeyDown("space") then
            Event.Publish("player_jump", {})
        end
    end,

    OnCollisionEnter = function(self, collision)
        Debug.Log("hit " .. collision.other:GetName())
    end,
}
```

Each instance gets `self.actor`, `self.key`, `self.type`, and `self.enabled`. Setting `enabled = false` stops the component's callbacks.

## Lua API

| Namespace / class | Functions |
|---|---|
| `Debug` | `Log(msg)` |
| `Application` | `Quit()`, `Sleep(ms)`, `GetFrame()`, `OpenURL(url)` |
| `Actor` | `GetName()`, `GetID()`, `GetComponent(type)`, `GetComponents(type)`, `GetComponentByKey(key)`, `AddComponent(type)`, `RemoveComponent(comp)`; static `Find(name)`, `FindAll(name)`, `Instantiate(template)`, `Destroy(actor)` |
| `Input` | `GetKey(k)`, `GetKeyDown(k)`, `GetKeyUp(k)`, `GetMousePosition()`, `GetMouseButton(b)`, `GetMouseButtonDown(b)`, `GetMouseButtonUp(b)`, `GetMouseScrollDelta()`, `HideCursor()`, `ShowCursor()` |
| `Image` | `Draw(name, x, y)`, `DrawEx(name, x, y, rot, sx, sy, px, py, r, g, b, a, order)`, `DrawUI(name, x, y)`, `DrawUIEx(name, x, y, r, g, b, a, order)`, `DrawPixel(x, y, r, g, b, a)`, `GetWindowSize()` |
| `Text` | `Draw(str, x, y, font, size, r, g, b, a)` |
| `Audio` | `Play(channel, clip, loop)`, `Halt(channel)`, `SetVolume(channel, volume)` |
| `Camera` | `SetPosition(x, y)`, `GetPositionX()`, `GetPositionY()`, `SetZoom(z)`, `GetZoom()` |
| `Scene` | `Load(name)`, `GetCurrent()`, `DontDestroy(actor)` |
| `Event` | `Publish(type, obj)`, `Subscribe(type, component, fn)`, `Unsubscribe(type, component, fn)` |
| `Physics` | `Raycast(pos, dir, dist)`, `RaycastAll(pos, dir, dist)` (returns `HitResult` objects with `actor`, `point`, `normal`, `is_trigger`) |
| `Vector2` | `Vector2(x, y)`, `.x`, `.y`, `Normalize()`, `Length()`, `+ - *`, static `Distance(a, b)`, `Dot(a, b)` |
| `Rigidbody` | properties `x`, `y`, `body_type`, `collider_type`, `width`, `height`, `radius`, `friction`, `bounciness`, `density`, `gravity_scale`, `has_collider`, `has_trigger`, `trigger_*`, …; methods `Get/SetPosition`, `Get/SetRotation`, `Get/SetVelocity`, `Get/SetAngularVelocity`, `Get/SetGravityScale`, `Get/SetUpDirection`, `Get/SetRightDirection`, `AddForce(v)` |
| `ParticleSystem` | emission/appearance properties (see `include/ParticleSystem.h`); `Play()`, `Stop()`, `Burst()` |
| `FluidSystem` | `InteractWithRigidbody(rb)`, `ApplyWhirlpool(x, y, rot_speed, suction, radius)` |

Physics runs at a fixed 1/60 s step with gravity `(0, 9.8)`, so +y points down. World units are meters, at 100 pixels per meter.

## Project layout

```
include/, src/        engine source (one manager per subsystem)
third_party/          vendored dependencies
premake5.lua          build configuration for all platforms
helper_scripts/       small content utilities (e.g. shift.py offsets actor positions in a scene)
```
