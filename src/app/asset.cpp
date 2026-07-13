#include "asset.hpp"
#include "util/log.hpp"
#include "util/string_util.hpp"
#include "util/file_util.hpp"

#include <SDL3_image/SDL_image.h>

bool parse_image_attribute(String attribute, SDL_Texture*& texture);
bool parse_audio_attribute(String attribute, TrackId& audio);
bool parse_font_attribute(String attribute, Font& font);
bool parse_shader_attribute(String attribute, Shader& shader);

bool parse_attribute_value(String attribute, const char* key, String& out_value);

String next_word(String source, int& offset)
{
    offset += string_match_character(source, offset, ' ');
    String word = string_slice_to_character(source, offset, ' ');
    offset += word.size;
    return word;
}

#define ASSET_LINE_IS_VALID              BIT(0)
#define ASSET_LINE_IS_COMMENT            BIT(1)
#define ASSET_LINE_IS_EMPTY              BIT(2)
#define ASSET_LINE_HAS_TRAILING_TOKENS   BIT(3)
#define ASSET_LINE_REPEATS_TOKENS        BIT(4)

using AssetParseLineResult = u32;

// return false on failure or comment
// <kind> <scope optional> <name> <path> [optional] [lazy]
AssetParseLineResult asset_parse_line(String file, String line, Asset& pointer)
{
#if LOG_VERBOSE
    SCOPE_STRING(line, cstr);
    log_info("Asset entry: %s\n", cstr);
#endif

    u32 result = 0;
    AssetFlags flags = 0;

    line.trim();

    if (line.size == 0)
    {
        return ASSET_LINE_IS_EMPTY;
    }

    if (string_starts_with(line, make_string("#")))
    {
        return ASSET_LINE_IS_COMMENT;
    }

    String image = make_string("image");
    String audio = make_string("audio");
    String shader = make_string("shader");
    String font = make_string("font");

    int cursor = 0;
    String kind = string_slice_to_character(line, 0, ' ');

    AssetKind asset_kind;

    if (string_compare(kind, image))
    {
        asset_kind = ASSET_KIND_IMAGE;
        cursor += image.size;
    }
    else if (string_compare(kind, audio))
    {
        asset_kind = ASSET_KIND_AUDIO;
        cursor += audio.size;
    }
    else if (string_compare(kind, shader))
    {
        asset_kind = ASSET_KIND_SHADER;
        cursor += shader.size;
    }
    else if (string_compare(kind, font))
    {
        asset_kind = ASSET_KIND_FONT;
        cursor += font.size;
    }
    else {
        return 0;
    }

    // space required after a field
    if (line.data[cursor] != ' ')
    {
        return 0;
    }

    bool is_folder = false;

    String name = {};
    String scope = next_word(line, cursor);
    if (string_compare(scope, make_string("file")))
    {
        is_folder = false;
        name = next_word(line, cursor);
    }
    else if (string_compare(scope, make_string("folder")))
    {
        is_folder = true;
        name = next_word(line, cursor);
    }
    else
    {
        // scope is an optional argument and it's missing
        is_folder = false;

        name = scope;
        scope = {};
    }

    flags |= is_folder ? ASSET_IS_FOLDER : 0;

    String path = next_word(line, cursor);

    if (name.size == 0 || path.size == 0)
    {
        return 0;
    }

    String next = next_word(line, cursor);
    bool attribute = true;
    while (attribute)
    {
        switch (asset_kind)
        {
            case ASSET_KIND_IMAGE: {
                if (!parse_image_attribute(next, pointer.data.image))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_AUDIO: {
                if (!parse_audio_attribute(next, pointer.data.audio))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_FONT: {
                if (!parse_font_attribute(next, pointer.data.font))
                {
                    attribute = false;
                }
                break;
            }
            case ASSET_KIND_SHADER: {
                if (!parse_shader_attribute(next, pointer.data.shader))
                {
                    attribute = false;
                }
                break;
            }
            default:
                panic("Invalid asset kind");
                break;
        }

        if (attribute)
        {
            next = next_word(line, cursor);
        }
    }

    while (next.size > 0)
    {
        if (string_compare(next, String("lazy")))
        {
            if (flags & ASSET_IS_LAZY) { result |= ASSET_LINE_REPEATS_TOKENS; }
            flags |= ASSET_IS_LAZY;
        }
        else if (string_compare(next, String("optional")))
        {
            if (flags & ASSET_IS_OPTIONAL) { result |= ASSET_LINE_REPEATS_TOKENS; }
            flags |= ASSET_IS_OPTIONAL;
        }
        else {
            break;
        }

        next = next_word(line, cursor);
    }

    String trail = string_slice_to_character(line, cursor, '\n');
    trail.trim();
    if (trail.size != 0)
    {
        result |= ASSET_LINE_HAS_TRAILING_TOKENS;
    }

    pointer.kind = asset_kind;
    pointer.flags = flags;
    pointer.name = StringReference(name.data - file.data, name.size);
    pointer.path = StringReference(path.data - file.data, path.size);
    pointer.identifier = NullAssetId;

    result |= ASSET_LINE_IS_VALID;
    return result;
}

bool parse_image_attribute(String attribute, SDL_Texture*& texture)
{
    return false;
}

bool parse_audio_attribute(String attribute, TrackId& audio)
{
    return false;
}

bool parse_font_attribute(String attribute, Font& font)
{
    String out = {};

    if (parse_attribute_value(attribute, "size", out)) {
        bool success = false;
        float value = string_to_real(out, &success);
        if (!success) {
            return false;
        }

        font.size = value;

        return true;
    }

    return false;
}

bool parse_shader_attribute(String attribute, Shader& shader)
{
    String out = {};

    if (parse_attribute_value(attribute, "stage", out)) {
        if (string_compare(out, String("vertex"))) {
            shader.stage = ShaderStageVertex;
        }
        else if (string_compare(out, String("fragment"))) {
            shader.stage = ShaderStageFragment;
        }
        else {
            return false;
        }

        return true;
    }
    else if (parse_attribute_value(attribute, "nUniforms", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) {
            return false;
        }

        shader.numUniformBuffers = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nSamplers", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) {
            return false;
        }

        shader.numSamplers = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nTextures", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) return false;
        shader.numStorageTextures = value;
        return true;
    }
    else if (parse_attribute_value(attribute, "nStorageBuffers", out)) {
        bool success = false;
        int value = string_to_integer(out, &success);
        if (!success) return false;
        shader.numStorageBuffers = value;
        return true;
    }

    return false;
}

bool parse_attribute_value(String attribute, const char* key, String& out_value)
{
    String k = make_string(key);
    if (!string_starts_with(attribute, k))
        return false;
    if (!attribute.advance(k.size))
        return false;
    attribute.trim();
    if (attribute.size == 0 || attribute.data[0] != '=')
        return false;
    attribute.advance(1);
    attribute.trim();
    out_value = attribute;
    return true;
}

bool parse_assets(const char* description, AssetCatalog& catalog)
{
    catalog.catalog.clear();
    bool success = load_file_text(description, catalog.catalog);
    if (!success)
    {
        return false;
    }

    return parse_asset_description(catalog.catalog.c_string(), catalog);
}

bool parse_asset_description(const char* description, AssetCatalog& catalog)
{
    int cursor = 0;
    int line_number = 0;

    String desc = make_string(description);
    while (cursor < desc.size)
    {
        String line = string_slice_to_character(desc, cursor, '\n');
        int next_line_offset = cursor + line.size;
        next_line_offset += string_match_character(desc, next_line_offset, '\n');

        Asset asset = {};
        auto result = asset_parse_line(desc, line, asset);

        line_number += 1;

        if (result & ASSET_LINE_IS_VALID)
        {
            catalog.add_asset(asset);

            if (result & ASSET_LINE_HAS_TRAILING_TOKENS)
            {
                log_warning("Asset description %s has trailing tokens on line %d\n", description, line_number);
            }
        }
        else
        {
            if ((result & ASSET_LINE_IS_COMMENT) || (result & ASSET_LINE_IS_EMPTY))
            {
                // do nothing
            }
            else
            {
                SCOPE_STRING(line, line_cstr);
                log_error("Could not parse line %d in asset description. Line: %s", line_number, line_cstr);
                return false;
            }
        }

        cursor = next_line_offset;
    }

    return true;
}

SDL_EnumerationResult asset_callback(void* userdata, const char* dirname, const char* fname);

bool load_asset(String_Builder& path, Asset& asset, AssetLoadContext& load_context);

AssetId get_asset(String name, AssetCatalog& catalog)
{
    for (auto& asset : catalog.assets)
    {
        auto asset_name = catalog.catalog.get_string(asset.name);
        if (string_compare(asset_name, name))
        {
            if (asset.identifier.is_valid())
            {
                return asset.identifier;
            }
            else
            {
                if (asset.flags & ASSET_IS_FOLDER) {
                    auto asset_path = catalog.get_asset_path(asset.identifier);
                    SCOPE_STRING(asset_path, folder);
                    get_to_run_tree_path(catalog.path, folder);
                    catalog.path.append(make_string(PathSeparator));

                    if (!SDL_EnumerateDirectory(catalog.path.c_string(), asset_callback, &catalog)) {
                        log_error("Couldn't load assets in folder: %s", asset_path);
                        return NullAssetId;
                    }

                    asset.identifier.generation += 1;
                    return asset.identifier;
                }
                else {
                    auto asset_path = catalog.catalog.get_string(asset.path);
                    SCOPE_STRING(asset_path, asset_path_c);
                    get_to_run_tree_path(catalog.path, asset_path_c);

                    bool load = load_asset(catalog.path, asset, catalog.load_context);
                    if (!load)
                    {
                        log_error("Couldn't load asset: %s", asset_path);
                        return NullAssetId;
                    }

                    asset.identifier.generation += 1;
                    return asset.identifier;
                }
            }
        }
    }

    SCOPE_STRING(name, asset_name);
    log_error("Couldn't find requested asset with name: %s", asset_name);
    return NullAssetId;
}

AssetId get_asset_at_index(int index, AssetCatalog& catalog)
{
    if (!catalog.assets.in_bounds(index))
    {
        log_error("Couldn't load asset: Requested asset index is out of bounds %d", index);
        return NullAssetId;
    }

    if (catalog.assets[index].identifier.is_valid())
    {
        return catalog.assets[index].identifier;
    }
    else
    {
        if (catalog.assets[index].flags & ASSET_IS_FOLDER) {
            auto asset_path = catalog.get_asset_path_at_index(index);
            SCOPE_STRING(asset_path, folder);
            get_to_run_tree_path(catalog.path, folder);
            catalog.path.append(make_string(PathSeparator));

            if (!SDL_EnumerateDirectory(catalog.path.c_string(), asset_callback, &catalog)) {
                log_error("Couldn't load assets in folder: %s", asset_path);
                return NullAssetId;
            }

            catalog.assets[index].identifier.generation += 1;
            return catalog.assets[index].identifier;
        }
        else {
            auto asset_path = catalog.get_asset_path_at_index(index);
            SCOPE_STRING(asset_path, asset_path_c);
            get_to_run_tree_path(catalog.path, asset_path_c);

            bool load = load_asset(catalog.path, catalog.assets[index], catalog.load_context);
            if (!load)
            {
                log_error("Couldn't load asset: %s", asset_path);
                return NullAssetId;
            }

            catalog.assets[index].identifier.generation += 1;
            return catalog.assets[index].identifier;
        }
    }
}

bool load_asset(String_Builder& path, Asset& asset, AssetLoadContext& load_context)
{
    switch (asset.kind)
    {
        case ASSET_KIND_IMAGE: {
            SDL_Texture* texture = IMG_LoadTexture(load_context.render->renderer, path.c_string());
            if (!texture)
            {
                asset.identifier.id = -1;
                return false;
            }

            asset.data.image = texture;

            return true;
        }
        case ASSET_KIND_AUDIO: {
            TrackId track = load_context.audio->add_track(path.c_string());
            if (track == NullTrackId)
            {
                asset.identifier.id = -1;
                return false;
            }

            asset.data.audio = track;

            return true;
        }
        case ASSET_KIND_FONT: {
            bool success = load_font_file(&asset.data.font, path.c_string(), asset.data.font.size);
            if (!success)
            {
                asset.identifier.id = -1;
                return false;
            }

            return true;
        }
        case ASSET_KIND_SHADER: {
            bool success = loadShader(*load_context.render, asset.data.shader, path.c_string());
            if (!success)
            {
                asset.identifier.id = -1;
                return false;
            }

            return true;
        }
        default: {
            return false;
        }
    }
}

SDL_EnumerationResult asset_callback(void* userdata, const char* dirname, const char* fname)
{
    AssetCatalog* catalog = (AssetCatalog*)userdata;

    String name = catalog->catalog.put_string(String(fname));

    String ext = string_get_extension(name);
    String file = string_get_file_name(name);
    AssetKind kind = get_asset_kind(ext);
    if (kind == ASSET_KIND_SENTINEL) {
        // we don't recognize the extension of this file
        return SDL_ENUM_CONTINUE;
    }

    Asset asset = {};
    asset.kind = kind;
    asset.flags = ASSET_IS_FROM_FOLDER;
    asset.name = catalog->catalog.get_reference(file);
    asset.path = {};

    int amount = catalog->path.append_path(String(fname));

    // we could make this recursize but maybe not necessary
    if (!load_asset(catalog->path, asset, catalog->load_context)) {
        log_error("Couldn't load asset: %s/%s", dirname, fname);
        return SDL_ENUM_FAILURE;
    }

    catalog->path.remove(amount);

    int index = catalog->assets.add(asset);
    catalog->assets.get_ref(index).identifier = { index, 1 };

    return SDL_ENUM_CONTINUE;
}

AssetKind get_asset_kind(String extension) {
    if (string_compare(extension, String("svg"))) {
        return ASSET_KIND_IMAGE;
    }
    else if (string_compare(extension, String("ogg"))) {
        return ASSET_KIND_AUDIO;
    }
    else if (string_compare(extension, String("wav"))) {
        return ASSET_KIND_AUDIO;
    }
    else if (string_compare(extension, String("ttf"))) {
        return ASSET_KIND_FONT;
    }
    else if (string_compare(extension, String("shader"))) {
        return ASSET_KIND_SHADER;
    }

    return ASSET_KIND_SENTINEL;
}

void get_base_path(String_Builder& builder)
{
    const char* base_path = SDL_GetBasePath();
    builder.clear_and_append(make_string(base_path));
}

void get_pref_path(String_Builder& builder, const char *org, const char *app)
{
    char* pref_path = SDL_GetPrefPath(org, app);
    builder.clear_and_append(make_string(pref_path));
    SDL_free(pref_path);
}

void get_to_run_tree_path(String_Builder& builder, const char* path)
{
    get_base_path(builder);
    builder.append_path(String("asset/"));
    builder.append_path(String(path));
}
