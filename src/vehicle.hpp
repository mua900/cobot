#ifndef _VEHICLE_H
#define _VEHICLE_H

#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"

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
};

enum ChasisKind {
    ChasisBasic,
    ChasisKindCount,
};

struct BasicChasis {
    vec2 scale = {};
};

struct Chasis {
    ChasisKind kind;
    union {
        BasicChasis basic;
    };
};

enum ControllerKind {
    ControllerBasic,
    ControllerKindCount,
};

struct BasicController {
    u32 codeSize = 0;
};

struct Controller {
    ControllerKind kind;
    union {
        BasicController basic;
    };
};

struct Vehicle {
    DArray<Tire> tire;
    DArray<Chasis> chasis;
    DArray<Controller> controller;
};

enum PartKind {
    TIRE,
    CHASIS,
    CONTROLLER,
    PART_KIND_COUNT,
    PART_KIND_SENTINEL,
};

struct VehiclePart {
    PartKind kind = PART_KIND_SENTINEL;
    union {
        Tire tire;
        Chasis chasis;
        Controller controller;
    };
};

// lower 16 bits are the subtype and the higher 16 bits are the type
using PartId = u32;
PartId getPartId(int partType, int subType);

const char* get_chasis_name(ChasisKind kind);
const char* get_tire_name(TireKind kind);
const char* get_controller_name(ControllerKind kind);

bool load_tire_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_chasis_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);
bool load_controller_icons(DArray<IconButton>& icons, Color background, AssetCatalog& catalog);

#endif // _VEHICLE_H
