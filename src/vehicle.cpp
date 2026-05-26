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
            return tire[id.index].part;
        }
        case PART_CONTROLLER: {
            return controller[id.index].part;
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


PartKindId getPartKindId(PartKind partKind, u16 subType)
{
    return (partKind << 16) | (subType);
}

PartId Vehicle::add_tire(Tire& t) {
    int index = tire.add(t);

    PartId thisPart = PartId(PART_TIRE, index);

    VPartTransform transform = getWorldTransform(PartId(PART_TIRE, index));
    volume = merge_volumes(volume, Rectangle(transform.position, transform.scale * get_tire_scale(t.kind)));

    return thisPart;
}

PartId Vehicle::add_chasis(Chasis& c) {
    
    int index = chasis.add(c);

    PartId thisPart = PartId(PART_CHASIS, index);
    switch (c.kind) {
        case ChasisBasic:
        {
            if (c.basic.frontLeft.part.is_valid())  getParentRef(c.basic.frontLeft.part) = thisPart;
            if (c.basic.frontRight.part.is_valid()) getParentRef(c.basic.frontRight.part) = thisPart;
            if (c.basic.backLeft.part.is_valid())   getParentRef(c.basic.backLeft.part) = thisPart;
            if (c.basic.backRight.part.is_valid())  getParentRef(c.basic.backRight.part) = thisPart;
            break;
        }
        default: panic("Invalid chasis kind");
    }

    VPartTransform transform = getWorldTransform(thisPart);
    volume = merge_volumes(volume, Rectangle(transform.position, transform.scale * get_chasis_scale(c.kind)));

    return thisPart;
}

PartId Vehicle::add_controller(Controller& c) {
    int index = controller.add(c);

    PartId thisPart = PartId(PART_CONTROLLER, index);

    VPartTransform transform = getWorldTransform(PartId(PART_CONTROLLER, index));
    volume = merge_volumes(volume, Rectangle(transform.position, transform.scale * get_controller_scale(c.kind)));

    return thisPart;
}

Tire* Vehicle::get_tire(PartId t) {
    if (t.kind != PART_TIRE) {
        return nullptr;
    }
    return tire.get_ptr(t.index);
}

Chasis* Vehicle::get_chasis(PartId c) {
    if (c.kind != PART_CHASIS) {
        return nullptr;
    }
    return chasis.get_ptr(c.index);
}

Controller* Vehicle::get_controller(PartId c) {
    if (c.kind != PART_CONTROLLER) {
        return nullptr;
    }
    return controller.get_ptr(c.index);
}

int Vehicle::add_root(PartId part)
{
    return rootParts.add(part);
}

PartId Vehicle::getPartAt(vec2 position) const
{
    position -= worldPosition;
    for (int i = 0; i < rootParts.size(); i++)
    {
        PartId part = get_part_on_location(rootParts[i], position, getPartData(rootParts[i]).transform);
        if (part.is_valid())
        {
            return part;
        }
    }

    return NullPartId;
}

PartId Vehicle::get_part_on_location(PartId part, vec2 location, VPartTransform parent) const
{
    switch (part.kind)
    {
        case PART_CHASIS: {
            Chasis& cha = chasis[part.index];
            VPartTransform t = chain_part_transform(parent, cha.part.transform);
            vec2 scale = get_chasis_scale(cha.kind);

            switch (cha.kind) {
                case ChasisBasic: {
                    PartId part;
                    part = get_part_on_location(cha.basic.frontLeft.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(cha.basic.frontRight.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(cha.basic.backLeft.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(cha.basic.backRight.part, location, t);
                    if (part.is_valid()) return part;
                    break;
                }
                default: panic("Invalid chasis kind");
            }

            if (Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        case PART_TIRE: {
            Tire& tr = tire[part.index];
            VPartTransform t = chain_part_transform(parent, tr.part.transform);
            vec2 scale = get_tire_scale(tr.kind);
            if (Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        case PART_CONTROLLER: {
            Controller& con = controller[part.index];
            VPartTransform t = chain_part_transform(parent, con.part.transform);
            vec2 scale = get_controller_scale(controller[part.index].kind);
            if (Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        default: panic("Invalid part type");
    }
}

Vehicle get_default_vehicle()
{
    Vehicle vehicle = {};

    vehicle.worldPosition = vec2(600, 300);
    vehicle.volume = Rectangle(vehicle.worldPosition, vec2());

    Chasis chasis = {};
    chasis.kind = ChasisBasic;
    chasis.part.transform.scale = 1;

    PartId chasis_id = vehicle.add_chasis(chasis);

    Tire tires[4] = {};
    for (auto& t : tires) {
        t.kind = TireBasic;
        t.part.transform.scale = 1;
        t.basic.size = 5;
        t.part.parent = chasis_id;
    }

    tires[0].part.transform.position = vec2(-25, -25);
    tires[1].part.transform.position = vec2(25, -25);
    tires[2].part.transform.position = vec2(-25, 25);
    tires[3].part.transform.position = vec2(25, 25);

    PartId fl = vehicle.add_tire(tires[0]);
    PartId fr = vehicle.add_tire(tires[1]);
    PartId bl = vehicle.add_tire(tires[2]);
    PartId br = vehicle.add_tire(tires[3]);

    Chasis* ch = vehicle.get_chasis(chasis_id);
    ch->basic.frontLeft.attach(fl);
    ch->basic.frontRight.attach(fr);
    ch->basic.backLeft.attach(bl);
    ch->basic.backRight.attach(br);

    Controller con = {};
    con.kind = ControllerBasic;
    con.part.transform.scale = 1;
    con.basic.codeSizeLimit = 128;
    con.basic.script = {};
    vehicle.add_controller(con);

    vehicle.add_root(chasis_id);

    return vehicle;
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
