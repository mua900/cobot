#ifndef _SPACE_H
#define _SPACE_H

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"

// Keplerian orbit (in 2 dimensions)
// distance(trueAnomaly) = semiMajorAxis*(1-eccentricity**2) / (1 + eccentricity*cos(semiMajorAxis))

// orbitting body
struct Body {
    float mass = 0;
    float radius = 0;

    float semiMajorAxis = 0;
    float eccentricity = 0;
    float trueAnomaly = 0;  // angular distance to periapsis
    float inclination = 0;  // angle between the orbital plane of the body and the reference plane
    float periapsisArgument = 0;  // the angle between the reference direction and the periapsis
    float ascendingNodeLongtitude = 0;  // the angle between the reference direction and the ascending node

    double calculateDistance()
    {
        return semiMajorAxis * (1.0 - eccentricity * eccentricity) / (1.0 + eccentricity * std::cos(trueAnomaly));
    }
};

struct Planet {
    // @todo planetary parameters

    Body body = {};
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

void draw_star_system();

#endif // _SPACE_H