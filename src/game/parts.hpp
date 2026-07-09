#ifndef PARTS_HPP
#define PARTS_HPP

#include "app/asset.hpp"
#include "app/text.hpp"
#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "script.hpp"

enum PartCategory {
    CategoryPower,
    CategoryStructure,
    CategoryComputer,
    CategoryGround,
    CategoryInstrument,
};

// to add a part add it to getPartCategory, get_part_name and get_part_scale
// alongside adding icons and part images for it to assets
enum PartKind {
    PartKindWheel = 0,
    PartKindChassis,
	PartKindSolarPanel,
    PartKindBattery,
    PartKindComputer,
    PartKindThermometer,
    PartKindLidar,
    PartKindCount,
    PartKindSentinel
};

#define PART_KIND_MASK(partKind) BIT(partKind)

using PartId = u32;
static const PartId NullPartId = -1;

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

    VPartData() {}
    VPartData(cobot::vec2 position) : transform(position, 1.0) {}
    VPartData(cobot::vec2 position, float scale) : transform(position, scale) {}
    VPartData(PartId parent, cobot::vec2 position, float scale) : parent(parent), transform(position, scale) {}
};

// @todo grid

struct AttachmentPoint {
    cobot::vec2 position = {};
    PartId part = {};  // the part that is attached
    // @todo
    // u32 mask;
	bool used = false;

    AttachmentPoint() {}
    AttachmentPoint(cobot::vec2 pos, PartId attached) : position(pos), part(attached) {}

    bool attach(PartId part_id) {
        part = part_id;
		used = true;
        return true;
    }

    bool unattach() {
        if (!used)
        {
            return false;
        }

        used = false;
        part = NullPartId;
        return true;
    }
};

struct Computer {
    u32 script = 0;
    u32 codeSizeLimit = 0;
};

enum ChassisAttachment {
    ChassisFrontLeft,
    ChassisFrontRight,
    ChassisBackLeft,
    ChassisBackRight,
    ChassisTop,
    ChassisAttachmentCount
};

struct Chassis {
    // @todo parameters
    AttachmentPoint points[ChassisAttachmentCount] = {};

    Chassis() {}
    void init();
};

struct Wheel {
    // @todo parameters
    float diameter = 0;
    float tranction = 0;
};

struct SolarPanel {
    // @todo parameters
    float photonFlux = 0;
};

struct Battery {
    float capacity;
    // tracked at the top level
    // float storedCharge;
};

struct VehiclePart {
    PartKind kind = PartKindSentinel;
    VPartData partData = {};
    union {
        Chassis chassis;
        Wheel wheel;
        Computer computer;
        SolarPanel solarPanel;
        Battery battery;
    } data = {};

    VehiclePart() : kind(PartKindSentinel) {}
    VehiclePart(PartKind type) : kind(type) {}
    VehiclePart(Chassis c) : kind(PartKindChassis) {
        data.chassis = c;
    }
    VehiclePart(Wheel w) : kind(PartKindWheel) {
        data.wheel = w;
    }
    VehiclePart(Computer c) : kind(PartKindComputer) {
        data.computer = c;
    }
    VehiclePart(SolarPanel sp) : kind(PartKindSolarPanel) {
        data.solarPanel = sp;
    }
    VehiclePart(Battery b) : kind(PartKindBattery) {
        data.battery = b;
    }

    void init();

    Array<AttachmentPoint> getAttachments();
};

struct AttachmentDistance {
    AttachmentPoint* point;
    PartId parent;
    float distance;
    const char* name = nullptr;
};

struct VPartImages {
    AssetId partImages [PartKindCount] = {};
};

bool load_part_images(VPartImages& images, AssetCatalog& catalog);
const char* get_part_name(PartKind kind);
cobot::vec2 get_part_scale(PartKind id);
SDL_Texture* get_part_texture(PartKind partKind, AssetCatalog& catalog);
bool load_part_icons(DArray<IconButton>& icons, cobot::Color background, AssetCatalog& catalog);

PartCategory getPartCategory(PartKind kind);

#endif // PARTS_HPP
