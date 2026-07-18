#include "app/draw.hpp"
#include "game.hpp"
#include "util/log.hpp"

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

void GameState::set_default_game_data()
{
    
}

void idleUpdate(GameState* game, TimeInfo time) {}
void idleFixedUpdate(GameState* game) {}

void vehicleSimulationUpdate(GameState* game, TimeInfo time)
{
    Vehicle* vehicle = game->get_active_vehicle();

    for (auto& part : vehicle->parts)
    {
        if (part.kind == PartKindComputer)
        {
            auto& computer = part.data.computer;

            Script& s = vehicle->scripts.get(computer.script);

            if (!s.commands.is_empty())
            {
                if (s.activeCommand == s.commands.size())
                {
                    continue;
                }

                if (vehicle->execute_command(s.commands.get_ref(s.activeCommand)))
                {
                    s.activeCommand += 1;
                }
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

Vehicle* GameState::get_active_vehicle() const
{
    if (vehicles.size() == 0) return nullptr;
    return vehicles.get_ptr(active_vehicle);
}

bool load_part_images(VPartImages& images, AssetCatalog& catalog)
{
    for (int i = 0; i < PartKindCount; i++)
    {
        String name = String(get_part_name(PartKind(i)));
        AssetId id = get_asset(name, catalog);
        if (!id.is_valid()) return false;
        images.partImages[i] = id;
    }
	
    return true;
}

void draw_vehicle_simulation(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partimages)
{
    draw_vehicle(context, catalog, vehicle, VehicleDrawParameters(&partimages, NullPartId, false));
}

void draw_star_system(const RenderContext& context, const AssetCatalog& catalog, GameState& game)
{
    cobot::vec2 render_size = context.render_size;
    auto& system = game.starSystem;

    ASSERT(context.camera);
    const Camera* camera = context.camera;

    float cameraData[8] = {
        camera->position.x, camera->position.y,
        camera->zoom, camera->rotation,
        camera->offset.x, camera->offset.y
    };

    // @todo
    /*
    if (!SDL_SetGPURenderState(context.renderer, context.render_states[RenderStateStar]))
    {
        return;
    }

    cobot::Quad starQuad;
    starQuad.vertices[cobot::QuadTopLeft]     = cobot::vec2(-system.star.radius,system.star.radius);
    starQuad.vertices[cobot::QuadTopRight]    = cobot::vec2(system.star.radius,system.star.radius);
    starQuad.vertices[cobot::QuadBottomLeft]  = cobot::vec2(-system.star.radius,-system.star.radius);
    starQuad.vertices[cobot::QuadBottomRight] = cobot::vec2(system.star.radius,-system.star.radius);

    log_info("%f", system.star.radius);

    cobot::vec4 uniformStarPosition = {
        0, 0, 0, system.star.radius
    };
    SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanet], 0, &uniformStarPosition, sizeof(uniformStarPosition));
    SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanet], 1, cameraData, sizeof(cameraData));

    draw_quad(context, starQuad, cobot::ColorF(0.6, 0.5, 0.1));
    */
    draw_circle(context, cobot::vec2(0,0), system.star.radius, cobot::ColorF(0.6, 0.5, 0.1));

	if (!SDL_SetGPURenderState(context.renderer, context.render_states[RenderStatePlanet]))
    {
        return;
    }

    SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanet], 1, cameraData, sizeof(cameraData));

    bool (*comparePlanetDepth)(Planet& a, Planet& b) = [](Planet& a, Planet& b) { return a.body.position.z < b.body.position.z; };
    sort_array(system.planets, comparePlanetDepth);

    for (auto& planet : system.planets)
    {
        cobot::vec4 uniformPosition = {
            planet.body.position.x, planet.body.position.y, planet.body.position.z, planet.body.radius
        };
        SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanet], 0, &uniformPosition, sizeof(uniformPosition));

        cobot::vec2 planetPos = planet.body.position.xy();
        float rad = planet.body.radius;

        cobot::Quad quad;
        quad.vertices[cobot::QuadTopLeft]     = planetPos + cobot::vec2(-rad,  rad);
        quad.vertices[cobot::QuadTopRight]    = planetPos + cobot::vec2( rad,  rad);
        quad.vertices[cobot::QuadBottomLeft]  = planetPos + cobot::vec2(-rad, -rad);
        quad.vertices[cobot::QuadBottomRight] = planetPos + cobot::vec2( rad, -rad);
        draw_quad(context, quad, planet.color);
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

void draw_planet_outline(RenderContext& context, const GameState& game, PlanetId planetid)
{
    const Planet* planet = nullptr;
    for (auto& p : game.starSystem.planets)
    {
        if (p.id == planetid)
        {
            planet = &p;
        }
    }

    draw_arc(context, planet->body.position.xy(), planet->body.radius + 5, planet->body.radius + 10, cobot::degree_to_radian_f(10), cobot::degree_to_radian_f(70), planet->color);
    draw_arc(context, planet->body.position.xy(), planet->body.radius + 5, planet->body.radius + 10, cobot::degree_to_radian_f(100), cobot::degree_to_radian_f(70), planet->color);
    draw_arc(context, planet->body.position.xy(), planet->body.radius + 5, planet->body.radius + 10, cobot::degree_to_radian_f(190), cobot::degree_to_radian_f(70), planet->color);
    draw_arc(context, planet->body.position.xy(), planet->body.radius + 5, planet->body.radius + 10, cobot::degree_to_radian_f(280), cobot::degree_to_radian_f(70), planet->color);
}

void draw_planet_map(RenderContext& render, Planet& planet)
{
    if (!SDL_SetGPURenderState(render.renderer, render.render_states[RenderStatePlanetSurface]))
    {
        return;
    }

    ASSERT(render.camera);
    const Camera* camera = render.camera;

    float cameraData[8] = {
        camera->position.x, camera->position.y,
        camera->zoom, camera->rotation,
        camera->offset.x, camera->offset.y
    };

    // @todo
    // SDL_SetGPURenderStateFragmentUniforms(context.render_states[RenderStatePlanetSurface], 0, nullptr, 0);

    SDL_SetGPURenderStateFragmentUniforms(render.render_states[RenderStatePlanetSurface], 1, cameraData, sizeof(cameraData));

    // @todo hack
    render.space = CoordinateSpace::Screen;
    draw_rectangle(render, cobot::Rectangle(render.render_size / 2, render.render_size), cobot::ColorF(0,0,1));
    render.space = CoordinateSpace::World;

    SDL_SetGPURenderState(render.renderer, nullptr);
}

bool initialize_game_ui(cobot::vec2 windowSize, AssetId fontId, AssetId editorFontId, UiState& ui, AssetCatalog& catalog, RenderContext& render)
{
    Font editor_font = catalog.get_font(editorFontId);
    Font font = catalog.get_font(fontId);
    cobot::Color button_color = cobot::Color(0x77, 0x55, 0x55);
    cobot::Color background = cobot::Color(0x33, 0x55, 0x66);

    auto mapButton = TextButton(create_text(render.renderer, String("Map"), font, button_color), windowSize * 0.05, windowSize * 0.1, background, true);
    mapButton.id = MapButton;

    AssetId buildIcon = get_asset(String("buildIcon"), catalog);
    AssetId debugIcon = get_asset(String("debugIcon"), catalog);
    AssetId runIcon = get_asset(String("runIcon"), catalog);

    if (!(buildIcon.is_valid() && debugIcon.is_valid() && runIcon.is_valid()))
    {
        return false;
    }

    TextEditor editor = TextEditor(MainEditor, cobot::Rectangle(900, 300, 500, 500), editorFontId,
                                    cobot::Color(0x22, 0x88, 0x22), cobot::Color(0x88, 0x22, 0x33), cobot::Color(0x55, 0x77, 0x44), cobot::Color(0x88, 0x33, 0x66),
                                    String("Program"), 30);
    cobot::Color icon_background(0x66, 0x11, 0x33);
    editor.icon1 = Icon(catalog.get_image(runIcon), icon_background);
    editor.icon2 = Icon(catalog.get_image(buildIcon), icon_background);
    editor.icon3 = Icon(catalog.get_image(debugIcon), icon_background);


    ui.button.add(mapButton);
    ui.editor.add(editor);

    return true;
}
