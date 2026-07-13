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

    if (map->heightmap)
    {
        delete[] map->heightmap;
    }

    MapPixel* heightmap = new MapPixel[numPixel];
    map->heightmap = new float[numPixel];
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float s = (OpenSimplex2::noise2(map->seed, x * scale, y * scale) + 1.0f) / 2.0f;
            int index = x + y * width;
            heightmap[index].a = u8(0xff);
            heightmap[index].b = u8(s * 255.0f);
            heightmap[index].g = u8(s * 255.0f);
            heightmap[index].r = u8(s * 255.0f);

            map->heightmap[index] = s;
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ABGR32, heightmap, width * sizeof(MapPixel));
    if (!surface) {
        delete[] heightmap;
        return false;
    }

    delete[] heightmap;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
        return false;
    }

    map->texture = texture;

    return texture;
}

bool load_map(const char* path, Map* map, SDL_Renderer* renderer)
{
    return false;
}
