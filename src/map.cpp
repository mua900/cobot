#include "map.hpp"

#include "external/open_simplex.hpp"

#include <ctime>

bool generate_map(SDL_Renderer* renderer, Map* map, u64 seed, int width, int height)
{
    int numPixel = width * height;
    MapPixel* heightmap = new MapPixel[numPixel];
    
    float scale = 0.1;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            u8 s = static_cast<u8>((OpenSimplex2::noise2(seed, x * scale, y * scale) + 1.0f) * 127.0f);
            heightmap[x + y * width].a = 0xff;
            heightmap[x + y * width].b = s;
            heightmap[x + y * width].g = s;
            heightmap[x + y * width].r = s;
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR32, heightmap, width * sizeof(MapPixel));
    if (!surface) {
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return false;
    }

    map->mapData = heightmap;
    map->texture = texture;

    return true;
}
