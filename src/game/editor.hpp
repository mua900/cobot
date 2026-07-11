#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "app/input.hpp"
#include "app/draw.hpp"
#include "app/ui.hpp"
#include "vehicle.hpp"

#define EDITOR_DEBUG 0

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

    bool check_placement(AttachmentDistance dist, const char** errorMessage);
    void set_part_position(cobot::vec2 where, AttachmentDistance dist, VPartData& partData, bool firstRoot);

    bool place_part(cobot::vec2 where, const char** errorMessage);
    bool place_editor_part(cobot::vec2 where, const char** errorMessage);

    void draw_selected_part(SDL_Texture* part, RenderContext& render, cobot::vec2 where);
};

bool input_mouse_vehicle_editor(VehicleEditor& editor, Input& input, UiState& ui, const Camera* camera);
bool input_keyboard_vehicle_editor(VehicleEditor& editor, Input& input);

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages);
void draw_vehicle_editor_background(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor);

#endif // EDITOR_HPP
