#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "app/input.hpp"
#include "app/draw.hpp"
#include "vehicle.hpp"

struct VehicleEditor {
    Vehicle vehicle = {};
    VPartTransform snap = {};
    bool haveSnap = false;
    bool rootPart = false;

    // from vehicle
    PartId selectedPart = {};
    bool haveSelectedPart = false;

    // from editor
    PartKind selectedEditorPartKind = {};
    bool haveSelectedEditorPart = false;

    bool place_part(cobot::vec2 where, const char** errorMessage);

    void draw_selected_part(SDL_Texture* part, RenderContext& render, cobot::vec2 where);
};

bool input_mouse_vehicle_editor(VehicleEditor& editor, Input& input, const Camera* camera);
bool input_keyboard_vehicle_editor(VehicleEditor& editor, Input& input);

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages);
void draw_vehicle_editor_background(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor);

#endif // EDITOR_HPP
