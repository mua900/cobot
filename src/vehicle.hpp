#ifndef _VEHICLE_H
#define _VEHICLE_H

#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"

enum PartKind : u16 {
    PART_TIRE,
    PART_CHASIS,
    PART_CONTROLLER,
    PART_KIND_COUNT,
    PART_KIND_SENTINEL,
};

struct PartId {
    u16 kind = 0;
    u16 index = 0;

    PartId() {}
    PartId(PartKind kind, u16 index) : kind(kind), index(index) {}
};

struct AttachmentPoint {
    u32 kindMask = 0;  // what kind of parts can be attached (it is ignored if it's 0)
    PartId part = {};  // the part that is attached

    AttachmentPoint() {}
    AttachmentPoint(u32 kind_mask) : kindMask(kind_mask) {}

    bool attach(PartId part_id) {
        if (kindMask != 0 && !(kindMask & part.kind)) {
            return false;
        }

        part = part_id;
    }
};

struct VPartData {
    vec2 position = {};  // relative to parent
    float scale = 0;

    VPartData() {}
    VPartData(vec2 position) : position(position), scale(1.0) {}
    VPartData(vec2 position, float scale) : position(position), scale(scale) {}
};

VPartData chain_part_data(VPartData p0, VPartData p1);

enum TireKind {
    TireBasic,
    TireKindCount,
};

struct BasicTire {
    float size = 0;
};

struct Tire {
    TireKind kind;
    VPartData part = {};
    union {
        BasicTire basic;
    };

    Tire() {}
    // implicit
    Tire(BasicTire t) : kind(TireBasic), basic(t) {}
    Tire(VPartData part, BasicTire t) : kind(TireBasic), part(part), basic(t) {}
};

enum ChasisKind {
    ChasisBasic,
    ChasisKindCount,
};

struct BasicChasis {
    AttachmentPoint frontLeft = {};
    AttachmentPoint frontRight = {};
    AttachmentPoint backLeft = {};
    AttachmentPoint backRight = {};
};

struct Chasis {
    ChasisKind kind;
    VPartData part = {};
    union {
        BasicChasis basic;
    };

    Chasis() {}
    // implicit
    Chasis(BasicChasis c) : kind(ChasisBasic), basic(c) {}
    Chasis(VPartData part, BasicChasis c) : kind(ChasisBasic), part(part), basic(c) {}
};

enum ControllerKind {
    ControllerBasic,
    ControllerKindCount,
};

struct BasicController {
    u32 codeSizeLimit = 0;
    int script = 0;
};

struct Controller {
    ControllerKind kind;
    VPartData part = {};
    union {
        BasicController basic;
    };

    Controller() {}
    // implicit
    Controller(BasicController c) : kind(ControllerBasic), basic(c) {}
    Controller(VPartData part, BasicController c) : kind(ControllerBasic), part(part), basic(c) {}
};

struct VehiclePart {
    PartKind kind = PART_KIND_SENTINEL;
    union {
        Tire* tire;
        Chasis* chasis;
        Controller* controller;
    };

    VehiclePart() {}
    VehiclePart(Tire* t) : kind(PART_TIRE), tire(t) {}
    VehiclePart(Chasis* c) : kind(PART_CHASIS), chasis(c) {}
    VehiclePart(Controller* c) : kind(PART_CONTROLLER), controller(c) {}
};

struct Vehicle {
    vec2 worldPosition = {};

    DArray<PartId> rootParts = {};
    
    DArray<Tire> tire = {};
    DArray<Chasis> chasis = {};
    DArray<Controller> controller = {};

    VPartData getPartData(PartId id) const;
};

// the lower 16 bits are the subkind and the higher 16 bits are the kind
using PartKindId = u32;
PartKindId getPartKindId(PartKind kind, u16 subkind);

constexpr static int MaxPartCount = cobot::max(cobot::max(ChasisKindCount, TireKindCount), ControllerKindCount);

const char* get_chasis_name(ChasisKind kind);
const char* get_tire_name(TireKind kind);
const char* get_controller_name(ControllerKind kind);

vec2 get_chasis_scale(ChasisKind kind);
vec2 get_tire_scale(TireKind kind);
vec2 get_controller_scale(ControllerKind kind);

bool load_tire_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_chasis_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_controller_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);

Vehicle get_default_vehicle();

#endif // _VEHICLE_H
