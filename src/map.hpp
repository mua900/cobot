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

struct MapPixel {
    u8 a = 0;
    u8 b = 0;
    u8 g = 0;
    u8 r = 0;
};

struct Map {
    Surface surface = {};
    Environment environment = {};
    SDL_Texture* texture = nullptr;
    MapPixel* mapData = nullptr;
};

bool generate_map(SDL_Renderer* renderer, Map* map, u64 seed, int width, int height);

#endif // _MAP_H