#ifndef _GAME_H
#define _GAME_H

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "text.hpp"

#include "vehicle.hpp"
#include "script.hpp"
#include "map.hpp"

struct GameState {
    Vehicle vehicle;
    Icon icons[32];  // @todo update
    DArray<Script> scripts = {};

    ~GameState();
};


#endif // _GAME_H