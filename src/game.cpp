#include "game.hpp"
#include "draw.hpp"


bool GameState::load_part_images(AssetCatalog& catalog)
{
    for (int i = 0; i < ChasisKindCount; i++) {
        String name = String(get_chasis_name(ChasisKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        partImages[PART_CHASIS][i] = id;
    }
    
    for (int i = 0; i < TireKindCount; i++) {
        String name = String(get_tire_name(TireKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        partImages[PART_TIRE][i] = id;
    }

    for (int i = 0; i < ControllerKindCount; i++) {
        String name = String(get_controller_name(ControllerKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        partImages[PART_CONTROLLER][i] = id;
    }

    return true;
}

void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const GameState& game);

void draw_chasis(const Chasis& chasis, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    AssetId imageId = game.partImages[PART_CHASIS][chasis.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    float scale = chasis.part.transform.scale * parent.scale;
    Rectangle area = Rectangle(chasis.part.transform.position + parent.position, get_chasis_scale(chasis.kind) * scale);
    render_texture(context.renderer, area, texture, true);

    switch (chasis.kind) {
        case ChasisBasic: {
            auto passDown = chain_part_transform(parent, chasis.part.transform);
            draw_vehicle_part(chasis.basic.frontLeft.part, passDown, context, catalog, game);
            draw_vehicle_part(chasis.basic.frontRight.part, passDown, context, catalog, game);
            draw_vehicle_part(chasis.basic.backLeft.part, passDown, context, catalog, game);
            draw_vehicle_part(chasis.basic.backRight.part, passDown, context, catalog, game);
            break;
        }
    }
}

void draw_tire(const Tire& tire, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    AssetId imageId = game.partImages[PART_TIRE][tire.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    float scale = tire.part.transform.scale * parent.scale;
    Rectangle area = Rectangle(tire.part.transform.position + parent.position, get_tire_scale(tire.kind) * scale);
    render_texture(context.renderer, area, texture, true);
}

void draw_controller(const Controller& controller, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    AssetId imageId = game.partImages[PART_CONTROLLER][controller.kind];
    SDL_Texture* texture = catalog.get_image(imageId);

    float scale = controller.part.transform.scale * parent.scale;
    Rectangle area = Rectangle(controller.part.transform.position + parent.position, get_controller_scale(controller.kind) * scale);
    render_texture(context.renderer, area, texture, true);
}


void draw_vehicle_part(PartId part, VPartTransform parent, const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    switch (part.kind)
    {
        case PART_CHASIS: {
            const Chasis& chasis = game.vehicle.chasis[part.index];
            draw_chasis(chasis, parent, context, catalog, game);
            break;
        }
        case PART_TIRE: {
            const Tire& tire = game.vehicle.tire[part.index];
            draw_tire(tire, parent, context, catalog, game);
            break;
        }
        case PART_CONTROLLER: {
            const Controller& controller = game.vehicle.controller[part.index];
            draw_controller(controller, parent, context, catalog, game);
            break;
        }
        default: panic("Invalid part type");
    }
}

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    for (int i = 0; i < game.vehicle.rootParts.size(); i++)
    {
        VPartData part_data = game.vehicle.getPartData(game.vehicle.rootParts[i]);
        part_data.transform.position += game.vehicle.worldPosition;
        draw_vehicle_part(game.vehicle.rootParts[i], part_data.transform, context, catalog, game);
    }
}
