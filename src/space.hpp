#ifndef _SPACE_H
#define _SPACE_H

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"

// Keplerian orbit (in 2 dimensions)
// distance(trueAnomaly) = semiMajorAxis*(1-eccentricity**2) / (1 + eccentricity*cos(trueAnomaly))

// vis viva
// velocity**2 = G * mass * ((2 / distance) - (1 / semiMajorAxis))

// orbitting body
struct Body {
    float mass = 0;
    float radius = 0;

    vec3 position = {};
    vec3 velocity = {};
    vec3 acceleration = {};

    float semiMajorAxis = 0;
    float eccentricity = 0;
    float trueAnomaly = 0;
    float longitudeOfAscendingNode = 0;
    float argumentOfPeriapsis = 0;
    float inclination = 0;

    Body() {}
    Body(float mass, float radius, vec3 init_position, vec3 init_velocity)
        : mass(mass), radius(radius), position(init_position), velocity(init_velocity)
    {}
    Body(float mass, float radius, float a, float e, float mu, float loan, float ap, float i)
        : mass(mass), radius(radius), semiMajorAxis(a), eccentricity(e), trueAnomaly(mu), longitudeOfAscendingNode(loan), argumentOfPeriapsis(ap), inclination(i)
    {}

    void determine_orbit(double centralBodyMass);
    void determine_state_vector(double centralBodyMass);
    vec3 calculatePosition(double centralBodyMass);
    vec3 calculateVelocity(double centralBodyMass);
};

struct Planet {
    // @todo planetary parameters

    ColorF color = {};
    Body body = {};

    Planet() {}
    Planet(ColorF color, Body body) : color(color), body(body) {}
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

StarSystem get_default_star_system();

#endif // _SPACE_H