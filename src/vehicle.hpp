#ifndef _VEHICLE_H
#define _VEHICLE_H

#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"
#include "script.hpp"

enum PartKind : u16 {
    PART_TIRE = 0,
    PART_CHASIS = 1,
    PART_CONTROLLER = 2,
    PART_KIND_COUNT = 3,
    PART_KIND_SENTINEL = 4,
};

struct PartId {
    u16 kind = PART_KIND_SENTINEL;
    u16 index = 0;

    PartId() {}
    PartId(PartKind kind, u16 index) : kind(kind), index(index) {}

    bool is_valid() const { return kind != PART_KIND_SENTINEL; }
    bool is_null() const { return kind == PART_KIND_SENTINEL; }
};

static const PartId NullPartId = PartId();

struct VPartTransform {
    vec2 position = {};
    float scale = 0;

    VPartTransform() {}
    VPartTransform(vec2 pos, float sca) : position(pos), scale(sca) {}
};

VPartTransform chain_part_transform(VPartTransform p0, VPartTransform p1);

struct VPartData {
    PartId parent = {};
    VPartTransform transform = {};
    // float weight = 0;

    VPartData() {}
    VPartData(vec2 position) : transform(position, 1.0f) {}
    VPartData(vec2 position, float scale) : transform(position, scale) {}
    VPartData(PartId parent, vec2 position, float scale) : parent(parent), transform(position, scale) {}
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

enum TireKind {
    TireBasic,
    TireKindCount,
    TireSentinel,
};

struct BasicTire {
    float size = 0;
};

struct Tire {
    TireKind kind = TireSentinel;
    VPartData part = {};
    union {
        BasicTire basic;
    };

    Tire() {
        basic = {};
    }
    // implicit
    Tire(BasicTire t) : kind(TireBasic), basic(t) {}
    Tire(VPartData part, BasicTire t) : kind(TireBasic), part(part), basic(t) {}
};

enum ChasisKind {
    ChasisBasic,
    ChasisKindCount,
    ChasisSentinel,
};

struct BasicChasis {
    AttachmentPoint frontLeft = {};
    AttachmentPoint frontRight = {};
    AttachmentPoint backLeft = {};
    AttachmentPoint backRight = {};
};

struct Chasis {
    ChasisKind kind = ChasisSentinel;
    VPartData part = {};
    union {
        BasicChasis basic;
    };

    Chasis() {
        basic = {};
    }
    // implicit
    Chasis(BasicChasis c) : kind(ChasisBasic), basic(c) {}
    Chasis(VPartData part, BasicChasis c) : kind(ChasisBasic), part(part), basic(c) {}
};

enum ControllerKind {
    ControllerBasic,
    ControllerKindCount,
    ControllerSentinel,
};

struct BasicController {
    u32 codeSizeLimit = 0;
};

struct Controller {
    ControllerKind kind = ControllerSentinel;
    int script = 0;
    VPartData part = {};
    union {
        BasicController basic;
    };

    Controller() {
        basic = {};
    }
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

    VehiclePart() {
        tire = nullptr;
    }
    VehiclePart(Tire* t) : kind(PART_TIRE), tire(t) {}
    VehiclePart(Chasis* c) : kind(PART_CHASIS), chasis(c) {}
    VehiclePart(Controller* c) : kind(PART_CONTROLLER), controller(c) {}
};

struct Vehicle {
    vec2 worldPosition = {};
    vec2 velocity = {};
    Rectangle volume = {};

    DArray<PartId> rootParts = {};
    
    DArray<Tire> tire = {};
    DArray<Chasis> chasis = {};
    DArray<Controller> controller = {};

    PartId add_tire(Tire& t);
    PartId add_chasis(Chasis& c);
    PartId add_controller(Controller& c);

    Tire* get_tire(PartId t);
    Chasis* get_chasis(PartId c);
    Controller* get_controller(PartId c);

    int add_root(PartId part);

    VPartTransform getWorldTransform(PartId part) const;
    VPartData& getPartData(PartId id) const;
    PartId& getParentRef(PartId part);

    PartId getPartAt(vec2 position) const;
private:
    PartId get_part_on_location(PartId part, vec2 location, VPartTransform parent) const;
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
