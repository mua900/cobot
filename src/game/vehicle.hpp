#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include "app/asset.hpp"
#include "app/text.hpp"

#include "script.hpp"
#include "parts.hpp"

#define VEHICLE_DEBUG 0

struct VehiclePart {
    PartKind kind = PartKindSentinel;
    union {
        StructurePart structure;
        GroundPart ground;
        Computer computer;
    } data = {};

    VehiclePart() : data{} {}
    VehiclePart(GroundPart g) : kind(PartGround) {
        data.ground = g;
    }
    VehiclePart(StructurePart s) : kind(PartStructure) {
        data.structure = s;        
    }
    VehiclePart(Computer c) : kind(PartComputer) {
        data.computer = c;
    }
};

typedef u32 VehicleId;
constexpr VehicleId NullVehicleId = -1;

struct Vehicle {
    MutableString name = {};
    cobot::vec2 worldPosition = {};
    cobot::vec2 velocity = {};
    float angularVelocity = {};
    float speed = 0;
    float orientation = 0;  // radians, 0 looking right
    cobot::Rectangle volume = {};

    DArray<PartId> rootParts = {};
    
    BucketList<GroundPart> groundPart = {};
    BucketList<StructurePart> structurePart = {};
    BucketList<Computer> computerPart = {};
	BucketList<PowerPart> powerPart = {};

    BucketList<Script> scripts = {};

    PartId add_ground_part(GroundPart& t);
    PartId add_structure_part(StructurePart& c);
    PartId add_computer_part(Computer& c);
    PartId add_power_part(PowerPart& p);

    GroundPart* get_ground_part(PartId t);
    StructurePart* get_structure_part(PartId c);
    Computer* get_computer_part(PartId c);
    PowerPart* get_power_part(PartId p);

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

void draw_structure_part(const StructurePart& sp, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);
void draw_ground_part(const GroundPart& ground, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);
void draw_computer_part(const Computer& computer, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);
void draw_power_part(const PowerPart& power, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VehicleDrawParameters& parameters);

#endif // VEHICLE_HPP
