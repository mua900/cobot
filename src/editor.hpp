#ifndef _EDITOR_H
#define _EDITOR_H

#include "vehicle.hpp"

struct VehicleEditor {
    Vehicle vehicle = {};
    VehiclePart selectedPart = {};
};

#endif // _EDITOR_H