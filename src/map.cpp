#include "map.hpp"

#include "external/open_simplex.hpp"

SDL_Texture* generate_map(SDL_Renderer* renderer, u64 seed, int width, int height, float scale, ColorF tint)
{
    int numPixel = width * height;
    MapPixel* heightmap = new MapPixel[numPixel];
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float s = (OpenSimplex2::noise2(seed, x * scale, y * scale) + 1.0f) / 2.0f;
            heightmap[x + y * width].a = u8(0xff);
            heightmap[x + y * width].b = u8(s * 255.0f);
            heightmap[x + y * width].g = u8(s * 255.0f);
            heightmap[x + y * width].r = u8(s * 255.0f);
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR32, heightmap, width * sizeof(MapPixel));
    if (!surface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return nullptr;
    }

    delete[] heightmap;

    return texture;
}
