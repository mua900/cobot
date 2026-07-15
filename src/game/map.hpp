#ifndef MAP_HPP
#define MAP_HPP

#include "app/draw.hpp"

struct MapPixel {
    u8 a = 0;
    u8 b = 0;
    u8 g = 0;
    u8 r = 0;
};

struct Map {
    u64 seed = 0;

    float* heightmap = nullptr;
    int dimension_x = 0;
    int dimension_y = 0;
    SDL_Texture* texture = nullptr;

    bool is_valid() const {
        return heightmap != nullptr;
    }
};

bool generate_map(Map* map, SDL_Renderer* renderer, float scale, cobot::ColorF tint);
bool load_map(const char* path, Map* map, SDL_Renderer* renderer);

float fbm(cobot::vec2 x, u64 seed, int numOctaves);
float noise(cobot::vec2 x, u64 seed, float scale);

struct MapState
{

};

#endif // MAP_HPP