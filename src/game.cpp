#include "game.hpp"
#include "draw.hpp"
#include "log.hpp"

constexpr int fixedUpdateRate = 40;
constexpr double fixedTimeStep = 1.0 / fixedUpdateRate;
constexpr s64 fixedTimeStepNS = NANOSECONDS_PER_SECOND / fixedUpdateRate;
constexpr s64 fixedTimeStepUS = MICROSECONDS_PER_SECOND / fixedUpdateRate;
constexpr s64 fixedTimeStepMS = MILLISECONDS_PER_SECOND / fixedUpdateRate;

void GameState::update(TimeInfo time)
{
    constexpr int maxIterationsPerFrame = 50;
    int iterations = 0;
    while ((updateState->elapsed < time.timeSeconds + time.deltaTimeSeconds) && iterations < maxIterationsPerFrame)
    {
        updateState->fixedUpdate(this);
        updateState->elapsed += fixedTimeStep;
        updateState->ticks += 1;

        iterations += 1;
    }

    updateState->update(this, time);
}

void idleUpdate(GameState* game, TimeInfo time) {}
void idleFixedUpdate(GameState* game) {}

void vehicleSimulationUpdate(GameState* game, TimeInfo time)
{
    double dts = time.deltaTimeSeconds;
    game->vehicle.worldPosition += dts * game->vehicle.velocity;

    for (auto& controller : game->vehicle.controller)
    {
        Script& s = game->scripts.get_ref(controller.script);
        run_script(s);
    }
}

void vehicleSimulationFixedUpdate(GameState* game)
{}

void starSystemUpdate(GameState* game, TimeInfo time)
{}

void starSystemFixedUpdate(GameState* game)
{
    game->starSystem.simulation_step(fixedTimeStep * game->updateState->timeScale);
}


Rectangle GameState::get_planet_screen_area(vec2 ws, int planet) const
{
    vec2 origin = ws / 2;
    const Planet& p = starSystem.planets.get_ref(planet);
    vec2 pos = origin + p.body.position.xy();
    return Rectangle(pos, vec2(p.body.radius));
}

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

void draw_game_state(const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    draw_vehicle(context, catalog, game);
}

void draw_game_star_system(const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    vec2 render_size = context.render_size;

    auto& system = game.starSystem;
    
    vec2 center = render_size / 2;
    draw_circle(context, center, system.star.radius, ColorF(0.6, 0.5, 0.1));

    float maxDepth = 10;
    for (auto& planet : system.planets)
    {
        vec2 pos = vec2(planet.body.position.x, planet.body.position.y);
        float zdistance = cobot::smoothstep(-maxDepth / 2, maxDepth / 2, planet.body.position.z);
        // remap to 0.5 - 1.0 range
        zdistance = (zdistance + 1.0f) / 2;
        draw_circle(context, center + pos, planet.body.radius, ColorF(planet.color, zdistance));
    }
}

void draw_orbits(RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    for (int i = 0; i < game.starSystem.planets.size(); i++)
    {
        draw_planet_orbit(context, game.starSystem.planets.get_ref(i), context.render_size / 2, game.starSystem.star.mass, 2);
    }
}

void draw_planet_orbit(RenderContext& context, const Planet& planet, vec2 offset, double centralBodyMass, float thick)
{
    Body body = planet.body;
    ColorF color = planet.color;

    constexpr float stepSize = 0.01;
    constexpr int numSteps = CONSTANT_TAU / stepSize;
    vec2 points[numSteps];

    float angle = 0;
    for (int index = 0; index < numSteps; index += 1)
    {
        body.trueAnomaly = angle;
        body.determine_state_vector(centralBodyMass);
        vec3 p = body.position;

        points[index] = offset + vec2(p.x, p.y);
        angle += stepSize;
    }

    draw_closed_path(context, points, numSteps, thick, color);
}

void draw_planet_outline(RenderContext& context, const GameState& game, int planetIndex)
{
    vec2 origin = context.render_size / 2;
    Planet& planet = game.starSystem.planets.get_ref(planetIndex);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(10), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(100), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(190), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(280), cobot::degree_to_radian_f(70), planet.color);
}
