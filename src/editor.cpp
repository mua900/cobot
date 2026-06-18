#include "editor.hpp"

bool VehicleEditor::place_part(cobot::vec2 where, PartKindId partKind)
{
    AttachmentDistance dist = vehicle.getAttachmentPointClosest(where, 20);
    if (dist.point)
    {
        

        return true;
    }
    else
    {
        return false;
    }
}
