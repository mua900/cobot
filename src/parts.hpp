#ifndef PARTS_HPP
#define PARTS_HPP

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "asset.hpp"
#include "text.hpp"
#include "script.hpp"

enum PartKind : u16 {
    PART_TIRE = 0,
    PART_CHASSIS = 1,
    PART_CONTROLLER = 2,
    PART_KIND_COUNT = 3,
    PART_KIND_SENTINEL = 4,
};

#define PART_KIND_MASK(partKind) BIT(partKind)

struct PartId {
    PartKind kind = PART_KIND_SENTINEL;
    u16 index = 0;

    PartId() {}
    PartId(PartKind kind, u16 index) : kind(kind), index(index) {}

    bool is_valid() const { return kind != PART_KIND_SENTINEL; }
    bool is_null() const { return kind == PART_KIND_SENTINEL; }
};

static const PartId NullPartId = PartId();

// the lower 16 bits are the subkind and the higher 16 bits are the kind
using PartKindId = u32;
PartKindId get_part_kind_id(PartKind kind, u16 subkind);
PartKind get_part_kind(PartKindId kindId);
u16 get_subkind(PartKindId kindId);

struct VPartTransform {
    cobot::vec2 position = {};
    float rotation = 0;  // radians
    float scale = 1;

    VPartTransform() {}
    VPartTransform(cobot::vec2 pos, float sca) : position(pos), scale(sca) {}
    VPartTransform(cobot::vec2 pos, float rot, float sca) : position(pos), rotation(rot), scale(sca) {}

    VPartTransform inverse() const;
};

VPartTransform chain_part_transform(VPartTransform parent, VPartTransform child);

struct VPartData {
    PartId parent = {};
    VPartTransform transform = {};
    float weight = 0;

    VPartData() {}
    VPartData(cobot::vec2 position) : transform(position, 1.0) {}
    VPartData(cobot::vec2 position, float scale) : transform(position, scale) {}
    VPartData(PartId parent, cobot::vec2 position, float scale) : parent(parent), transform(position, scale) {}
};

struct AttachmentPoint {
    cobot::vec2 position = {};
    PartId part = {};  // the part that is attached
    u32 kindMask = 0;  // what kind of parts can be attached (it is ignored if it's 0)

    AttachmentPoint() {}
    AttachmentPoint(u32 kind_mask) : kindMask(kind_mask) {}

    bool attach(PartId part_id) {
        if (kindMask != 0 && !(kindMask & BIT(part_id.kind))) {
            return false;
        }

        part = part_id;
        return true;
    }
};

struct AttachmentDistance {
    AttachmentPoint* point;
    PartId parent;
    float distance;
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

enum ChassisKind {
    ChassisBasic,
    ChassisKindCount,
    ChassisSentinel,
};

struct BasicChassis {
    AttachmentPoint frontLeft = {};
    AttachmentPoint frontRight = {};
    AttachmentPoint backLeft = {};
    AttachmentPoint backRight = {};
};

struct Chassis {
    ChassisKind kind = ChassisSentinel;
    VPartData part = {};
    union {
        BasicChassis basic;
    };

    Chassis() {
        basic = {};
    }
    // implicit
    Chassis(BasicChassis c) : kind(ChassisBasic), basic(c) {}
    Chassis(VPartData part, BasicChassis c) : kind(ChassisBasic), part(part), basic(c) {}
};

BasicChassis getBasicChassis();

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

constexpr static int MaxPartCount = cobot::max(cobot::max(ChassisKindCount, TireKindCount), ControllerKindCount);

struct VPartImages {
    AssetId partImages [PART_KIND_COUNT][MaxPartCount] = {};
};

bool load_part_images(VPartImages& images, AssetCatalog& catalog);

const char* get_chassis_name(ChassisKind kind);
const char* get_tire_name(TireKind kind);
const char* get_controller_name(ControllerKind kind);

cobot::vec2 get_part_scale(PartKindId id);
cobot::vec2 get_chassis_scale(ChassisKind kind);
cobot::vec2 get_tire_scale(TireKind kind);
cobot::vec2 get_controller_scale(ControllerKind kind);

SDL_Texture* get_part_texture(PartKindId partKind, AssetCatalog& catalog);

bool load_tire_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);
bool load_chassis_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);
bool load_controller_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);

#endif // PARTS_HPP