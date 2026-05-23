#ifndef _GAME_H
#define _GAME_H

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "text.hpp"

#include "vehicle.hpp"
#include "script.hpp"
#include "map.hpp"
#include "mission.hpp"

struct GameState {
    Vehicle vehicle = {};
    Map map = {};
    Mission mission = {};
    DArray<Script> scripts = {};

    AssetId partImages [PART_KIND_COUNT][MaxPartCount] = {};

    bool load_part_images(AssetCatalog& catalog);
};

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const GameState& game);

#endif // _GAME_H