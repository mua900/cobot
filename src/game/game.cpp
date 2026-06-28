#include "app/draw.hpp"
#include "game.hpp"
#include "log.hpp"

void GameState::update(TimeInfo time)
{
    constexpr int maxIterationsPerFrame = 50;
    int iterations = 0;
	double timeStep = updateState->calculateTimeStep();
    while ((updateState->elapsed < time.timeSeconds + time.deltaTimeSeconds) && iterations < maxIterationsPerFrame)
    {
        updateState->fixedUpdate(this);
        updateState->elapsed += timeStep;
        updateState->ticks += 1;

        iterations += 1;
    }

    updateState->update(this, time);
}

void idleUpdate(GameState* game, TimeInfo time) {}
void idleFixedUpdate(GameState* game) {}

void vehicleSimulationUpdate(GameState* game, TimeInfo time)
{
    Vehicle* vehicle = game->get_active_vehicle();

    for (auto& computer : vehicle->computerPart)
    {
        Script& s = vehicle->scripts.get(computer.script);
        run_script(s);

        if (!s.commands.empty())
        {
            if (vehicle->execute_command(*s.commands.get_start()))
            {
                s.commands.remove_start();
            }
        }
    }

    vehicle->worldPosition += time.deltaTimeSeconds * vehicle->velocity;
}

void vehicleSimulationFixedUpdate(GameState* game)
{}

void starSystemUpdate(GameState* game, TimeInfo time)
{}

void starSystemFixedUpdate(GameState* game)
{
    game->starSystem.simulation_step(game->updateState->calculateTimeStep() * game->updateState->timeScale);
}

void keyboardIdle(GameState* game, KeyboardState* keyboard) {}

void keyboardVehicle(GameState* game, KeyboardState* keyboard) {
    if (!keyboard->do_input)
    {
        return;
    }

    Vehicle* vehicle = game->get_active_vehicle();
    if (keyboard->key_pressed(KEY_UP)) {
        vehicle->velocity = vehicle->speed * vehicle->forward();
    }
    else if (keyboard->key_pressed(KEY_DOWN)) {
        vehicle->velocity = -vehicle->speed * vehicle->forward();
    }
    else {
        vehicle->velocity = cobot::vec2(0);
    }

    if (keyboard->key_pressed(KEY_LEFT)) {
        vehicle->orientation -= 0.1;
    }
    else if (keyboard->key_pressed(KEY_RIGHT)) {
        vehicle->orientation += 0.1;
    }
}

void keyboardStarSystem(GameState* game, KeyboardState* keyboard) {}


cobot::Rectangle GameState::get_planet_screen_area(cobot::vec2 ws, int planet) const
{
    cobot::vec2 origin = ws / 2;
    const Planet& p = starSystem.planets.get_ref(planet);
    cobot::vec2 pos = origin + p.body.position.xy();
    return cobot::Rectangle(pos, cobot::vec2(p.body.radius));
}

Vehicle* GameState::get_active_vehicle() const
{
    if (vehicles.size() == 0) return nullptr;
    return vehicles.get_ptr(active_vehicle);
}

bool load_part_images(VPartImages& images, AssetCatalog& catalog)
{
    for (int i = 0; i < StructurePartKindCount; i++) {
        String name = String(get_structure_part_name(StructurePartKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        images.partImages[PartStructure][i] = id;
    }
    
    for (int i = 0; i < GroundPartKindCount; i++) {
        String name = String(get_ground_part_name(GroundPartKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        images.partImages[PartGround][i] = id;
    }

    for (int i = 0; i < ComputerKindCount; i++) {
        String name = String(get_computer_part_name(ComputerKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        images.partImages[PartComputer][i] = id;
    }

    return true;
}

void draw_vehicle_simulation(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partimages)
{
    draw_vehicle(context, catalog, vehicle, VehicleDrawParameters(&partimages, NullPartId, false));
}

void draw_star_system(const RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    cobot::vec2 render_size = context.render_size;
    auto& system = game.starSystem;

    draw_circle(context, cobot::vec2(0,0), system.star.radius, cobot::ColorF(0.6, 0.5, 0.1));

    // @todo fix
    // SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanet], 0, nullptr, sizeof(nullptr));
    // SDL_SetGPURenderState(context.renderer, context.render_states[RenderStatePlanet]);

    float maxDepth = 10;
    for (auto& planet : system.planets)
    {
        float zdistance = cobot::smoothstep(-maxDepth / 2, maxDepth / 2, planet.body.position.z);
        // remap to 0.5 - 1.0 range
        zdistance = (zdistance + 1.0f) / 2;
        // draw_circle(context, center + pos, planet.body.radius, ColorF(planet.color, zdistance));
        draw_circle_with_texture(context, planet.body.position.xy(), planet.body.radius, planet.map, cobot::ColorF(planet.color, zdistance));
    }

    SDL_SetGPURenderState(context.renderer, nullptr);
}

void draw_orbits(RenderContext& context, const AssetCatalog& catalog, const GameState& game)
{
    for (int i = 0; i < game.starSystem.planets.size(); i++)
    {
        draw_planet_orbit(context, game.starSystem.planets.get_ref(i), game.starSystem.star.mass, 2);
    }
}

void draw_planet_orbit(RenderContext& context, const Planet& planet, double centralBodyMass, float thick)
{
    Body body = planet.body;
    cobot::ColorF color = planet.color;

    constexpr float stepSize = 0.01;
    constexpr int numSteps = CONSTANT_TAU / stepSize;
    cobot::vec2 points[numSteps];

    float angle = 0;
    for (int index = 0; index < numSteps; index += 1)
    {
        body.parameters.trueAnomaly = angle;
        body.determine_state_vector(centralBodyMass);

        points[index] = body.position.xy();
        angle += stepSize;
    }

    draw_closed_path(context, points, numSteps, thick, color);
}

void draw_planet_outline(RenderContext& context, const GameState& game, int planetIndex)
{
    cobot::vec2 origin = context.render_size / 2;
    Planet& planet = game.starSystem.planets.get_ref(planetIndex);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(10), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(100), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(190), cobot::degree_to_radian_f(70), planet.color);
    draw_arc(context, origin + planet.body.position.xy(), planet.body.radius + 5, planet.body.radius + 10, cobot::degree_to_radian_f(280), cobot::degree_to_radian_f(70), planet.color);
}
