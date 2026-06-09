#ifndef _MISSSION_H
#define _MISSION_H

#include "common.hpp"
#include "vehicle.hpp"
#include "space.hpp"

struct Mission {
    VehicleId vehicle = 0;
    PlanetId planet = {};
    String name = {};
    String objective = {};

    // where
    float latitude = 0;
    float longitude = 0;

    String_Builder buffer = {};
};

#endif // _MISSION_H