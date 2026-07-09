#include "editor.hpp"
#include "log.hpp"

void VehicleEditor::set_part_position(cobot::vec2 where, AttachmentDistance dist, VPartData& partData, bool firstRoot)
{
    if (dist.point)
    {
        partData.parent = dist.parent;
    }

    if (rootPart)
    {
        partData.transform.position = firstRoot ? cobot::vec2() : where - vehicle.worldPosition;
    }
}

bool VehicleEditor::check_placement(AttachmentDistance dist, const char** errorMessage)
{
    if (dist.point && rootPart)
    {
        // trying to attach a root part to an attachment point
        *errorMessage = "Root parts can not be attached to other parts";
        return false;
    }
    if (!(dist.point || rootPart))
    {
        // trying to attach a part that isn't root and also isn't attached to anything
        *errorMessage = "Non-root parts must attach to something or they are not a part of the vehicle";
        return false;
    }

    return true;
}

bool VehicleEditor::place_part(cobot::vec2 where, const char** errorMessage)
{
    if (!haveSelectedPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    if (!check_placement(dist, errorMessage))
    {
        vehicle.remove_part(selectedPart);

#if EDITOR_DEBUG
        log_debug("Removed part: %u", selectedPart);
#endif

        return false;
    }

    bool firstRootPart = rootPart && (vehicle.rootParts.count() == 0);
    VehiclePart& part = vehicle.get_part(selectedPart);

    set_part_position(where, dist, part.partData, firstRootPart);

    if (dist.point)
    {
        dist.point->attach(selectedPart);
    }

    if (firstRootPart)
    {
        vehicle.worldPosition = where;
    }
    
    if (rootPart)
    {
        vehicle.add_root(selectedPart);
    }

#if EDITOR_DEBUG
    log_debug("Attached part: %u", selectedPart);
#endif

    return true;
}

bool VehicleEditor::place_editor_part(cobot::vec2 where, const char** errorMessage)
{
    if (!haveSelectedEditorPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    PartKind kind = selectedEditorPartKind;

    if (!check_placement(dist, errorMessage))
    {
        return false;
    }

    bool firstRootPart = rootPart && (vehicle.rootParts.count() == 0);
    VehiclePart part (kind);
    part.init();

    set_part_position(where, dist, part.partData, firstRootPart);

    PartId id = vehicle.add_part(part);

    if (dist.point)
    {
        dist.point->attach(id);
    }
    
    if (firstRootPart)
    {
        vehicle.worldPosition = where;
    }

    if (rootPart)
    {
        vehicle.add_root(id);
    }

#if EDITOR_DEBUG
    log_debug("Attached part kind: %u", selectedEditorPartKind);
#endif

    return true;
}

void VehicleEditor::draw_selected_part(SDL_Texture* part, RenderContext& render, cobot::vec2 where)
{
    cobot::vec2 textureSize;
    SDL_GetTextureSize(part, &textureSize.x, &textureSize.y);

    cobot::RectangleRot area = {};
    if (haveSnap)
    {
        area = cobot::RectangleRot(snap.position, textureSize, snap.rotation);
    }
    else
    {
        area = cobot::RectangleRot(where, textureSize, 0);
    }

    cobot::Quad points = area.get_points();
    draw_quad_with_texture(render, points, part, cobot::ColorF(0.9,0.9,0.9,0.5));
}


bool input_mouse_vehicle_editor(VehicleEditor& editor, Input& input, const Camera* camera)
{
    cobot::vec2 ms = input.mouse.pos;
    cobot::vec2 mouseWorld = camera->screen_to_world(ms);

    PartId part = editor.vehicle.getPartAt(mouseWorld);
    if (part != NullPartId)
    {
        editor.vehicle.unattach_from_parent(part);

        editor.selectedPart = part;
        editor.haveSelectedPart = true;
        return true;
    }

    return false;
}

bool input_keyboard_vehicle_editor(VehicleEditor& editor, Input& input)
{
    return false;
}

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages)
{
	const Camera* camera = render.camera;
	ASSERT(camera);

    cobot::vec2 ws = render.render_size;

    auto mouseWorld = camera->screen_to_world(input.mouse.pos);

	draw_vehicle_editor_background(render, catalog, input, editor);
	
    if (editor.haveSelectedEditorPart)
    {
        SDL_Texture* texture = get_part_texture(editor.selectedEditorPartKind, catalog);
        editor.draw_selected_part(texture, render, mouseWorld);
    }
    else if (editor.haveSelectedPart)
    {
        SDL_Texture* texture = get_part_texture(editor.vehicle.get_part(editor.selectedPart).kind, catalog);
        editor.draw_selected_part(texture, render, mouseWorld);
    }

    draw_vehicle(render, catalog, editor.vehicle, VehicleDrawParameters(&partImages, NullPartId, true));
}

void draw_vehicle_editor_background(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor)
{
	draw_arc(render, cobot::vec2(0,0), 300, 320, 10 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
	draw_arc(render, cobot::vec2(0,0), 300, 320, 190 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
}
