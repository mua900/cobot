#include "parts.hpp"

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

BasicChassis getBasicChassis()
{
    int hDist = 25;
    int vDist = 36;

    BasicChassis c;
    c.frontLeft.position = cobot::vec2( hDist, -vDist);
    c.frontRight.position = cobot::vec2( hDist,  vDist);
    c.backLeft.position = cobot::vec2(-hDist, -vDist);
    c.backRight.position = cobot::vec2(-hDist,  vDist);
	return c;
}


const char* get_chassis_name(ChassisKind kind) {
    return "BasicChassis";
}

const char* get_tire_name(TireKind kind) {
    return "BasicTire";
}

const char* get_controller_name(ControllerKind kind) {
    return "BasicController";
}


cobot::vec2 get_part_scale(PartKindId id)
{
    PartKind kind = get_part_kind(id);
    u16 subKind = get_subkind(id);
    switch (kind)
    {
        case PART_TIRE:         return get_tire_scale(TireKind(subKind));
        case PART_CHASSIS:       return get_chassis_scale(ChassisKind(subKind));
        case PART_CONTROLLER:   return get_controller_scale(ControllerKind(subKind));
        default: panic("Unhandled part kind");
    }
}

cobot::vec2 get_chassis_scale(ChassisKind kind) {
    return cobot::vec2(100, 100);
}

cobot::vec2 get_tire_scale(TireKind kind) {
    return cobot::vec2(25, 25);
}

cobot::vec2 get_controller_scale(ControllerKind kind) {
    return cobot::vec2(10, 10);
}


u16 get_subkind(PartKindId kindId)
{
    return kindId & 0xffff;
}

PartKind get_part_kind(PartKindId kindId)
{
    return PartKind((kindId >> 16) & 0xffff);
}

PartKindId get_part_kind_id(PartKind partKind, u16 subType)
{
    return (partKind << 16) | (subType);
}
