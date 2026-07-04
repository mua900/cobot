#ifndef MISSION_HPP
#define MISSION_HPP

#include "common.hpp"
#include "vehicle.hpp"
#include "space.hpp"
#include "map.hpp"

struct Mission {
    VehicleId vehicle = NullVehicleId;
    PlanetId planet = NullPlanetId;
    String name = {};
    String objective = {};

    // where on the planet we are?
    float latitude = 0;
    float longitude = 0;

    String_Builder buffer = {};

    bool is_valid() const {
        return vehicle != NullVehicleId && planet != NullPlanetId;
    }
};

#endif // MISSION_HPP