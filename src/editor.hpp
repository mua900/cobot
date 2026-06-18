#ifndef _EDITOR_H
#define _EDITOR_H

#include "vehicle.hpp"

struct VehicleEditor {
    Vehicle vehicle = {};
    VehiclePart selectedPart = {};

    bool place_part(cobot::vec2 where, PartKindId partKind);
};

#endif // _EDITOR_H