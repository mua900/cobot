#ifndef MAP_HPP
#define MAP_HPP

#include "app/draw.hpp"

struct MapPixel {
    u8 a = 0;
    u8 b = 0;
    u8 g = 0;
    u8 r = 0;
};

SDL_Texture* generate_map(SDL_Renderer* renderer, u64 seed, int width, int height, float scale, cobot::ColorF tint);

// @todo
struct Map {
    float* heightmap = nullptr;
    int dimension_x = 0;
    int dimension_y = 0;
    SDL_Texture* texture = nullptr;

    bool is_valid() const {
        return heightmap != nullptr;
    }
};

bool load_map(const char* path, Map* map, SDL_Renderer* renderer);

#endif // MAP_HPP