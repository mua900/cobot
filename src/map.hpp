#ifndef _MAP_H
#define _MAP_H

#include "draw.hpp"

struct MapPixel {
    u8 a = 0;
    u8 b = 0;
    u8 g = 0;
    u8 r = 0;
};

struct Map {
    SDL_Texture* texture = nullptr;
    MapPixel* mapData = nullptr;
};

SDL_Texture* generate_map(SDL_Renderer* renderer, u64 seed, int width, int height, float scale, ColorF tint);

#endif // _MAP_H