#include "parts.hpp"
#include "util/log.hpp"

VPartTransform chain_part_transform(VPartTransform parent, VPartTransform child)
{
    cobot::vec2 position = parent.position + (child.position * parent.scale).rotated(parent.rotation);
    float rotation = parent.rotation + child.rotation;
    float scale = parent.scale * child.scale;
    return VPartTransform(position, rotation, scale);
}

VPartTransform VPartTransform::inverse() const
{
    return VPartTransform(-position, -rotation, 1.0 / scale);
}

void VehiclePart::init()
{
    if (kind == PartKindChassis)
    {
        data.chassis.init();
    }
}

Array<AttachmentPoint> VehiclePart::getAttachments()
{
    if (kind == PartKindChassis)
    {
        return Array<AttachmentPoint>(data.chassis.points, ChassisAttachmentCount);
    }

    return {};
}

void Chassis::init()
{
    int hDist = 25;
    int vDist = 36;

    points[ChassisFrontLeft] = AttachmentPoint(cobot::vec2(-hDist, vDist), NullPartId);
    points[ChassisFrontRight] = AttachmentPoint(cobot::vec2(hDist, vDist), NullPartId);
    points[ChassisBackLeft] = AttachmentPoint(cobot::vec2(-hDist, -vDist), NullPartId);
    points[ChassisBackRight] = AttachmentPoint(cobot::vec2(hDist, -vDist), NullPartId);
    points[ChassisTop] = AttachmentPoint(cobot::vec2(0, 0), NullPartId);
}


PartCategory getPartCategory(PartKind kind)
{
    switch (kind)
    {
        case PartKindSolarPanel:    return CategoryPower;
        case PartKindBattery:       return CategoryPower;
        case PartKindWheel:         return CategoryGround;
        case PartKindChassis:       return CategoryStructure;
        case PartKindComputer:      return CategoryComputer;
        case PartKindThermometer:   return CategoryInstrument;
        case PartKindLidar:         return CategoryInstrument;

        case PartKindSentinel:  // fallthrough
        default:
            panic("Invalid part type");
    }
}

const char* get_part_name(PartKind kind) {
    switch (kind)
    {
        case PartKindSolarPanel:    return "SolarPanel";
        case PartKindBattery:       return "Battery";
        case PartKindWheel:         return "Wheel";
        case PartKindChassis:       return "Chassis";
        case PartKindComputer:      return "BasicComputer";
        case PartKindThermometer:   return "Thermometer";
        case PartKindLidar:         return "Lidar";

        case PartKindSentinel:  // fallthrough
        default:
            panic("Invalid part type");
    }
}

cobot::vec2 get_part_scale(PartKind kind)
{
    switch (kind)
    {
        // @todo sensible numbers
        case PartKindSolarPanel:    return cobot::vec2(50, 50);
        case PartKindBattery:       return cobot::vec2(40, 40);
        case PartKindWheel:         return cobot::vec2(25, 25);
        case PartKindChassis:       return cobot::vec2(100, 100);
        case PartKindComputer:      return cobot::vec2(10, 10);
        case PartKindThermometer:   return cobot::vec2(10, 10);
        case PartKindLidar:         return cobot::vec2(10, 15);

        case PartKindSentinel:  // fallthrough
        default: panic("Invalid part type");
    }
}

bool load_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < PartKindCount; i++)
    {
        const char* name = get_part_name(PartKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, i));
    }

    return true;
}
