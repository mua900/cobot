#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "app/input.hpp"
#include "app/draw.hpp"
#include "vehicle.hpp"

struct VehicleEditor {
    Vehicle vehicle = {};
    PartKindId selectedPartKind = {};
    bool haveSeletedPart = false;
    VPartTransform snap = {};
    bool haveSnap = false;
    bool rootPart = false;

    bool place_part(cobot::vec2 where);
};

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages);

#endif // EDITOR_HPP