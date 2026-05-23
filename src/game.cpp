#include "game.hpp"
#include "draw.hpp"


bool GameState::load_part_images(AssetCatalog& catalog)
{
    for (int kind = 0; kind < PART_KIND_COUNT; kind ++) {
        for (int i = 0; i < ChasisKindCount; i++) {
            String name = String(get_chasis_name(ChasisKind(i)));
            AssetId id = get_asset(name, catalog);
            if (!id.is_valid()) return false;
            partImages[kind][i] = id;
        }
    }
    return true;
}

void draw_vehicle(const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    for (int i = 0; i < game.vehicle.chasis.size(); i++)
    {
        SDL_Texture* texture = catalog.get_image(game.partImages[PART_CHASIS][i]);
        Rectangle area = { 200, 200, 500, 500 };
        render_texture(context.renderer, area, texture, true);
    }

    for (int i = 0; i < game.vehicle.tire.size(); i++)
    {
        SDL_Texture* texture = catalog.get_image(game.partImages[PART_TIRE][i]);
        Rectangle area = { 200, 200, 500, 500 };
        render_texture(context.renderer, area, texture, true);
    }

    for (int i = 0; i < game.vehicle.controller.size(); i++)
    {
        SDL_Texture* texture = catalog.get_image(game.partImages[PART_CONTROLLER][i]);
        Rectangle area = { 200, 200, 500, 500 };
        render_texture(context.renderer, area, texture, true);
    }
}
