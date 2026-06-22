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
