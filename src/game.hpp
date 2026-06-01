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

struct GameState {
    s64 ticks = 0;

    Camera camera = {};
    Vehicle vehicle = {};
    Map map = {};
    Mission mission = {};
    StarSystem starSystem = {};
    DArray<Script> scripts = {};

    AssetId partImages [PART_KIND_COUNT][MaxPartCount] = {};

    void update(TimeInfo time);
    void fixedUpdate();

    bool load_part_images(AssetCatalog& catalog);
};

// @todo
void draw_game_state(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);

#endif // _GAME_H