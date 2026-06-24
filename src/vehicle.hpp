#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "asset.hpp"
#include "text.hpp"
#include "script.hpp"

#include "parts.hpp"

struct VehiclePart {
    PartKind kind = PART_KIND_SENTINEL;
    union {
        Tire* tire;
        Chassis* chassis;
        Controller* controller;
    };

    VehiclePart() {
        tire = nullptr;
    }
    VehiclePart(Tire* t) : kind(PART_TIRE), tire(t) {}
    VehiclePart(Chassis* c) : kind(PART_CHASSIS), chassis(c) {}
    VehiclePart(Controller* c) : kind(PART_CONTROLLER), controller(c) {}
};

typedef u32 VehicleId;
constexpr VehicleId NullVehicleId = -1;

struct Vehicle {
    String name = {};  // @todo this would need to change when we need to get names from user
    cobot::vec2 worldPosition = {};
    cobot::vec2 velocity = {};
    float speed = 0;
    float orientation = 0;  // radians, 0 looking right
    cobot::Rectangle volume = {};

    DArray<PartId> rootParts = {};
    
    BucketList<Tire> tire = {};
    BucketList<Chassis> chassis = {};
    BucketList<Controller> controller = {};

    PartId add_tire(Tire& t);
    PartId add_chassis(Chassis& c);
    PartId add_controller(Controller& c);

    Tire* get_tire(PartId t);
    Chassis* get_chassis(PartId c);
    Controller* get_controller(PartId c);

    int add_root(PartId part);

    u16 getSubKind(PartId part);
    VPartTransform getWorldTransform(PartId part) const;
    VPartData& getPartData(PartId id) const;
    PartId& getParentRef(PartId part);

    cobot::vec2 forward() const;
    VPartTransform get_vehicle_transform() const;

    cobot::Rectangle calculate_volume() const;
    cobot::Rectangle calculate_part_volume(PartId part) const;

    bool execute_command(VehicleCommand& command);

    PartId getPartAt(cobot::vec2 position) const;
    AttachmentDistance getAttachmentPointClosest(cobot::vec2 position, float radius);
private:
    cobot::Rectangle calculate_part_volume_with_parent(VPartTransform parent, PartId part) const;
    AttachmentDistance get_attachment_point_near(PartId part, VPartTransform parent, cobot::vec2 position, float radius);
    PartId get_part_on_location(PartId part, cobot::vec2 location, VPartTransform parent) const;
};

Vehicle get_default_vehicle();

void draw_attachment_point(AttachmentPoint point, VPartTransform parent, const RenderContext& context, float radius);

struct VehicleDrawParameters {
    const VPartImages* partImages;
    PartId selectedPart;
    bool in_editor;

    VehicleDrawParameters(const VPartImages* img, PartId selected, bool editor)
        :
        partImages(img),
        selectedPart(selected),
        in_editor(editor)
    {}
};

void draw_chassis(const Chassis& chasis, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);
void draw_tire(const Tire& tire, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);
void draw_controller(const Controller& controller, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

#endif // VEHICLE_HPP
