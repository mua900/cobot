#include "vehicle.hpp"

const char* get_chasis_name(ChasisKind kind) {
    return "BasicChasis";
}

const char* get_tire_name(TireKind kind) {
    return "BasicTire";
}

const char* get_controller_name(ControllerKind kind) {
    return "BasicController";
}


bool load_tire_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)TireKindCount; i++)
    {
        const char* name = get_tire_name(TireKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, getPartId(PART_TIRE, i)));
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

        icons.add(IconButton(texture, background, getPartId(PART_CHASIS, i)));
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

        icons.add(IconButton(texture, background, getPartId(PART_CONTROLLER, i)));
    }

    return true;
}

PartId getPartId(int partKind, int subType)
{
    return (partKind << 16) | (subType);
}


Vehicle get_default_vehicle()
{
    Vehicle vehicle = {};

    BasicTire tires[4] = {};
    for (auto& t : tires) {
        t.size = 5;
    }

    int fl = vehicle.tire.add(tires[0]);
    int fr = vehicle.tire.add(tires[1]);
    int bl = vehicle.tire.add(tires[2]);
    int br = vehicle.tire.add(tires[3]);

    BasicChasis chasis = {};
    chasis.scale = 10;
    chasis.frontLeft.attach(PART_TIRE, fl);
    chasis.frontRight.attach(PART_TIRE, fr);
    chasis.backLeft.attach(PART_TIRE, bl);
    chasis.backRight.attach(PART_TIRE, br);

    vehicle.chasis.add(chasis);

    BasicController con = {};
    con.codeSizeLimit = 128;
    con.script = 0;  // @todo
    vehicle.controller.add(con);

    return vehicle;
}
