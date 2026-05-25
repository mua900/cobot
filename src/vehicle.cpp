#include "vehicle.hpp"

VPartTransform chain_part_transform(VPartTransform p0, VPartTransform p1)
{
    return VPartTransform(p0.position + p1.position, p0.scale * p1.scale);
}

VPartTransform Vehicle::getWorldTransform(PartId part) const
{
    VPartData data = getPartData(part);
    VPartTransform t = data.transform;
    while (data.parent.is_valid()) {
        data = getPartData(data.parent);
        t = chain_part_transform(t, data.transform);
    }

    t = chain_part_transform(t, VPartTransform(worldPosition, 1.0f));

    return t;
}

PartId& Vehicle::getParentRef(PartId part)
{
    VPartData& data = getPartData(part);
    return data.parent;
}

VPartData& Vehicle::getPartData(PartId id) const
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

int Vehicle::add_tire(Tire& t, PartId parent) {
    if (parent.is_valid()) {
        volume = merge_volumes(volume, Rectangle(t.part.transform.position, t.part.transform.scale));
    }
    return tire.add(t);
}

int Vehicle::add_chasis(Chasis& c, PartId parent) {
    if (parent.is_valid()) {
        volume = merge_volumes(volume, Rectangle(c.part.transform.position, c.part.transform.scale));
    }
    return chasis.add(c);
}

int Vehicle::add_controller(Controller& c, PartId parent) {
    if (parent.is_valid()) {
        volume = merge_volumes(volume, Rectangle(c.part.transform.position, c.part.transform.scale));
    }
    return controller.add(c);
}

int Vehicle::add_root(PartId part)
{
    switch (part.kind)
    {
        case PART_CHASIS: {
            Chasis& cha = chasis[part.index];
            if (cha.part.parent.is_valid()) {
                return -1;
            }

            switch (cha.kind) {
                case ChasisBasic:
                {
                    getParentRef(cha.basic.frontLeft.part) = part;
                    getParentRef(cha.basic.frontRight.part) = part;
                    getParentRef(cha.basic.backLeft.part) = part;
                    getParentRef(cha.basic.backRight.part) = part;
                    break;
                }
                default: panic("Invalid chasis kind");
            }
            
            return rootParts.add(part);
        }
        case PART_TIRE: {
            return -1;
        }
        case PART_CONTROLLER: {
            return -1;
        }
        default: panic("Invalid part type");
    }
}


Vehicle get_default_vehicle()
{
    Vehicle vehicle = {};

    vehicle.worldPosition = vec2(600, 300);
    vehicle.volume = Rectangle(vehicle.worldPosition, vec2());

    Tire tires[4] = {};
    for (auto& t : tires) {
        t.kind = TireBasic;
        t.part.transform.scale = 1;
        t.basic.size = 5;
    }

    tires[0].part.transform.position = vec2(-25, -25);
    tires[1].part.transform.position = vec2(25, -25);
    tires[2].part.transform.position = vec2(-25, 25);
    tires[3].part.transform.position = vec2(25, 25);

    int fl = vehicle.add_tire(tires[0], NullPartId);
    int fr = vehicle.add_tire(tires[1], NullPartId);
    int bl = vehicle.add_tire(tires[2], NullPartId);
    int br = vehicle.add_tire(tires[3], NullPartId);

    Chasis chasis = {};
    chasis.kind = ChasisBasic;
    chasis.part.transform.scale = 1;
    chasis.basic.frontLeft.attach(PartId(PART_TIRE, fl));
    chasis.basic.frontRight.attach(PartId(PART_TIRE, fr));
    chasis.basic.backLeft.attach(PartId(PART_TIRE, bl));
    chasis.basic.backRight.attach(PartId(PART_TIRE, br));

    int chasis_idx = vehicle.add_chasis(chasis, NullPartId);

    Controller con = {};
    con.kind = ControllerBasic;
    con.part.transform.scale = 1;
    con.basic.codeSizeLimit = 128;
    con.basic.script = {};
    vehicle.add_controller(con, NullPartId);

    vehicle.add_root(PartId(PART_CHASIS, chasis_idx));

    return vehicle;
}
