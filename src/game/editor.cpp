#include "editor.hpp"

bool VehicleEditor::place_part(cobot::vec2 where, const char** errorMessage)
{
    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    if (!haveSeletedPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    PartKind kind = selectedPartKind;

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

    bool firstRootPart = rootPart && (vehicle.rootParts.count() == 0);
    VPartData partData = VPartData();

    if (dist.point)
    {
        partData.parent = dist.parent;
    }

    if (rootPart)
    {
        partData.transform.position = firstRootPart ? cobot::vec2() : where - vehicle.worldPosition;
    }

    VehiclePart part(kind);
    part.partData = partData;
    part.init();

    PartId id = vehicle.add_part(part);

    if (dist.point)
    {
        dist.point->attach(id);
    }
    
    if (rootPart)
    {
        if (vehicle.rootParts.count() == 0)
        {
            vehicle.worldPosition = where;
        }

        vehicle.add_root(id);
    }

    return true;
}

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages)
{
	const Camera* camera = render.camera;
	ASSERT(camera);

    cobot::vec2 ws = render.render_size;

	draw_vehicle_editor_background(render, catalog, input, editor);
	
    if (editor.haveSeletedPart)
    {
        SDL_Texture* texture = get_part_texture(editor.selectedPartKind, catalog);
    
        cobot::RectangleRot area = {};
        if (editor.haveSnap)
        {
            area = cobot::RectangleRot(editor.snap.position, cobot::vec2(100, 100), editor.snap.rotation);
        }
        else
        {
            area = cobot::RectangleRot(camera->screen_to_world(input.mouse.pos), cobot::vec2(100, 100), 0);
        }

        cobot::Quad points = area.get_points();
        draw_quad_with_texture(render, points, texture, cobot::ColorF(0.9,0.9,0.9,0.5));
    }

    draw_vehicle(render, catalog, editor.vehicle, VehicleDrawParameters(&partImages, NullPartId, true));
}

void draw_vehicle_editor_background(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor)
{
	draw_arc(render, cobot::vec2(0,0), 300, 320, 10 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
	draw_arc(render, cobot::vec2(0,0), 300, 320, 190 * cobot::DEGREE_TO_RADIAN_F, 160 * cobot::DEGREE_TO_RADIAN_F, cobot::ColorF(0.8,0.8,0.8));
}
