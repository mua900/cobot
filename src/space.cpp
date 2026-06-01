#include "space.hpp"

// Gravitational constant in m^3*kg^-1*s^-2
constexpr double G = 6.6743e-11;

// acceleration = - G * M / r**3
void StarSystem::simulation_step(double dt)
{
    for (auto& planet : this->planets)
    {
        Body body = planet.body;
        double distance = body.calculateDistance();
        double acceleration = - (G * star.mass) / pow(distance, 3);

        
    }
}
