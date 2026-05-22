#include "map.hpp"

#include "external/open_simplex.hpp"

Map generate_map(int width, int height)
{
    Map map;

    int numPixel = width * height;
    float* heightmap = new float[numPixel];
    
    for (int i = 0; i < numPixel; i++)
    {
        OpenSimplex2::noise2(0, 0, 0);
    }

    delete[] heightmap;

    return map;
}
