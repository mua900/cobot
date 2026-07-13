#ifndef MISSION_HPP
#define MISSION_HPP

#include "util/common.hpp"
#include "vehicle.hpp"
#include "space.hpp"
#include "map.hpp"

struct Mission {
    VehicleId vehicle = NullVehicleId;
    PlanetId planet = NullPlanetId;
    String name = {};
    String description = {};

    // where on the planet we are?
    double latitude;
    double longitude;
    // unit 3d vector point to the position
    cobot::vec3d position;

    String_Builder buffer = {};

    bool is_valid() const {
        return vehicle != NullVehicleId && planet != NullPlanetId && name.size != 0;
    }
};

#define MISSION_ID_BIT BIT(16)

#endif // MISSION_HPP