#include "vehicle.hpp"
#include "log.hpp"

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
        case PartStructure: {
            StructurePart& sp = structurePart.get(part.index);
            global = chain_part_transform(parent, sp.part.transform);
            scale = get_structure_part_scale(sp.kind);
            volume = cobot::Rectangle(global.position, global.scale * scale);
            switch (sp.kind)
            {
                case StructurePartChassis:
                {
                    if (sp.chassis.frontLeft.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, sp.chassis.frontLeft.part));
                    if (sp.chassis.frontRight.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, sp.chassis.frontRight.part));
                    if (sp.chassis.backLeft.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, sp.chassis.backLeft.part));
                    if (sp.chassis.backRight.part.is_valid()) volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, sp.chassis.backRight.part));
                    break;
                }
            }
            break;
        }
        case PartComputer: {
            global = chain_part_transform(parent, computerPart.get(part.index).part.transform);
            scale = get_computer_part_scale(computerPart.get(part.index).kind);
            volume = cobot::Rectangle(global.position, global.scale * scale);
            break;
        }
        case PartGround: {
            global = chain_part_transform(parent, groundPart.get(part.index).part.transform);
            scale = get_ground_part_scale(groundPart.get(part.index).kind);
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
        case PartStructure: return structurePart.get(part.index).kind;
        case PartGround:    return groundPart.get(part.index).kind;
        case PartComputer:  return computerPart.get(part.index).kind;
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
        case PartStructure: {
            return structurePart[id.index].part;
        }
        case PartGround: {
            return groundPart[id.index].part;
        }
        case PartComputer: {
            return computerPart[id.index].part;
        }
        default: panic("Invalid part type");
    }
}

PartId Vehicle::add_ground_part(GroundPart& g) {
    int index = groundPart.add(g);

    PartId thisPart = PartId(PartGround, index);

    VPartTransform transform = getWorldTransform(PartId(PartGround, index));
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_ground_part_scale(g.kind)));

    return thisPart;
}

PartId Vehicle::add_structure_part(StructurePart& sp) {
    
    int index = structurePart.add(sp);

    PartId thisPart = PartId(PartStructure, index);
    switch (sp.kind) {
        case StructurePartChassis:
        {
            if (sp.chassis.frontLeft.part.is_valid())  getParentRef(sp.chassis.frontLeft.part) = thisPart;
            if (sp.chassis.frontRight.part.is_valid()) getParentRef(sp.chassis.frontRight.part) = thisPart;
            if (sp.chassis.backLeft.part.is_valid())   getParentRef(sp.chassis.backLeft.part) = thisPart;
            if (sp.chassis.backRight.part.is_valid())  getParentRef(sp.chassis.backRight.part) = thisPart;
            break;
        }
        default: panic("Invalid chassis kind");
    }

    VPartTransform transform = getWorldTransform(thisPart);
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_structure_part_scale(sp.kind)));

    return thisPart;
}

PartId Vehicle::add_computer_part(Computer& c) {
    int index = computerPart.add(c);

    PartId thisPart = PartId(PartComputer, index);

    VPartTransform transform = getWorldTransform(PartId(PartComputer, index));
    volume = merge_volumes(volume, cobot::Rectangle(transform.position, transform.scale * get_computer_part_scale(c.kind)));

    return thisPart;
}

GroundPart* Vehicle::get_ground_part(PartId t) {
    if (t.kind != PartGround) {
        return nullptr;
    }
    return groundPart.get_ptr(t.index);
}

StructurePart* Vehicle::get_structure_part(PartId c) {
    if (c.kind != PartStructure) {
        return nullptr;
    }
    return structurePart.get_ptr(c.index);
}

