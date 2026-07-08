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
	else if (command.type == CommandTurn)
	{
		cobot::vec2 difference = command.program.turnTarget - worldPosition;
		float angle = atan2f(difference.y, difference.x);
		bool turned = fabsf(orientation - angle) < 0.05;
		if (!turned)
		{
			orientation += (angle - orientation) > 0 ? 0.1 : -0.1;
			orientation = cobot::normalize_angle_radians_f(orientation);
		}

		return turned;
	}

    return false;
}

VPartTransform Vehicle::get_vehicle_transform() const
{
    return VPartTransform(worldPosition, orientation, 1.0);
}

float Vehicle::calculatePowerConsumption() const
{
    return 0;  // @todo
}

float Vehicle::calculatePowerGeneration() const
{
    return 0;  // @todo
}

cobot::Rectangle Vehicle::calculate_volume()
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

cobot::Rectangle Vehicle::calculate_part_volume(PartId part)
{
    return calculate_part_volume_with_parent(get_vehicle_transform(), part);
}

cobot::Rectangle Vehicle::calculate_part_volume_with_parent(VPartTransform parent, PartId partid)
{
    cobot::Rectangle volume {};

    VehiclePart& part = get_part(partid);
    Array<AttachmentPoint> points = part.getAttachments();
    cobot::vec2 scale = get_part_scale(part.kind);

    VPartTransform global = chain_part_transform(parent, part.partData.transform);
    volume = cobot::Rectangle(global.position, global.scale * scale);

    for (int i = 0; i < points.count; i++)
    {
        if (points[i].used)
        {
            volume = cobot::merge_volumes(volume, calculate_part_volume_with_parent(global, points[i].part));
        }
    }

    return volume;
}

bool Vehicle::unattach_from_parent(PartId id)
{
    VehiclePart& part = get_part(id);
    auto parentId = part.partData.parent;

    if (parentId != NullPartId)
    {
        VehiclePart& parent = get_part(parentId);
        Array<AttachmentPoint> points = parent.getAttachments();
        for (auto& p : points)
        {
            if (p.part == parentId)
            {
                if (!p.unattach())
                {
                    return false;
                }
            }
        }
    }

    return true;
}

cobot::vec2 Vehicle::forward() const
{
    return cobot::vec2(std::cosf(orientation), std::sinf(orientation));
}

VPartTransform Vehicle::getWorldTransform(PartId part) const
{
	VPartData data = getPartData(part);
    VPartTransform t = data.transform;
	int depth = 0;
    while (data.parent != NullPartId) {
        data = getPartData(data.parent);
        t = chain_part_transform(data.transform, t);

		depth += 1;

		if (depth >= VehicleMaxDepth)
		{
			log_error("----------");
			log_error("Vehicle hierarchy is deeper than maximum depth or too many iterations : %d in getWorldTransform for vehicle", VehicleMaxDepth);
			log_error("----------");
			return t;
		}
    }

    t = chain_part_transform(get_vehicle_transform(), t);

    return t;
}

PartId& Vehicle::getParentRef(PartId part)
{
    VPartData& data = getPartData(part);
    return data.parent;
}

VehiclePart& Vehicle::get_part(PartId id) const
{
    return parts.get(id);
}

VPartData& Vehicle::getPartData(PartId id) const
{
    return get_part(id).partData;
}

PartId Vehicle::add_part(VehiclePart& part)
{
    if (part.kind == PartKindComputer)
    {
        lua_State* L = luaL_newstate();
        if (!L) return NullPartId;

        int s = scripts.add(Script(L));
        init_script(scripts.get(s), this);
    }

    u32 id = parts.add(part);
    return id;
}

int Vehicle::add_root(PartId part)
{
    get_part(part).partData.parent = NullPartId;
    return rootParts.add(part);
}

PartId Vehicle::getPartAt(cobot::vec2 position) const
{
    position -= worldPosition;
    int count = rootParts.count();
    for (int i = 0; i < count; i++)
    {
        PartId part = get_part_on_location(rootParts[i], position, getPartData(rootParts[i]).transform);
        if (part != NullPartId)
        {
            return part;
        }
    }

    return NullPartId;
}

