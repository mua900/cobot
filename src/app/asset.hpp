#ifndef ASSET_HPP
#define ASSET_HPP

#include "common.hpp"
#include "template.hpp"
#include "text.hpp"
#include "audio.hpp"
#include "draw.hpp"

enum AssetKind {
    ASSET_KIND_ZERO   = 0,
    ASSET_KIND_IMAGE  = 1,
    ASSET_KIND_AUDIO  = 2,
    ASSET_KIND_FONT   = 3,
    ASSET_KIND_SHADER = 4,
    ASSET_KIND_COUNT,
    ASSET_KIND_SENTINEL,
};

struct AssetId {
    int id;
    int generation;

    bool is_valid() const { return id != -1 && generation != 0; }
};

static constexpr AssetId NullAssetId = AssetId {-1, 0};

struct AssetLoadContext {
    RenderContext* render;
    AudioPlayer* audio;
};

using AssetFlags = u8;

// by default 0
#define ASSET_IS_FOLDER      BIT(0)
#define ASSET_IS_OPTIONAL    BIT(1)
#define ASSET_IS_LAZY        BIT(2)
#define ASSET_IS_FROM_FOLDER BIT(3)

struct Asset {
    AssetKind kind;
    StringReference name = {};
    StringReference path = {};
    AssetFlags flags = 0;

    AssetId identifier = {};
    union {
        Font font;
        SDL_Texture* image;
        TrackId audio;
        Shader shader;
    } data = {};

    Asset() : kind(ASSET_KIND_ZERO), identifier(NullAssetId), data{} {}
    Asset(AssetKind kind) : kind(kind), identifier(NullAssetId), data{}
    {}
};

struct AssetCatalog {
    String_Builder catalog;
    AssetLoadContext load_context;
    DArray<Asset> assets;
    String_Builder path;

    void add_asset(Asset& asset)
    {
        int index = assets.add(asset);
        assets.get_ref(index).identifier.id = index;
        assets.get_ref(index).identifier.generation = 0;
    }

    String get_asset_name_at_index(int index) const {
        return catalog.get_string(assets.get_ref(index).name);
    }

    String get_asset_path_at_index(int index) const {
        return catalog.get_string(assets.get_ref(index).path);
    }

    String get_asset_name(AssetId id) const {
        if (!id.is_valid()) return String();

        const Asset& asset = assets.get_ref(id.id);
        return catalog.get_string(asset.name);
    }

    String get_asset_path(AssetId id) const {
        if (!id.is_valid()) return String();

        const Asset& asset = assets.get_ref(id.id);
        return catalog.get_string(asset.path);
    }

    SDL_Texture* get_image(AssetId id) const
    {
        if (!id.is_valid())
        {
            return nullptr;
        }

        const Asset& asset = assets.get_ref(id.id);
        if (asset.kind != ASSET_KIND_IMAGE || asset.identifier.generation != id.generation)
        {
            return nullptr;
        }

        return asset.data.image;
    }

    Font get_font(AssetId id) const
    {
        if (!id.is_valid())
        {
            return Font();
        }

        const Asset& asset = assets.get_ref(id.id);
        if (asset.kind != ASSET_KIND_FONT || asset.identifier.generation != id.generation)
        {
            return Font();
        }

        return asset.data.font;
    }

    TrackId get_audio(AssetId id) const
    {
        if (!id.is_valid())
        {
            return NullTrackId;
        }

        const Asset& asset = assets.get_ref(id.id);
        if (asset.kind != ASSET_KIND_AUDIO || asset.identifier.generation != id.generation)
        {
            return NullTrackId;
        }

        return asset.data.audio;
    }

    SDL_GPUShader* get_shader(AssetId id) const
    {
        if (!id.is_valid())
        {
            return nullptr;
        }

        const Asset& asset = assets.get_ref(id.id);
        if (asset.kind != ASSET_KIND_SHADER || asset.identifier.generation != id.generation)
        {
            return nullptr;
        }

        return asset.data.shader.shader;
    }
};

// parse asset catalog file and add assets listed in it to the catalog
bool parse_asset_description(const char* description, AssetCatalog& catalog);
// load the file pointed to by path and call parse_asset_description on it and return it's result
bool parse_assets(const char* path, AssetCatalog& catalog);
// returns the existing handle if the asset is already loaded otherwise loads the asset on the fly and returns the handle
// returns null id if no asset with the given name is found or the asset load fails
AssetId get_asset(String name, AssetCatalog& catalog);
// useful when iterating through the assets and you already know the index
AssetId get_asset_at_index(int index, AssetCatalog& catalog);

AssetKind get_asset_kind(String file_extension);

void get_base_path(String_Builder& builder);
void get_to_run_tree_path(String_Builder& builder, const char* path);

#endif // ASSET_HPP
