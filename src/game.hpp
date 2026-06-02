#ifndef _GAME_H
#define _GAME_H

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "text.hpp"
#include "time.hpp"

#include "vehicle.hpp"
#include "script.hpp"
#include "map.hpp"
#include "mission.hpp"
#include "space.hpp"

struct Camera {
    vec2 position = {};
    float zoom = 0;
};

struct GameState;

typedef void (*UpdateFunction)(GameState* game, TimeInfo time);
typedef void (*FixedUpdateFunction)(GameState* game);

struct UpdateState {
    UpdateFunction update = nullptr;
    FixedUpdateFunction fixedUpdate = nullptr;
    s64 ticks = 0;
};

struct GameState {
    Camera camera = {};
    Vehicle vehicle = {};
    Map map = {};
    Mission mission = {};
    StarSystem starSystem = {};
    DArray<Script> scripts = {};

    AssetId partImages [PART_KIND_COUNT][MaxPartCount] = {};
    UpdateState updateState = {};

    void update(TimeInfo time);

    bool load_part_images(AssetCatalog& catalog);
};

void idleUpdate(GameState* game, TimeInfo time);
void idleFixedUpdate(GameState* game);

void vehicleSimulationUpdate(GameState* game, TimeInfo time);
void vehicleSimulationFixedUpdate(GameState* game);

void starSystemUpdate(GameState* game, TimeInfo time);
void starSystemFixedUpdate(GameState* game);

void draw_game_state(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);
void draw_game_star_system(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);
void draw_planet_orbit(RenderContext& context, const Planet& planet, vec2 offset, double centralBodyMass, float thick, ColorF color);
void draw_orbits(RenderContext& context, const AssetCatalog& catalog, const GameState& game, ColorF color);
void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);

#endif // _GAME_H