#include "editor.hpp"

bool VehicleEditor::place_part(cobot::vec2 where, PartKindId partKind, bool root)
{
    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    PartKind kind = get_part_kind(partKind);
    u16 subkind = get_subkind(partKind);

    PartId id = {};

    switch (kind)
    {
        case PART_CHASSIS:
        {
            Chassis chassis = Chassis();
            chassis.kind = ChassisKind(subkind);
            chassis.part.parent = dist.parent;
            switch (subkind)
            {
                case ChassisBasic:
                {
                    chassis.basic = getBasicChassis();
                    break;
                }
            }
            id = vehicle.add_chassis(chassis);
            break;
        }
        case PART_CONTROLLER:
        {
            Controller controller = Controller();
            controller.kind = ControllerKind(subkind);
            controller.part.parent = dist.parent;
            id = vehicle.add_controller(controller);
            break;
        }
        case PART_TIRE:
        {
            Tire tire = Tire();
            tire.kind = TireKind(subkind);
            tire.part.parent = dist.parent;
            id = vehicle.add_tire(tire);
            break;
        }
        default:
            break;
    }

    if (dist.point && root)
    {
        // trying to attach a root part to an attachment point
        return false;
    }
    if (!(dist.point || root))
    {
        // trying to attach a part that isn't root and also isn't attached to anything
        return false;
    }

    if (dist.point)
    {
        dist.point->attach(id);
    }
    
    if (root)
    {
        if (vehicle.rootParts.size() == 0)
        {
            vehicle.worldPosition = where;
        }

        vehicle.add_root(id);
    }

    return true;
}

void draw_veditor(RenderContext& render, AssetCatalog& catalog, Input& input, VehicleEditor& editor, VPartImages& partImages)
{
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
            area = cobot::RectangleRot(input.mouse.pos, cobot::vec2(100, 100), 0);
        }

        cobot::Quad points = area.get_points();
        draw_quad_with_texture(render, points, texture, cobot::ColorF(0.9,0.9,0.9,0.5));
    }

    draw_vehicle(render, catalog, editor.vehicle, VehicleDrawParameters(&partImages, NullPartId, true));
}
