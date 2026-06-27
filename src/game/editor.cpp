#include "editor.hpp"

bool VehicleEditor::place_part(cobot::vec2 where, const char** errorMessage)
{
    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);

    if (!haveSeletedPart)
    {
        *errorMessage = "No part selected";
        return false;
    }

    PartKindId partKind = selectedPartKind;

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

    PartKind kind = get_part_kind(partKind);
    u16 subkind = get_subkind(partKind);

    PartId id = {};

    bool firstRootPart = rootPart && (vehicle.rootParts.size() == 0);
    VPartData partData = VPartData();

    if (dist.point)
    {
        partData.parent = dist.parent;
    }

    if (rootPart)
    {
        partData.transform.position = firstRootPart ? cobot::vec2() : where - vehicle.worldPosition;
    }

    switch (kind)
    {
        case PartStructure:
        {
            StructurePart structure = StructurePart();
            structure.kind = StructurePartKind(subkind);
            structure.part = partData;
            switch (subkind)
            {
                case StructurePartChassis:
                {
                    structure.chassis = getChassis();
                    break;
                }
            }
            id = vehicle.add_structure_part(structure);
            break;
        }
        case PartComputer:
        {
            Computer computer = Computer();
            computer.kind = ComputerKind(subkind);
            computer.part = partData;
            id = vehicle.add_computer_part(computer);
            break;
        }
        case PartGround:
        {
            GroundPart ground = GroundPart();
            ground.kind = GroundPartKind(subkind);
            ground.part = partData;
            id = vehicle.add_ground_part(ground);
            break;
        }
        default:
            panic("Unknown part kind");
    }

    if (dist.point)
    {
        dist.point->attach(id);
    }
    
    if (rootPart)
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
    cobot::vec2 ws = render.render_size;

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

    draw_circle(render, cobot::vec2(ws.x * 0.5, ws.y * 0.95), ws.y * 0.04, editor.rootPart ? cobot::ColorF(0.2, 0.6, 0.2) : cobot::ColorF(0.7, 0.3, 0.1));
}
