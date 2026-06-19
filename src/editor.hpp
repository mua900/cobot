#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "vehicle.hpp"

struct EditorContext {
    PartKindId selectedPartKind = {};
    bool haveSeletedPart = false;
    bool rootPart = false;
};

struct VehicleEditor {
    Vehicle vehicle = {};
    VehiclePart selectedPart = {};

    bool place_part(cobot::vec2 where, PartKindId partKind, bool root);
};

#endif // EDITOR_HPP