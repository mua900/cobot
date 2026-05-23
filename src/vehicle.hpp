#ifndef _VEHICLE_H
#define _VEHICLE_H

#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"

enum PartKind : u32 {
    PART_TIRE,
    PART_CHASIS,
    PART_CONTROLLER,
    PART_KIND_COUNT,
    PART_KIND_SENTINEL,
};

struct AttachmentPoint {
    u32 kindMask = 0;   // what kind of parts can be attached (it is ignored if it's 0)
    PartKind kind;      // the kind of the attached part
    int part = 0;       // the index of the part

    AttachmentPoint() {}
    AttachmentPoint(u32 kind_mask) : kindMask(kind_mask) {}

    bool attach(PartKind partKind, int partIndex) {
        if (kindMask != 0 && !(kindMask & partKind)) {
            return false;
        }

        kind = partKind;
        part = partIndex;
    }
};

enum TireKind {
    TireBasic,
    TireKindCount,
};

struct BasicTire {
    float size = 0;
};

struct Tire {
    TireKind kind;
    union {
        BasicTire basic;
    };

    Tire() {}
    // implicit
    Tire(BasicTire t) : kind(TireBasic), basic(t) {}
};

enum ChasisKind {
    ChasisBasic,
    ChasisKindCount,
};

struct BasicChasis {
    vec2 scale = {};
    AttachmentPoint frontLeft = {};
    AttachmentPoint frontRight = {};
    AttachmentPoint backLeft = {};
    AttachmentPoint backRight = {};
};

struct Chasis {
    ChasisKind kind;
    union {
        BasicChasis basic;
    };

    Chasis() {}
    // implicit
    Chasis(BasicChasis c) : kind(ChasisBasic), basic(c) {}
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
    union {
        BasicController basic;
    };

    Controller() {}
    // implicit
    Controller(BasicController c) : kind(ControllerBasic), basic(c) {}
};

struct Vehicle {
    vec2 worldPosition = {};

    DArray<Tire> tire;
    DArray<Chasis> chasis;
    DArray<Controller> controller;
};

struct VehiclePart {
    PartKind kind = PART_KIND_SENTINEL;
    union {
        Tire tire;
        Chasis chasis;
        Controller controller;
    };
};

constexpr static int MaxPartCount = cobot::max(cobot::max(ChasisKindCount, TireKindCount), ControllerKindCount);

// lower 16 bits are the subtype and the higher 16 bits are the type
using PartId = u32;
PartId getPartId(int partType, int subType);

const char* get_chasis_name(ChasisKind kind);
const char* get_tire_name(TireKind kind);
const char* get_controller_name(ControllerKind kind);

bool load_tire_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_chasis_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_controller_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);

Vehicle get_default_vehicle();

#endif // _VEHICLE_H
