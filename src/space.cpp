#include "space.hpp"

#include <cmath>

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

StarSystem get_default_star_system(SDL_Renderer* renderer)
{
    StarSystem system = {};
    system.star.mass = 1'000;
    system.star.radius = 100;

    Body body = Body(1, 60, 300, 0, 0, 0, 0, 0.2);
    body.determine_state_vector(system.star.mass);
    ColorF p1color(0.5, 0.3, 0.6);
    Planet p1 =  Planet(String("Everest"), p1color, body);
    p1.map = generate_map(renderer, 543, 256, 256, 0.1, p1color);
    system.planets.add(p1);

    body = Body(0.2, 40, 300, 0.4, 0, 0.3, 0.4, 0.6);
    body.determine_state_vector(system.star.mass);
    ColorF p2color(0.3, 0.8, 0.5);
    Planet p2 = Planet(String("Erciyes"), p2color, body);
    p2.map = generate_map(renderer, 734, 256, 256, 0.1, p2color);
    system.planets.add(p2);

    body = Body(0.7, 50, 300, 0.5, 0, 0, 0, 0);
    body.determine_state_vector(system.star.mass);
    ColorF p3color(0.8, 0.5, 0.5);
    Planet p3 = Planet(String("Illimani"), p3color, body);
    p3.map = generate_map(renderer, 173, 256, 256, 0.1, p3color);
    system.planets.add(p3);

    return system;
}

void Body::determine_orbit(double centralBodyMass)
{
    // https://en.wikipedia.org/wiki/Orbit_determination#Methods
    double gravitationalParameter = G * centralBodyMass;
    vec3 specificOrbitalMomentum = cross3(position, velocity);
    double orbitalMomentum = specificOrbitalMomentum.magnitude();
    vec3 ascendingNodeVector = cross3(vec3(0,0,1), specificOrbitalMomentum);
    double ascendingNodeMagnitude = ascendingNodeVector.magnitude();
    vec3 eccentricityVector = (cross3(velocity, specificOrbitalMomentum) / gravitationalParameter) - position.normalized();
    double eccentricity = eccentricityVector.magnitude();
    double p = (orbitalMomentum * orbitalMomentum) / gravitationalParameter;
    double semiMajorAxis = p / (1.0 - eccentricity * eccentricity);
    double inclination = std::acos(dot3(vec3(0,0,1), specificOrbitalMomentum) / orbitalMomentum);
    // https://en.wikipedia.org/wiki/Longitude_of_the_ascending_node#Calculation_from_state_vectors
    double ascendingNodeLongtitude = (ascendingNodeMagnitude == 0) ? 0 : std::acos(ascendingNodeVector.x / ascendingNodeMagnitude);
    if (ascendingNodeVector.y < 0) ascendingNodeLongtitude = CONSTANT_TAU - ascendingNodeLongtitude;
    double periapsisArgument = (eccentricity == 0 || ascendingNodeMagnitude == 0) ? 0 : std::acos(dot3(ascendingNodeVector, eccentricityVector) / (ascendingNodeMagnitude * eccentricity));

    double dir = dot3(velocity, position);
    double trueAnomaly = (eccentricity == 0) ? 0 : std::acos(dot3(eccentricityVector, position) / (eccentricity * position.magnitude()));
    if (dir < 0.0) trueAnomaly = CONSTANT_TAU - trueAnomaly;

    this->parameters.semiMajorAxis = semiMajorAxis;
    this->parameters.eccentricity = eccentricity;
    this->parameters.inclination = inclination;
    this->parameters.longitudeOfAscendingNode = ascendingNodeLongtitude;
    this->parameters.argumentOfPeriapsis = periapsisArgument;
    this->parameters.trueAnomaly = trueAnomaly;
}

void Body::determine_state_vector(double centralBodyMass)
{
    // https://en.wikipedia.org/wiki/Perifocal_coordinate_system
    double gravitationalParameter = centralBodyMass * G;
    double r = parameters.semiMajorAxis * (1.0 - parameters.eccentricity * parameters.eccentricity) / (1.0 + parameters.eccentricity * std::cos(parameters.trueAnomaly));
    double p = parameters.semiMajorAxis * (1.0 - parameters.eccentricity * parameters.eccentricity);  // semi parameter
    // https://en.wikipedia.org/wiki/Specific_angular_momentum#Third_law
    double specificOrbitalMomentum = std::sqrt(gravitationalParameter * p);
    vec3 perifocalPosition = vec3(std::cos(parameters.trueAnomaly), std::sin(parameters.trueAnomaly), 0) * r;
    vec3 perifocalVelocity = vec3(-std::sin(parameters.trueAnomaly), parameters.eccentricity + std::cos(parameters.trueAnomaly), 0) * (gravitationalParameter / specificOrbitalMomentum);

    // rotation from perifocal coordinate system to cartesian coordinate system
    // rot = rot_z(LOAN) * rot_x(inclination) * rot_z(AOP)

    mat3x3 rotation = {};
    mat3x3 rot = {};
    get_rotation_z(&rotation, parameters.argumentOfPeriapsis);
    get_rotation_x(&rot, parameters.inclination);
    mat3mul(&rotation, &rot, &rotation);
    get_rotation_z(&rot, parameters.longitudeOfAscendingNode);
    mat3mul(&rotation, &rot, &rotation);

    vec3 position = mat3apply(&rotation, perifocalPosition);
    vec3 velocity = mat3apply(&rotation, perifocalVelocity);

    this->position = position;
    this->velocity = velocity;
}
