#include "vehicle.hpp"

VPartData chain_part_data(VPartData p0, VPartData p1)
{
    return VPartData(p0.position + p1.position, p0.scale * p1.scale);
}

VPartData Vehicle::getPartData(PartId id) const
{
    switch (id.kind) {
        case PART_CHASIS: {
            return chasis[id.index].part;
        }
        case PART_TIRE: {
            return controller[id.index].part;
        }
        case PART_CONTROLLER: {
            return tire[id.index].part;
        }
        default: panic("Invalid part type");
    }
}

const char* get_chasis_name(ChasisKind kind) {
    return "BasicChasis";
}

const char* get_tire_name(TireKind kind) {
    return "BasicTire";
}

const char* get_controller_name(ControllerKind kind) {
    return "BasicController";
}


vec2 get_chasis_scale(ChasisKind kind) {
    return vec2(100, 100);
}

vec2 get_tire_scale(TireKind kind) {
    return vec2(25, 25);
}

vec2 get_controller_scale(ControllerKind kind) {
    return vec2(10, 10);
}


bool load_tire_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)TireKindCount; i++)
    {
        const char* name = get_tire_name(TireKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, getPartKindId(PART_TIRE, i)));
    }

    return true;
}

bool load_chasis_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog) {
    for (int i = 0; i < (int)ChasisKindCount; i++)
    {
        const char* name = get_chasis_name(ChasisKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, getPartKindId(PART_CHASIS, i)));
    }

    return true;
}

bool load_controller_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog) {
    for (int i = 0; i < (int)ControllerKindCount; i++)
    {
        const char* name = get_controller_name(ControllerKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, getPartKindId(PART_CONTROLLER, i)));
    }

    return true;
}

PartKindId getPartKindId(PartKind partKind, u16 subType)
{
    return (partKind << 16) | (subType);
}


Vehicle get_default_vehicle()
{
    Vehicle vehicle = {};

    vehicle.worldPosition = vec2(600, 300);

    Tire tires[4] = {};
    for (auto& t : tires) {
        t.kind = TireBasic;
        t.part.scale = 1;
        t.basic.size = 5;
    }

    tires[0].part.position = vec2(-25, -25);
    tires[1].part.position = vec2(25, -25);
    tires[2].part.position = vec2(-25, 25);
    tires[3].part.position = vec2(25, 25);

    int fl = vehicle.tire.add(tires[0]);
    int fr = vehicle.tire.add(tires[1]);
    int bl = vehicle.tire.add(tires[2]);
    int br = vehicle.tire.add(tires[3]);

    Chasis chasis = {};
    chasis.kind = ChasisBasic;
    chasis.part.scale = 1;
    chasis.basic.frontLeft.attach(PartId(PART_TIRE, fl));
    chasis.basic.frontRight.attach(PartId(PART_TIRE, fr));
    chasis.basic.backLeft.attach(PartId(PART_TIRE, bl));
    chasis.basic.backRight.attach(PartId(PART_TIRE, br));

    int chasis_idx = vehicle.chasis.add(chasis);

    Controller con = {};
    con.kind = ControllerBasic;
    con.part.scale = 1;
    con.basic.codeSizeLimit = 128;
    con.basic.script = 0;  // @todo
    vehicle.controller.add(con);

    vehicle.rootParts.add(PartId(PART_CHASIS, chasis_idx));

    return vehicle;
}
