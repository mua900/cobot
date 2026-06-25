#ifndef PARTS_HPP
#define PARTS_HPP

#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "asset.hpp"
#include "text.hpp"
#include "script.hpp"

enum PartKind : u16 {
    PartGround = 0,
    PartStructure = 1,
    PartComputer = 2,
    PartKindCount = 3,
    PartKindSentinel = 4,
};

#define PART_KIND_MASK(partKind) BIT(partKind)

struct PartId {
    PartKind kind = PartKindSentinel;
    u16 index = 0;

    PartId() {}
    PartId(PartKind kind, u16 index) : kind(kind), index(index) {}

    bool is_valid() const { return kind != PartKindSentinel; }
    bool is_null() const { return kind == PartKindSentinel; }
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
	const char* name = nullptr;
};

enum GroundPartKind {
    GroundPartWheel = 0,
    GroundPartKindCount,
    GroundPartSentinel,
};

struct Wheel {
    float size = 0;
};

struct GroundPart {
    GroundPartKind kind = GroundPartSentinel;
    VPartData part = {};
    union {
        Wheel wheel;
    };

    GroundPart() {
        wheel = {};
    }
    // implicit
    GroundPart(Wheel t) : kind(GroundPartWheel), wheel(t) {}
    GroundPart(VPartData part, Wheel t) : kind(GroundPartWheel), part(part), wheel(wheel) {}
};

enum StructurePartKind {
    StructurePartChassis = 0,
    StructurePartKindCount,
    StructurePartSentinel,
};

struct Chassis {
    AttachmentPoint frontLeft = {};
    AttachmentPoint frontRight = {};
    AttachmentPoint backLeft = {};
    AttachmentPoint backRight = {};
};

struct StructurePart {
    StructurePartKind kind = StructurePartSentinel;
    VPartData part = {};
    union {
        Chassis chassis;
    };

    StructurePart() {
        chassis = {};
    }
    // implicit
    StructurePart(Chassis c) : kind(StructurePartChassis), chassis(c) {}
    StructurePart(VPartData part, Chassis c) : kind(StructurePartChassis), part(part), chassis(c) {}
};

Chassis getChassis();

enum ComputerKind {
    ComputerBasic = 0,
    ComputerKindCount,
    ComputerSentinel,
};

struct BasicComputer {
    u32 codeSizeLimit = 0;
};

struct Computer {
    ComputerKind kind = ComputerSentinel;
    int script = 0;
    VPartData part = {};
    union {
        BasicComputer basic;
    };

    Computer() {
        basic = {};
    }
    // implicit
    Computer(BasicComputer c) : kind(ComputerBasic), basic(c) {}
    Computer(VPartData part, BasicComputer c) : kind(ComputerBasic), part(part), basic(c) {}
};

constexpr static int MaxPartCount = cobot::max(cobot::max(StructurePartKindCount, GroundPartKindCount), ComputerKindCount);

struct VPartImages {
    AssetId partImages [PartKindCount][MaxPartCount] = {};
};

bool load_part_images(VPartImages& images, AssetCatalog& catalog);

const char* get_structure_part_name(StructurePartKind kind);
const char* get_ground_part_name(GroundPartKind kind);
const char* get_computer_part_name(ComputerKind kind);

cobot::vec2 get_part_scale(PartKindId id);
cobot::vec2 get_structure_part_scale(StructurePartKind kind);
cobot::vec2 get_ground_part_scale(GroundPartKind kind);
cobot::vec2 get_computer_part_scale(ComputerKind kind);

SDL_Texture* get_part_texture(PartKindId partKind, AssetCatalog& catalog);

bool load_ground_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);
bool load_structure_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);
bool load_computer_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);

#endif // PARTS_HPP
