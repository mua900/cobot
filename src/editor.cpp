#include "editor.hpp"

bool VehicleEditor::place_part(cobot::vec2 where, PartKindId partKind)
{
    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);
    if (dist.point)
    {
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

        dist.point->attach(id);

        return true;
    }
    else
    {
        return false;
    }
}