AttachmentDistance Vehicle::getAttachmentPointClosest(cobot::vec2 position, float radius)
{
    AttachmentDistance distance = { nullptr, NullPartId, radius + 1 };
    int count = rootParts.count();
    for (int i = 0; i < count; i++)
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
    if (part == NullPartId)
    {
        return {};
    }

    VehiclePart& p = get_part(part);
    Array<AttachmentPoint> points = p.getAttachments();

    VPartTransform t = chain_part_transform(parent, p.partData.transform);

    AttachmentDistance d = {};
    for (int i = 0; i < points.count; i++)
    {
        const char* names [5] = {
            "FrontLeft",
            "FrontRight",
            "BackLeft",
            "BackRight",
            "Top",
        };

        AttachmentDistance dist = {};
        if (points[i].used)
        {
            dist = get_attachment_point_near(points[i].part, t, position, radius);
        }
        else
        {
            dist = { &points[i], part, distance2(position, chain_part_transform(t, VPartTransform(points[i].position, 1)).position), names[i] };
        }

        if (dist.distance < radius)
        {
            if ((dist.point) && (!d.point || dist.distance < d.distance))
            {
                d = dist;
            }
        }
    }

    return d;
}

PartId Vehicle::get_part_on_location(PartId part, cobot::vec2 location, VPartTransform parent) const
{
    VehiclePart& p = get_part(part);
    VPartTransform t = chain_part_transform(parent, p.partData.transform);
    cobot::vec2 scale = get_part_scale(p.kind) * t.scale;

    Array<AttachmentPoint> points = p.getAttachments();
    for (int i = 0; i < points.count; i++)
    {
        if (points[i].used)
        {
            PartId child = get_part_on_location(points[i].part, location, t);
            if (child)
            {
                return child;
            }
        }
    }

    if (cobot::Rectangle(t.position, scale).contains_centered(location)) {
        return part;
    }
    else {
        return NullPartId;
    }
}

Vehicle get_default_vehicle()
{
    Vehicle vehicle = {};

    vehicle.name = String("Default");
    vehicle.speed = 10;

    vehicle.worldPosition = cobot::vec2(0,0);
    vehicle.volume = cobot::Rectangle(vehicle.worldPosition, cobot::vec2());

    VehiclePart chassis = VehiclePart(PartKindChassis);
	chassis.partData.parent = NullPartId;
    chassis.init();

    PartId chassis_id = vehicle.add_part(chassis);

    VehiclePart wheels[4] = {};
    for (auto& t : wheels) {
        t = VehiclePart(PartKindWheel);
        t.partData.parent = chassis_id;
    }

    PartId fl = vehicle.add_part(wheels[0]);
    PartId fr = vehicle.add_part(wheels[1]);
    PartId bl = vehicle.add_part(wheels[2]);
    PartId br = vehicle.add_part(wheels[3]);

	VehiclePart solarPanel = VehiclePart(PartKindSolarPanel);
	PartId solar = vehicle.add_part(solarPanel);
	
    VehiclePart& ch = vehicle.get_part(chassis_id);
    ch.data.chassis.points[ChassisFrontLeft].attach(fl);
    ch.data.chassis.points[ChassisFrontRight].attach(fr);
    ch.data.chassis.points[ChassisBackLeft].attach(bl);
    ch.data.chassis.points[ChassisBackRight].attach(br);
	ch.data.chassis.points[ChassisTop].attach(solar);

    VehiclePart com = VehiclePart(PartKindComputer);
    vehicle.add_part(com);

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

void draw_vehicle_part(PartId id, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    VehiclePart& part = vehicle.get_part(id);
    VPartTransform transform = chain_part_transform(parent, part.partData.transform);
    cobot::Rectangle area = cobot::Rectangle(transform.position, get_part_scale(part.kind) * transform.scale);

    AssetId imageId = parameters.partImages->partImages[part.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    render_texture_rotate(context, area, texture, transform.rotation, FlipNone, true);

    auto points = part.getAttachments();
    for (int i = 0; i < points.count; i++)
    {
        if (points[i].used)
        {
            draw_vehicle_part(points[i].part, chain_part_transform(transform, VPartTransform(points[i].position, 1)), context, catalog, vehicle, parameters);
        }
        else if (parameters.in_editor)
        {
            draw_attachment_point(points[i], transform, context, 10);
        }
    }
}

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters)
{
    VPartTransform vtransform = vehicle.get_vehicle_transform();
    for (auto& root : vehicle.rootParts)
    {
        draw_vehicle_part(root, vtransform, context, catalog, vehicle, parameters);
    }
}

SDL_Texture* get_part_texture(PartKind kind, AssetCatalog& catalog)
{
    const char* name = get_part_name(kind);

    AssetId id = get_asset(String(name), catalog);
    return catalog.get_image(id);
}
