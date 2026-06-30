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

Chassis getChassis()
{
    int hDist = 25;
    int vDist = 36;

    Chassis c;
    c.frontLeft.position = cobot::vec2(-hDist, vDist);
    c.frontRight.position = cobot::vec2(hDist, vDist);
    c.backLeft.position = cobot::vec2(-hDist, -vDist);
    c.backRight.position = cobot::vec2(hDist, -vDist);
	return c;
}


const char* get_structure_part_name(StructurePartKind kind) {
    switch (kind)
    {
        case StructurePartChassis:
            return "Chassis";
        case StructurePartSentinel:
        default:
            panic("Invalid structure part type");
    }
    
}

const char* get_ground_part_name(GroundPartKind kind) {
    switch (kind)
    {
        case GroundPartWheel:
        {
            return "Wheel";
        }
        case GroundPartSentinel:
        default:
            panic("Invalid ground part type");
    }
}

const char* get_computer_part_name(ComputerKind kind) {
    switch (kind)
    {
        case ComputerBasic:
            return "BasicComputer";
        case ComputerSentinel:
        default:
            panic("Invalid computer part type");
    }
}


cobot::vec2 get_part_scale(PartKindId id)
{
    PartKind kind = get_part_kind(id);
    u16 subKind = get_subkind(id);
    switch (kind)
    {
        case PartGround:     return get_ground_part_scale(GroundPartKind(subKind));
        case PartStructure:  return get_structure_part_scale(StructurePartKind(subKind));
        case PartComputer:   return get_computer_part_scale(ComputerKind(subKind));
        default: panic("Unhandled part kind");
    }
}

cobot::vec2 get_structure_part_scale(StructurePartKind kind) {
    return cobot::vec2(100, 100);
}

cobot::vec2 get_ground_part_scale(GroundPartKind kind) {
    return cobot::vec2(25, 25);
}

cobot::vec2 get_computer_part_scale(ComputerKind kind) {
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

bool load_ground_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)GroundPartKindCount; i++)
    {
        const char* name = get_ground_part_name(GroundPartKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PartGround, i)));
    }

    return true;
}

bool load_structure_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)StructurePartKindCount; i++)
    {
        const char* name = get_structure_part_name(StructurePartKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PartStructure, i)));
    }

    return true;
}

bool load_computer_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)ComputerKindCount; i++)
    {
        const char* name = get_computer_part_name(ComputerKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PartComputer, i)));
    }

    return true;
}
