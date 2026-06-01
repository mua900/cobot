#ifndef _SPACE_H
#define _SPACE_H

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"

// @todo a way to convert between position + velocity and orbital elements which are more intuative
// Keplerian orbit (in 2 dimensions)
// distance(trueAnomaly) = semiMajorAxis*(1-eccentricity**2) / (1 + eccentricity*cos(semiMajorAxis))

// orbitting body
struct Body {
    float mass = 0;
    float radius = 0;

    vec3 position = {};
    vec3 velocity = {};
    vec3 acceleration = {};

    Body() {}
    Body(float mass, float radius, vec3 init_position, vec3 init_velocity)
        : mass(mass), radius(radius), position(init_position), velocity(init_velocity)
    {}
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

void draw_star_system();

#endif // _SPACE_H