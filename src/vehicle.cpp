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
    Vehicle vehicle;

    BasicChasis chasis = {};
    chasis.scale = 10;

    return vehicle;
}
