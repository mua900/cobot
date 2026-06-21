#include "vehicle.hpp"
#include "log.hpp"

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

bool Vehicle::execute_command(VehicleCommand& command)
{
    if (command.type == CommandMove)
    {
        cobot::vec2 difference = command.program.target - worldPosition;
        float angle = atan2f(difference.y, difference.x);
        float distance = difference.magnitude();
        bool arrived = distance < 0.01;
        bool turned = fabsf(orientation - angle) < 0.05;
        if (!arrived)
        {
            cobot::vec2 direction = difference / distance;
            velocity = speed * direction;
        }
        if (!turned)
        {
            orientation += (angle - orientation) > 0 ? 0.1 : -0.1;
            orientation = cobot::normalize_angle_radians_f(orientation);
        }

        return turned && arrived;
    }

    return false;
}

VPartTransform Vehicle::get_vehicle_transform() const
{
    return VPartTransform(worldPosition, orientation, 1.0);
}

cobot::Rectangle Vehicle::calculate_volume() const
{
    cobot::Rectangle volume = {};
    VPartTransform t = get_vehicle_transform();
    for (auto& root : rootParts)
    {
        cobot::Rectangle partVolume = calculate_part_volume_with_parent(t, root);
        cobot::merge_volumes(volume, partVolume);
    }

    return volume;
}

cobot::Rectangle Vehicle::calculate_part_volume(PartId part) const
{
    return calculate_part_volume_with_parent(get_vehicle_transform(), part);
}

cobot::Rectangle Vehicle::calculate_part_volume_with_parent(VPartTransform parent, PartId part) const
{
    cobot::Rectangle volume {};
    VPartTransform global = {};
    cobot::vec2 scale = {};
    switch (part.kind)
    {
        case PART_CHASSIS: {
            Chassis& c = chassis.get_ref(part.index);
            global = chain_part_transform(parent, c.part.transform);
            scale = get_chassis_scale(c.kind);
            volume = cobot::Rectangle(global.position, global.scale * scale);
            switch (c.kind)
            {
                case ChassisBasic:
                {
                    if (c.basic.frontLeft.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, c.basic.frontLeft.part));
                    if (c.basic.frontRight.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, c.basic.frontRight.part));
                    if (c.basic.backLeft.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, c.basic.backLeft.part));
                    if (c.basic.backRight.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, c.basic.backRight.part));
                    break;
                }
            }
            break;
        }
        case PART_CONTROLLER: {
            global = chain_part_transform(parent, controller.get_ref(part.index).part.transform);
            scale = get_controller_scale(controller.get_ref(part.index).kind);
            volume = cobot::Rectangle(global.position, global.scale * scale);
            break;
        }
        case PART_TIRE: {
            global = chain_part_transform(parent, tire.get_ref(part.index).part.transform);
            scale = get_tire_scale(tire.get_ref(part.index).kind);
            volume = cobot::Rectangle(global.position, global.scale * scale);
            break;
        }
        default:
            panic("Invalid part kind");
    }

    return volume;
}

cobot::vec2 Vehicle::forward() const
{
    return cobot::vec2(std::cosf(orientation), std::sinf(orientation));
}

VPartTransform Vehicle::getWorldTransform(PartId part) const
{
    VPartData data = getPartData(part);
    VPartTransform t = data.transform;
    while (data.parent.is_valid()) {
        data = getPartData(data.parent);
        t = chain_part_transform(data.transform, t);
    }

    t = chain_part_transform(VPartTransform(worldPosition, 1.0), t);

    return t;
}

u16 Vehicle::getSubKind(PartId part)
{
    switch (part.kind)
    {
        case PART_CHASSIS:       return chassis.get_ref(part.index).kind;
        case PART_TIRE:         return tire.get_ref(part.index).kind;
        case PART_CONTROLLER:   return controller.get_ref(part.index).kind;
        default: panic("Unknown part kind");
    }
}

PartId& Vehicle::getParentRef(PartId part)
{
    VPartData& data = getPartData(part);
    return data.parent;
}

VPartData& Vehicle::getPartData(PartId id) const
{
    switch (id.kind) {
        case PART_CHASSIS: {
            return chassis[id.index].part;
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

PartId Vehicle::add_tire(Tire& t) {
    int index = tire.add(t);

    PartId thisPart = PartId(PART_TIRE, index);

    VPartTransform transform = getWorldTransform(PartId(PART_TIRE, index));
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_tire_scale(t.kind)));

    return thisPart;
}

PartId Vehicle::add_chassis(Chassis& c) {
    
    int index = chassis.add(c);

    PartId thisPart = PartId(PART_CHASSIS, index);
    switch (c.kind) {
        case ChassisBasic:
        {
            if (c.basic.frontLeft.part.is_valid())  getParentRef(c.basic.frontLeft.part) = thisPart;
            if (c.basic.frontRight.part.is_valid()) getParentRef(c.basic.frontRight.part) = thisPart;
            if (c.basic.backLeft.part.is_valid())   getParentRef(c.basic.backLeft.part) = thisPart;
            if (c.basic.backRight.part.is_valid())  getParentRef(c.basic.backRight.part) = thisPart;
            break;
        }
        default: panic("Invalid chassis kind");
    }

    VPartTransform transform = getWorldTransform(thisPart);
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_chassis_scale(c.kind)));

    return thisPart;
}

