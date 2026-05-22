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
};


#endif // _GAME_H