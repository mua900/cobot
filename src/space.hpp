#ifndef _SPACE_H
#define _SPACE_H

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "map.hpp"

// Keplerian orbit (in 2 dimensions)
// distance(trueAnomaly) = semiMajorAxis*(1-eccentricity**2) / (1 + eccentricity*cos(trueAnomaly))

// vis viva
// velocity**2 = G * mass * ((2 / distance) - (1 / semiMajorAxis))

enum OrbitalParameter {
    OrbitSemiMajorAxis,
    OrbitEccentricity,
    OrbitTrueAnomaly,
    OrbitLongitudeOfTheAscendingNode,
    OrbitArgumentOfPeriapsis,
    OrbitInclination,

    OrbitParameterCount,
};

struct OrbitalParameters {
    float semiMajorAxis = 0;
    float eccentricity = 0;
    float trueAnomaly = 0;
    float longitudeOfAscendingNode = 0;
    float argumentOfPeriapsis = 0;
    float inclination = 0;

    OrbitalParameters() {}
    OrbitalParameters(float a, float e, float mu, float loan, float ap, float i)
        : semiMajorAxis(a), eccentricity(e), trueAnomaly(mu), longitudeOfAscendingNode(loan), argumentOfPeriapsis(ap), inclination(i)
    {}
};

// orbitting body
struct Body {
    float mass = 0;
    float radius = 0;

    vec3 position = {};
    vec3 velocity = {};
    vec3 acceleration = {};

    OrbitalParameters parameters = {};

    Body() {}
    Body(float mass, float radius, vec3 init_position, vec3 init_velocity)
        : mass(mass), radius(radius), position(init_position), velocity(init_velocity)
    {}
    Body(float mass, float radius, float semi_major_axis, float eccentricity, float true_anomaly, float longitude_of_ascending_node, float argument_of_periapsis, float inclination)
        : mass(mass), radius(radius), parameters(semi_major_axis, eccentricity, true_anomaly, longitude_of_ascending_node, argument_of_periapsis, inclination)
    {}

    void determine_orbit(double centralBodyMass);
    void determine_state_vector(double centralBodyMass);
};

typedef u32 PlanetId;
constexpr PlanetId NullPlanetId = -1;

enum DefaultPlanetId : u32 {
    DefaultPlanetRed,
    DefaultPlanetGreen,
    DefaultPlanetBlue,
    DefaultPlanetCount,
};

typedef float (*PressureFunction) (float altitude);

struct Atmosphere {
    PressureFunction pressure_function = nullptr;
};

struct CelestialRotation {
    // > 0 -> prograde, 0 -> no rotation, < 0 -> retrograde
    int direction = 0;
    float rotational_period = 0;
    float axial_tilt = 0;
};

struct Planet {
    PlanetId id = {};
    String name = {};
    ColorF color = {};
    CelestialRotation rotation = {};
    Atmosphere atmosphere = {};
    SDL_Texture* map = {};
    Body body = {};

    Planet() {}
    Planet(String name, ColorF color, Body body) : name(name), color(color), body(body) {}
};

struct Star {
    // the star is at the center of it's system
    // vec2 position = {};
    float mass = 0;
    float radius = 0;
};

struct StarSystem {
    Star star = {};
    DArray<Planet> planets = {};

    void simulation_step(double dt);
};

StarSystem get_default_star_system(SDL_Renderer* renderer);

#endif // _SPACE_H