#include "map.hpp"

#include "external/open_simplex.hpp"

// @todo voronoise https://iquilezles.org/articles/voronoise/
// @todo domain warping
// @todo use parameters
bool generate_map(Map* map, SDL_Renderer* renderer, float scale, cobot::ColorF tint)
{
    int width = map->dimension_x;
    int height = map->dimension_y;
    int numPixel = width * height;
    MapPixel* heightmap = new MapPixel[numPixel];
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float s = (OpenSimplex2::noise2(map->seed, x * scale, y * scale) + 1.0f) / 2.0f;
            heightmap[x + y * width].a = u8(0xff);
            heightmap[x + y * width].b = u8(s * 255.0f);
            heightmap[x + y * width].g = u8(s * 255.0f);
            heightmap[x + y * width].r = u8(s * 255.0f);
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR32, heightmap, width * sizeof(MapPixel));
    if (!surface) {
        delete[] heightmap;
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        delete[] heightmap;
        return false;
    }

    map->heightmap = heightmap;
    map->texture = texture;

    return texture;
}

bool load_map(const char* path, Map* map, SDL_Renderer* renderer)
{
    return false;
}
