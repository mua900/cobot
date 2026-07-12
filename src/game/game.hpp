#ifndef GAME_HPP
#define GAME_HPP

#include "app/text.hpp"
#include "app/time.hpp"
#include "app/input.hpp"
#include "app/ui.hpp"

#include "util/common.hpp"
#include "util/template.hpp"
#include "util/math_util.hpp"

#include "vehicle.hpp"
#include "script.hpp"
#include "map.hpp"
#include "mission.hpp"
#include "space.hpp"

struct GameState;

typedef void (*KeyboardCallback)(GameState* game, KeyboardState* keyboard);

typedef void (*UpdateFunction)(GameState* game, TimeInfo time);
typedef void (*FixedUpdateFunction)(GameState* game);

struct UpdateState {
    UpdateFunction update = nullptr;
    FixedUpdateFunction fixedUpdate = nullptr;
    s64 ticks = 0;
    double elapsed = 0;
    double timeScale = 0;
	int updateRate = 0;
	
    double calculateTimeStep() { return 1.0 / updateRate; }
};

struct GameState {
    VehicleId active_vehicle = 0;
    PlanetId active_planet = {};
    DArray<Vehicle> vehicles = {};
    DArray<Mission> mission = {};
    Map map = {};  // @todo
    StarSystem starSystem = {};

    UpdateState* updateState = nullptr;
    KeyboardCallback keyboard = nullptr;

    String_Builder builder;

    Vehicle* get_active_vehicle() const;
    void update(TimeInfo time);
};

void idleUpdate(GameState* game, TimeInfo time);
void idleFixedUpdate(GameState* game);

void vehicleSimulationUpdate(GameState* game, TimeInfo time);
void vehicleSimulationFixedUpdate(GameState* game);

void starSystemUpdate(GameState* game, TimeInfo time);
void starSystemFixedUpdate(GameState* game);

void keyboardIdle(GameState* game, KeyboardState* keyboard);
void keyboardVehicle(GameState* game, KeyboardState* keyboard);
void keyboardStarSystem(GameState* game, KeyboardState* keyboard);

void draw_vehicle_simulation(const RenderContext& context, const AssetCatalog& catalog, const Vehicle& vehicle, const VPartImages& partImages);
void draw_star_system(const RenderContext& context, const AssetCatalog& catalog, GameState& game);
void draw_planet_orbit(RenderContext& context, const Planet& planet, double centralBodyMass, float thick);
void draw_orbits(RenderContext& context, const AssetCatalog& catalog, const GameState& game);
void draw_planet_outline(RenderContext& context, const GameState& game, int planet);

bool initialize_game_ui(cobot::vec2 windowSize, AssetId fontId, AssetId editorFontId, UiState& ui, AssetCatalog& catalog, RenderContext& render);

#endif // GAME_HPP
