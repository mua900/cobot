#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "app/asset.hpp"
#include "app/text.hpp"

#include "script.hpp"
#include "parts.hpp"

#define VEHICLE_DEBUG 0

typedef u32 VehicleId;
constexpr VehicleId NullVehicleId = -1;

// maximum depth for part hierarchy
constexpr int VehicleMaxDepth = 1024;

struct Vehicle {
    MutableString name = {};
    cobot::vec2 worldPosition = {};
    cobot::vec2 velocity = {};
    float angularVelocity = {};
    float speed = 0;
    float orientation = 0;  // radians, 0 looking right
    cobot::Rectangle volume = {};
    float electricCharge;

    BucketList<PartId> rootParts = {};

    BucketList<VehiclePart> parts = {};
    BucketList<Script> scripts = {};

    PartId add_part(VehiclePart& t);
    VehiclePart& get_part(PartId part) const;

    int add_root(PartId part);

    VPartTransform getWorldTransform(PartId part) const;
    VPartData& getPartData(PartId id) const;
    PartId& getParentRef(PartId part);

    cobot::vec2 forward() const;
    VPartTransform get_vehicle_transform() const;

    cobot::Rectangle calculate_volume();
    cobot::Rectangle calculate_part_volume(PartId part);

    bool execute_command(VehicleCommand& command);

    PartId getPartAt(cobot::vec2 position) const;
    AttachmentDistance getAttachmentPointClosest(cobot::vec2 position, float radius);

    float calculatePowerConsumption() const;
    float calculatePowerGeneration() const;
private:
    cobot::Rectangle calculate_part_volume_with_parent(VPartTransform parent, PartId part);
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

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

#endif // VEHICLE_HPP
