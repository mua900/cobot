#ifndef _MAP_H
#define _MAP_H

#include "draw.hpp"

struct Surface {
    float roughness = 0;
};

struct Environment {
    float pressure = 0;
    float moist = 0;
};

struct Map {
    // @todo
    Surface surface = {};
    Environment environment = {};
    SDL_Texture* texture = nullptr;
};

#endif // _MAP_H