#ifndef _OPEN_SIMPLEX_H
#define _OPEN_SIMPLEX_H

// Adapted from https://github.com/KdotJPG/OpenSimplex2/blob/master/java/OpenSimplex2.java

#include <cstdint>
typedef int64_t s64;

static const int N_GRADS_2D_EXPONENT = 7;
static const int N_GRADS_3D_EXPONENT = 8;
static const int N_GRADS_4D_EXPONENT = 9;
static const int N_GRADS_2D = 1 << N_GRADS_2D_EXPONENT;
static const int N_GRADS_3D = 1 << N_GRADS_3D_EXPONENT;
static const int N_GRADS_4D = 1 << N_GRADS_4D_EXPONENT;

class OpenSimplex2 {
public:
    static void initializeGradients2d();
    static void initializeGradients3d();
    static void initializeGradients4d();

    /*
     * Noise Evaluators
     */

    /**
     * 2D Simplex noise, standard lattice orientation.
     */
    static float noise2(s64 seed, double x, double y);

    /**
     * 2D Simplex noise, with Y pointing down the main diagonal.
     * Might be better for a 2D sandbox style game, where Y is vertical.
     * Probably slightly less optimal for heightmaps or continent maps,
     * unless your map is centered around an equator. It's a subtle
     * difference, but the option is here to make it an easy choice.
     */
    static float noise2_ImproveX(s64 seed, double x, double y);

private:
    /**
     * 2D Simplex noise base.
     */
    static float noise2_UnskewedBase(s64 seed, double xs, double ys);

public:
    /**
     * 3D OpenSimplex2 noise, with better visual isotropy in (X, Y).
     * Recommended for 3D terrain and time-varied animations.
     * The Z coordinate should always be the "different" coordinate in whatever your use case is.
     * If Y is vertical in world coordinates, call noise3_ImproveXZ(x, z, Y) or use noise3_XZBeforeY.
     * If Z is vertical in world coordinates, call noise3_ImproveXZ(x, y, Z).
     * For a time varied animation, call noise3_ImproveXY(x, y, T).
     */
    static float noise3_ImproveXY(s64 seed, double x, double y, double z);

    /**
     * 3D OpenSimplex2 noise, with better visual isotropy in (X, Z).
     * Recommended for 3D terrain and time-varied animations.
     * The Y coordinate should always be the "different" coordinate in whatever your use case is.
     * If Y is vertical in world coordinates, call noise3_ImproveXZ(x, Y, z).
     * If Z is vertical in world coordinates, call noise3_ImproveXZ(x, Z, y) or use noise3_ImproveXY.
     * For a time varied animation, call noise3_ImproveXZ(x, T, y) or use noise3_ImproveXY.
     */
    static float noise3_ImproveXZ(s64 seed, double x, double y, double z);

    /**
     * 3D OpenSimplex2 noise, fallback rotation option
     * Use noise3_ImproveXY or noise3_ImproveXZ instead, wherever appropriate.
     * They have less diagonal bias. This function's best use is as a fallback.
     */
    static float noise3_Fallback(s64 seed, double x, double y, double z);

private:
    /**
     * Generate overlapping cubic lattices for 3D OpenSimplex2 noise.
     */
    static float noise3_UnrotatedBase(s64 seed, double xr, double yr, double zr);

public:
    /**
     * 4D OpenSimplex2 noise, with XYZ oriented like noise3_ImproveXY
     * and W for an extra degree of freedom. W repeats eventually.
     * Recommended for time-varied animations which texture a 3D object (W=time)
     * in a space where Z is vertical
     */
    static float noise4_ImproveXYZ_ImproveXY(s64 seed, double x, double y, double z, double w);

    /**
     * 4D OpenSimplex2 noise, with XYZ oriented like noise3_ImproveXZ
     * and W for an extra degree of freedom. W repeats eventually.
     * Recommended for time-varied animations which texture a 3D object (W=time)
     * in a space where Y is vertical
     */
    static float noise4_ImproveXYZ_ImproveXZ(s64 seed, double x, double y, double z, double w);

    /**
     * 4D OpenSimplex2 noise, with XYZ oriented like noise3_Fallback
     * and W for an extra degree of freedom. W repeats eventually.
     * Recommended for time-varied animations which texture a 3D object (W=time)
     * where there isn't a clear distinction between horizontal and vertical
     */
    static float noise4_ImproveXYZ(s64 seed, double x, double y, double z, double w);
    
    /**
     * 4D OpenSimplex2 noise, with XY and ZW forming orthogonal triangular-based planes.
     * Recommended for 3D terrain, where X and Y (or Z and W) are horizontal.
     * Recommended for noise(x, y, sin(time), cos(time)) trick.
     */
    static float noise4_ImproveXY_ImproveZW(s64 seed, double x, double y, double z, double w);

    /**
     * 4D OpenSimplex2 noise, fallback lattice orientation.
     */
    static float noise4_Fallback(s64 seed, double x, double y, double z, double w);

private:
    /**
     * 4D OpenSimplex2 noise base.
     */
    static float noise4_UnskewedBase(s64 seed, double xs, double ys, double zs, double ws);

    /*
     * Utility
     */

    static float grad(s64 seed, s64 xsvp, s64 ysvp, float dx, float dy);
    static float grad(s64 seed, s64 xrvp, s64 yrvp, s64 zrvp, float dx, float dy, float dz);
    static float grad(s64 seed, s64 xsvp, s64 ysvp, s64 zsvp, s64 wsvp, float dx, float dy, float dz, float dw);
    static int fastFloor(double x);
    static int fastRound(double x);
};

#endif // _OPEN_SIMPLEX_H