Computer* Vehicle::get_computer_part(PartId c) {
    if (c.kind != PartComputer) {
        return nullptr;
    }
    return computerPart.get_ptr(c.index);
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
    AttachmentDistance distance = { nullptr, NullPartId, radius + 1 };
    for (int i = 0; i < rootParts.size(); i++)
    {
        auto point = get_attachment_point_near(rootParts.get(i), get_vehicle_transform(), position, radius);
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
        case PartStructure:
        {
            StructurePart& c = structurePart[part.index];
            VPartTransform t = chain_part_transform(parent, c.part.transform);

            switch (c.kind)
            {
                case StructurePartChassis:
                {
                    AttachmentDistance d = { nullptr, NullPartId, radius + 1 };
                    AttachmentPoint* points[4] = {
                        &c.chassis.frontLeft,
                        &c.chassis.frontRight,
                        &c.chassis.backLeft,
                        &c.chassis.backRight,
                    };

					const char* names [4] = {
						"FrontLeft",
						"FrontRight",
						"BackLeft",
						"BackRight",
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
                            dist = { points[i], part, distance2(position, chain_part_transform(t, VPartTransform(points[i]->position, 1)).position), names[i] };
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
        case PartComputer: break;
        case PartGround: break;
        default: break;
    }

    return {};
}

PartId Vehicle::get_part_on_location(PartId part, cobot::vec2 location, VPartTransform parent) const
{
    switch (part.kind)
    {
        case PartStructure: {
            StructurePart& sp = structurePart[part.index];
            VPartTransform t = chain_part_transform(parent, sp.part.transform);
            cobot::vec2 scale = get_structure_part_scale(sp.kind) * t.scale;

            switch (sp.kind) {
                case StructurePartChassis: {
                    PartId part;
                    part = get_part_on_location(sp.chassis.frontLeft.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(sp.chassis.frontRight.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(sp.chassis.backLeft.part, location, t);
                    if (part.is_valid()) return part;
                    part = get_part_on_location(sp.chassis.backRight.part, location, t);
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
        case PartGround: {
            GroundPart& ground = groundPart[part.index];
            VPartTransform t = chain_part_transform(parent, ground.part.transform);
            cobot::vec2 scale = get_ground_part_scale(ground.kind);
            if (cobot::Rectangle(t.position, scale * t.scale).contains_centered(location)) {
                return part;
            }
            else {
                return NullPartId;
            }
        }
        case PartComputer: {
            Computer& com = computerPart[part.index];
            VPartTransform t = chain_part_transform(parent, com.part.transform);
            cobot::vec2 scale = get_computer_part_scale(computerPart[part.index].kind);
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

    StructurePart chassis = {};
    chassis.kind = StructurePartChassis;
    chassis.chassis = getChassis();
    chassis.part.transform.scale = 1.0;

    PartId chassis_id = vehicle.add_structure_part(chassis);

    GroundPart wheels[4] = {};
    for (auto& t : wheels) {
        t.kind = GroundPartWheel;
        t.part.transform.scale = 1.0;
        t.wheel.size = 5;
        t.part.parent = chassis_id;
    }

    PartId fl = vehicle.add_ground_part(wheels[0]);
    PartId fr = vehicle.add_ground_part(wheels[1]);
    PartId bl = vehicle.add_ground_part(wheels[2]);
    PartId br = vehicle.add_ground_part(wheels[3]);

    StructurePart* ch = vehicle.get_structure_part(chassis_id);
    ch->chassis.frontLeft.attach(fl);
    ch->chassis.frontRight.attach(fr);
    ch->chassis.backLeft.attach(bl);
    ch->chassis.backRight.attach(br);

    Computer con = {};
    con.kind = ComputerBasic;
    con.script = {};
    con.part.transform.scale = 1.0;
    con.basic.codeSizeLimit = 128;
    vehicle.add_computer_part(con);

    vehicle.add_root(chassis_id);

    return vehicle;
}

void draw_attachment_point(AttachmentPoint point, VPartTransform parent, const RenderContext& context, float radius)
{
    VPartTransform t = chain_part_transform(parent, VPartTransform(point.position, 1));

    draw_circle_segment(context, t.position, radius, t.rotation, CONSTANT_HALF_PI, cobot::ColorF(0.1, 0.7, 0.1));
    draw_circle_segment(context, t.position, radius, t.rotation + CONSTANT_HALF_PI, CONSTANT_HALF_PI, cobot::ColorF(0.1, 0.1, 0.1));
    draw_circle_segment(context, t.position, radius, t.rotation + CONSTANT_PI, CONSTANT_HALF_PI, cobot::ColorF(0.1, 0.7, 0.1));
    draw_circle_segment(context, t.position, radius, t.rotation + CONSTANT_ONE_AND_HALF_PI, CONSTANT_HALF_PI, cobot::ColorF(0.1, 0.1, 0.1));
}

void draw_structure_part(const StructurePart& structure, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    AssetId imageId = parameters.partImages->partImages[PartStructure][structure.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, structure.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_structure_part_scale(structure.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);

    switch (structure.kind) {
        case StructurePartChassis: {
            const AttachmentPoint points[4] = {
                structure.chassis.frontLeft,
                structure.chassis.frontRight,
                structure.chassis.backLeft,
                structure.chassis.backRight,
            };

            for (int i = 0; i < 4; i++)
            {
                if (points[i].part.is_valid())
                {
                    draw_vehicle_part(points[i].part, chain_part_transform(transform, VPartTransform(points[i].position, 1)), context, catalog, vehicle, parameters);
                }
                else if (parameters.in_editor)
                {
                    draw_attachment_point(points[i], transform, context, 10);
                }
            }
            break;
        }
    }
}

void draw_ground_part(const GroundPart& ground, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    AssetId imageId = parameters.partImages->partImages[PartGround][ground.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, ground.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_ground_part_scale(ground.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);
}

void draw_computer_part(const Computer& computer, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    AssetId imageId = parameters.partImages->partImages[PartComputer][computer.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    VPartTransform transform = chain_part_transform(parent, computer.part.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_computer_part_scale(computer.kind) * transform.scale);
    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);
}

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    switch (part.kind)
    {
        case PartStructure: {
            const StructurePart& structure = vehicle.structurePart[part.index];
            draw_structure_part(structure, parent, context, catalog, vehicle, parameters);
            break;
        }
        case PartGround: {
            const GroundPart& ground = vehicle.groundPart[part.index];
            draw_ground_part(ground, parent, context, catalog, vehicle, parameters);
            break;
        }
        case PartComputer: {
            const Computer& computer = vehicle.computerPart[part.index];
            draw_computer_part(computer, parent, context, catalog, vehicle, parameters);
            break;
        }
        default: panic("Invalid part type");
    }
}

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    for (int i = 0; i < vehicle.rootParts.size(); i++)
    {
        VPartData part_data = vehicle.getPartData(vehicle.rootParts[i]);
        VPartTransform vtransform = vehicle.get_vehicle_transform();
        draw_vehicle_part(vehicle.rootParts[i], vtransform, context, catalog, vehicle, parameters);
    }
}

SDL_Texture* get_part_texture(PartKindId partKind, AssetCatalog& catalog)
{
    const char* name = nullptr;
    PartKind kind = get_part_kind(partKind);
    u16 subKind = get_subkind(partKind);

    switch (kind)
    {
        case PartStructure:      name = get_structure_part_name(StructurePartKind(subKind)); break;
        case PartGround:         name = get_ground_part_name(GroundPartKind(subKind)); break;
        case PartComputer:       name = get_computer_part_name(ComputerKind(subKind)); break;
        default: panic("Invalid part kind");
    }

    AssetId id = get_asset(String(name), catalog);
    return catalog.get_image(id);
}