PartId Vehicle::add_controller(Controller& c) {
    int index = controller.add(c);

    PartId thisPart = PartId(PART_CONTROLLER, index);

    VPartTransform transform = getWorldTransform(PartId(PART_CONTROLLER, index));
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_controller_scale(c.kind)));

    return thisPart;
}

Tire* Vehicle::get_tire(PartId t) {
    if (t.kind != PART_TIRE) {
        return nullptr;
    }
    return tire.get_ptr(t.index);
}

Chassis* Vehicle::get_chassis(PartId c) {
    if (c.kind != PART_CHASSIS) {
        return nullptr;
    }
    return chassis.get_ptr(c.index);
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

PartId Vehicle::getPartAt(cobot::vec2 position) const
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

AttachmentDistance Vehicle::getAttachmentPointClosest(cobot::vec2 position, float radius)
{
    position -= worldPosition;

    AttachmentDistance distance = { nullptr, NullPartId, radius + 1 };
    for (int i = 0; i < rootParts.size(); i++)
    {
        auto point = get_attachment_point_near(rootParts.get_ref(i), get_vehicle_transform(), position, radius);
        if (point.point)
        {
            if (point.distance < distance.distance)
            {
                distance = point;
            }
        }
    }

    return distance;
}

AttachmentDistance Vehicle::get_attachment_point_near(PartId part, VPartTransform parent, cobot::vec2 position, float radius)
{
    if (part.is_null())
    {
        return {};
    }

    switch (part.kind)
    {
        case PART_CHASSIS:
        {
            Chassis& c = chassis[part.index];
            VPartTransform t = chain_part_transform(parent, c.part.transform);

            switch (c.kind)
            {
                case ChassisBasic:
                {
                    AttachmentDistance d = { nullptr, NullPartId, radius + 1 };
                    AttachmentPoint* points[4] = {
                        &c.basic.frontLeft,
                        &c.basic.frontRight,
                        &c.basic.backLeft,
                        &c.basic.backRight,
                    };

                    for (int i = 0; i < 4; i++)
                    {
                        AttachmentDistance dist = {};
                        if (points[i]->part.is_valid())
                        {
                            dist = get_attachment_point_near(points[i]->part, t, position, radius);
                        }
                        else
                        {
                            dist = { points[i], part, distance2(position, chain_part_transform(t, VPartTransform(points[i]->position, 1)).position) };
                        }

                        if (dist.distance < radius)
                        {
                            if (!d.point || dist.distance < d.distance)
                            {
                                d = dist;
                            }
                        }
                    }

                    return d;
                }
            }

            break;
        }
        case PART_CONTROLLER: break;
        case PART_TIRE: break;
        default: break;
    }

    return {};
}

PartId Vehicle::get_part_on_location(PartId part, cobot::vec2 location, VPartTransform parent) const
{
    switch (part.kind)
    {
        case PART_CHASSIS: {
            Chassis& cha = chassis[part.index];
            VPartTransform t = chain_part_transform(parent, cha.part.transform);
            cobot::vec2 scale = get_chassis_scale(cha.kind) * t.scale;

            switch (cha.kind) {
                case ChassisBasic: {
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
                default: panic("Invalid chassis kind");
            }

            if (cobot::Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        case PART_TIRE: {
            Tire& tr = tire[part.index];
            VPartTransform t = chain_part_transform(parent, tr.part.transform);
            cobot::vec2 scale = get_tire_scale(tr.kind);
            if (cobot::Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        case PART_CONTROLLER: {
            Controller& con = controller[part.index];
            VPartTransform t = chain_part_transform(parent, con.part.transform);
            cobot::vec2 scale = get_controller_scale(controller[part.index].kind);
            if (cobot::Rectangle(t.position, scale * t.scale).contains_centered(location)) {
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

    vehicle.name = String("Default");
    vehicle.speed = 10;

    vehicle.worldPosition = cobot::vec2(600, 300);
    vehicle.volume = cobot::Rectangle(vehicle.worldPosition, cobot::vec2());

    Chassis chassis = {};
    chassis.kind = ChassisBasic;
    chassis.part.transform.scale = 1.0;

    int hDist = 25;
    int vDist = 36;
    chassis.basic.frontLeft.position = cobot::vec2( hDist, -vDist);
    chassis.basic.frontRight.position = cobot::vec2( hDist,  vDist);
    chassis.basic.backLeft.position = cobot::vec2(-hDist, -vDist);
    chassis.basic.backRight.position = cobot::vec2(-hDist,  vDist);

    PartId chassis_id = vehicle.add_chassis(chassis);

    Tire tires[4] = {};
    for (auto& t : tires) {
        t.kind = TireBasic;
        t.part.transform.scale = 1.0;
        t.basic.size = 5;
        t.part.parent = chassis_id;
    }

    PartId fl = vehicle.add_tire(tires[0]);
    PartId fr = vehicle.add_tire(tires[1]);
    PartId bl = vehicle.add_tire(tires[2]);
    PartId br = vehicle.add_tire(tires[3]);

    Chassis* ch = vehicle.get_chassis(chassis_id);
    ch->basic.frontLeft.attach(fl);
    ch->basic.frontRight.attach(fr);
    ch->basic.backLeft.attach(bl);
    ch->basic.backRight.attach(br);

    Controller con = {};
    con.kind = ControllerBasic;
    con.script = {};
    con.part.transform.scale = 1.0;
    con.basic.codeSizeLimit = 128;
    vehicle.add_controller(con);

    vehicle.add_root(chassis_id);

    return vehicle;
}

void draw_chassis(const Chassis& chassis, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages)
{
    AssetId imageId = partImages.partImages[PART_CHASSIS][chassis.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, chassis.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_chassis_scale(chassis.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);

    switch (chassis.kind) {
        case ChassisBasic: {
            if (chassis.basic.frontLeft.part.is_valid())
            {
                draw_vehicle_part(chassis.basic.frontLeft.part, chain_part_transform(transform, VPartTransform(chassis.basic.frontLeft.position, 1)), context, catalog, vehicle, partImages);
            }
            if (chassis.basic.frontRight.part.is_valid())
            {
                draw_vehicle_part(chassis.basic.frontRight.part, chain_part_transform(transform, VPartTransform(chassis.basic.frontRight.position, 1)), context, catalog, vehicle, partImages);
            }
            if (chassis.basic.backLeft.part.is_valid())
            {
                draw_vehicle_part(chassis.basic.backLeft.part, chain_part_transform(transform, VPartTransform(chassis.basic.backLeft.position, 1)), context, catalog, vehicle, partImages);
            }
            if (chassis.basic.backRight.part.is_valid())
            {
                draw_vehicle_part(chassis.basic.backRight.part, chain_part_transform(transform, VPartTransform(chassis.basic.backRight.position, 1)), context, catalog, vehicle, partImages);
            }
            break;
        }
    }
}

void draw_tire(const Tire& tire, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages)
{
    AssetId imageId = partImages.partImages[PART_TIRE][tire.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, tire.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_tire_scale(tire.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);
}

void draw_controller(const Controller& controller, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages)
{
    AssetId imageId = partImages.partImages[PART_CONTROLLER][controller.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, controller.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_controller_scale(controller.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);
}

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages)
{
    switch (part.kind)
    {
        case PART_CHASSIS: {
            const Chassis& chassis = vehicle.chassis[part.index];
            draw_chassis(chassis, parent, context, catalog, vehicle, partImages);
            break;
        }
        case PART_TIRE: {
            const Tire& tire = vehicle.tire[part.index];
            draw_tire(tire, parent, context, catalog, vehicle, partImages);
            break;
        }
        case PART_CONTROLLER: {
            const Controller& controller = vehicle.controller[part.index];
            draw_controller(controller, parent, context, catalog, vehicle, partImages);
            break;
        }
        default: panic("Invalid part type");
    }
}

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages)
{
    for (int i = 0; i < vehicle.rootParts.size(); i++)
    {
        VPartData part_data = vehicle.getPartData(vehicle.rootParts[i]);
        VPartTransform vtransform = vehicle.get_vehicle_transform();
        draw_vehicle_part(vehicle.rootParts[i], vtransform, context, catalog, vehicle, partImages);
    }
}

SDL_Texture* get_part_texture(PartKindId partKind, AssetCatalog& catalog)
{
    const char* name = nullptr;
    PartKind kind = get_part_kind(partKind);
    u16 subKind = get_subkind(partKind);

    switch (kind)
    {
        case PART_CHASSIS:      name = get_chassis_name(ChassisKind(subKind)); break;
        case PART_TIRE:         name = get_tire_name(TireKind(subKind)); break;
        case PART_CONTROLLER:   name = get_controller_name(ControllerKind(subKind)); break;
        default: panic("Invalid part kind");
    }

    AssetId id = get_asset(String(name), catalog);
    return catalog.get_image(id);
}

bool load_tire_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog)
{
    for (int i = 0; i < (int)TireKindCount; i++)
    {
        const char* name = get_tire_name(TireKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PART_TIRE, i)));
    }

    return true;
}

bool load_chassis_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog) {
    for (int i = 0; i < (int)ChassisKindCount; i++)
    {
        const char* name = get_chassis_name(ChassisKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PART_CHASSIS, i)));
    }

    return true;
}

bool load_controller_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog) {
    for (int i = 0; i < (int)ControllerKindCount; i++)
    {
        const char* name = get_controller_name(ControllerKind(i));
        AssetId id = get_asset(String(name), catalog);
        if (!id.is_valid()) return false;
        SDL_Texture* texture = catalog.get_image(id);

        icons.add(IconButton(texture, background, get_part_kind_id(PART_CONTROLLER, i)));
    }

    return true;
}
