#include "space.hpp"

// Gravitational constant in m^3*kg^-1*s^-2
constexpr double G = 6.6743e-11;

// acceleration = - G * M / r**3
void StarSystem::simulation_step(double dt)
{
    for (auto& planet : this->planets)
    {
        Body& body = planet.body;
        double distance = body.position.magnitude();
        double accel = - (G * star.mass) / pow(distance, 3);
        vec3 acceleration = body.position * accel;

        body.position += body.velocity * dt + body.acceleration * (dt * dt * 0.5);
        body.velocity += (acceleration + body.acceleration) * (dt * 0.5);
        body.acceleration = acceleration;
    }
}

StarSystem get_default_star_system()
{
    StarSystem system = {};
    system.star.mass = 1000;
    system.star.radius = 100;
    system.planets.add(Planet(ColorF(0.5, 0.3, 0.6), Body(1, 50, vec3(200, 0, 0), vec3(0, 300, 0))));
    return system;
}