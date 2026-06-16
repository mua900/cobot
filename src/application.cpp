#include "application.hpp"
#include "log.hpp"

#include "external/open_simplex.hpp"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

static const char* org_name = "flying-carpet";
static const char* cobot_identifier = "flying-carpet.cobot";
static const char* cobot_name = "cobot";
static const char* cobot_version = "0.1.0";

bool Application::initialize()
{
    if (!SDL_SetAppMetadata(cobot_name, cobot_version, cobot_identifier))
    {
        SDL_Log(SDL_GetError());
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Failed to init SDL: %s\n", SDL_GetError());
        return false;
    }

    String_Builder path(256);

    get_base_path(path);
    if (!read_asset_catalog(path)) {
        log_error("Could not read asset catalog\n");
        return false;
    }

    {
        if (!TTF_Init())
        {
            fprintf(stderr, "Could not initialize TTF: %s\n", SDL_GetError());
            return false;
        }

        if (!MIX_Init())
        {
            fprintf(stderr, "Could not initialize MIX: %s\n", SDL_GetError());
            return false;
        }
    }

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_HIDDEN;  // show the window after the initialization is complete
        SDL_Window* window = SDL_CreateWindow("cobot", INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, flags);
        if (!window)
        {
            SDL_Log("Failed to create window with SDL: %s\n", SDL_GetError());
            return false;
        }

        // minimum aspect ratio of 1 and maximum aspect ratio of 2 default 1.6
        SDL_SetWindowAspectRatio(window, 1.0, 2.0);

        m_window = { window };

        if (!init_render())
        {
            return false;
        }

        SDL_ShowWindow(window);
    }

    {
        SDL_Cursor* normal = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
        SDL_Cursor* text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
        SDL_Cursor* resize_ew = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
        SDL_Cursor* resize_ns = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
        if (!(normal && text && resize_ew && resize_ns))
        {
            return false;
        }

        m_input.mouse.cursor.normal = normal;
        m_input.mouse.cursor.text = text;
        m_input.mouse.cursor.resize_ew = resize_ew;
        m_input.mouse.cursor.resize_ns = resize_ns;
    }

    if (!load_assets())
    {
        return false;
    }

    // game state
    {
        if (!game.load_part_images(m_catalog))
        {
            return false;
        }

        m_update_states[UpdateStateIdle] = { idleUpdate, idleFixedUpdate, 0, 0, 1 };
        m_update_states[UpdateStateVehicleSimulation] = { vehicleSimulationUpdate, vehicleSimulationFixedUpdate, 0, 0, 1 };
        m_update_states[UpdateStateSolarSystem] = { starSystemUpdate, starSystemFixedUpdate, 0, 0, 1e6 };

        game.updateState = &m_update_states[UpdateStateIdle];
        game.keyboard = keyboardIdle;
    }

    gameInfo.selectedTimescale = game.updateState->timeScale;

    {
        int num_keys = 0;
        m_input.keyboard.keys = SDL_GetKeyboardState(&num_keys);
        m_input.keyboard.num_keys = num_keys;
        m_input.keyboard.do_input = true;
    }

    initialize_libraries();

    if (!init_game_state()) {
        return false;
    }

    AssetId fontId = get_asset(String("FiraSans"), m_catalog);
    AssetId editorFontId = get_asset(String("FiraCode"), m_catalog);
    if (!(fontId.is_valid() && editorFontId.is_valid()))
    {
        return false;
    }
    m_font = fontId;
    m_editor_font = editorFontId;

    if (!init_ui()) {
        log_error("Couldn't initialize user interface.");
        return false;
    }

    quit = false;

    return true;
}

bool Application::init_game_state()
{
    game.vehicles.add(get_default_vehicle());
    game.active_vehicle = 0;
    game.starSystem = get_default_star_system(m_render.renderer);
    int s = game.scripts.add(Script(init_lua()));
    game.scripts.get_ref(s).commands.add(VehicleCommand());
    game.scripts.get_ref(s).set_program_data(0);

    return true;
}

void Application::update_game_state()
{
    game.updateState->timeScale = gameInfo.wantPause ? 0 : gameInfo.selectedTimescale;
    if (gameInfo.wantPause) {
        gameInfo.wantPause = false;
    }
}

bool Application::read_asset_catalog(String_Builder& path)
{
    const char* desc_name = "run_tree.txt";
    path.append(make_string(desc_name));
    bool parse_description = parse_assets(path.c_string(), m_catalog);

    m_catalog.load_context.render = &m_render;
    m_catalog.load_context.audio = &m_audio_player;

    return parse_description;
}

bool Application::load_assets()
{
    // the size can actually change when we are trying to load assets since folder references will expand and include arbitrary amount of files
    // so save the amount we need to iterate
    int count = m_catalog.assets.size();
    for (int i = 0; i < count; i++)
    {
        Asset& asset = m_catalog.assets[i];
        if (!(asset.flags & ASSET_IS_LAZY))
        {
            AssetId id = get_asset_at_index(i, m_catalog);
            if (!id.is_valid())
            {
                auto asset_name = m_catalog.get_asset_name_at_index(i);
                SCOPE_STRING(asset_name, name);
                if (!(asset.flags & ASSET_IS_OPTIONAL)) {
                    log_error("Couldn't load asset %s", name);
                    return false;
                }
                else {
                    log_warning("Couldn't load asset %s", name);
                }
            }
        }
    }

    return true;
}

UiState& Application::get_active_ui()
{
    switch (m_mode)
    {
        case ModeMenu: {
            switch (m_menu)
            {
                case MenuMain:
                    return m_ui[UiMainMenu];
                case MenuSettings:
                    return m_ui[UiSettings];
                case MenuLoad:
                    return m_ui[UiLoad];
                case MenuMissionEditor:
                    return m_ui[UiMissionEditor];
                default:
                    panic("Invalid menu type");
            }
        }
        case ModeGame: {
            return m_ui[UiGame];
        }
        case ModeEditor: {
            return m_ui[UiEditor];
        }
        case ModeSolarSystem: {
            return m_ui[UiSolarSystem];
        }
        default: {
            panic("Invalid game mode");
        }
    }
}

void Application::handle_events()
{
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_EVENT_QUIT:
            {
                quit = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_KeyboardEvent keyboard = e.key;
                keyboard_input_down(keyboard);
                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                SDL_KeyboardEvent keyboard = e.key;
                keyboard_input_up(keyboard);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                SDL_MouseButtonEvent mouse = e.button;
                m_input.mouse.down = true;
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);

                on_mouse_down();

                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                SDL_MouseButtonEvent mouse = e.button;

                m_input.mouse.down = false;
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);

                on_mouse_up(mouse.button);

                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
            {
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);
                on_mouse_move();
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                int render_size_x, render_size_y;
                SDL_GetRenderOutputSize(m_render.renderer, &render_size_x, &render_size_y);
                m_render.render_size = vec2(render_size_x, render_size_y);

                update_ui_state(vec2(render_size_x, render_size_y));

                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                SDL_TextInputEvent text = e.text;
                String input_text = make_string(text.text);

                UiState& ui = get_active_ui();
                Text_Field* text_field = ui.get_selected_text_field();
                if (text_field)
                {
                    Font font = m_catalog.get_font(m_editor_font);

                    text_field->append_string(input_text);
                    text_field->update_text(m_render.renderer, font, true);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    update_keyboard_state();
    game.keyboard(&game, &m_input.keyboard);
}

void Application::on_mouse_move()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = get_active_ui();

    for (auto& editor : ui.editor)
    {
        Rectangle text_area = editor.get_text_area();
        Rectangle title_area = editor.get_title_area();

        if (editor.resize.resize)
        {
            Rectangle area = editor.resize.initialArea;
            vec2 p = area.get_point_at_direction(editor.resize.direction);
            vec2 d = mouse_pos - p;
            Direction direction = editor.resize.direction;

            if (direction & DirEast)
            {
                area.x += d.x / 2;
                area.w += d.x;
            }
            else if (direction & DirWest)
            {
                area.x += d.x / 2;
                area.w -= d.x;
            }

            if (direction & DirNorth)
            {
                area.y += d.y / 2;
                area.h += d.y;
            }
            else if (direction & DirSouth)
            {
                area.y += d.y / 2;
                area.h -= d.y;
            }

            area.w = cobot::clamp(30, 2000, area.w);
            area.h = cobot::clamp(30, 2000, area.h);

            editor.field.m_area = area;
        }
        else
        {
            Direction dir = text_area.on_edge(mouse_pos, 3);
            set_text_editor_cursor(text_area, dir);
        }
    }
}

void Application::set_text_editor_cursor(Rectangle text_area, Direction dir)
{
    vec2 mouse_pos = m_input.mouse.pos;

    if (dir) {
        if (direction_is_vertical(dir))
        {
            SDL_SetCursor(m_input.mouse.cursor.resize_ns);
        }
        else if (direction_is_horizontal(dir))
        {
            SDL_SetCursor(m_input.mouse.cursor.resize_ew);
        }
    }
    else if (text_area.contains_centered(mouse_pos)) {
        SDL_SetCursor(m_input.mouse.cursor.text);
    }
    else {
        SDL_SetCursor(m_input.mouse.cursor.normal);
    }
}

bool Application::keyboard_input_up(SDL_KeyboardEvent keyboard)
{
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_DOWN: // fallthrough
        case SDL_SCANCODE_UP: {
            game.get_active_vehicle().velocity.y = 0;
            return true;
        }
    }

    return false;
}

bool Application::keyboard_input_down(SDL_KeyboardEvent keyboard)
{
    if (keyboard_input_down_common(keyboard))
    {
        return true;
    }
    else
    {
        switch (m_mode)
        {
            case ModeGame:
            {
                return keyboard_input_down_game(keyboard);
            }
            case ModeSolarSystem:
            {
                return keyboard_input_down_solar_system(keyboard);
            }
        }

        return false;
    }
}

bool Application::keyboard_input_down_game(KeyboardEvent keyboard)
{
    return false;
}

bool Application::keyboard_input_down_common(KeyboardEvent keyboard)
{
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
        {
            if (doing_text_input)
            {
                text_input_stop();
                return true;
            }

            quit = true;
            return true;
        }
        case SDL_SCANCODE_RETURN:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field)
                {
                    field->insert_line();
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_TAB:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field)
                {
                    field->insert_tab(4);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_BACKSPACE:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field)
                {
                    field->delete_at_cursor();

                    Font font = m_catalog.get_font(m_editor_font);
                    field->update_text(m_render.renderer, m_catalog.get_font(field->fontId), true);
                }
            }
            return true;
        }
        case SDL_SCANCODE_DELETE:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field)
                {
                    field->delete_after_cursor();
                    field->update_text(m_render.renderer, m_catalog.get_font(field->fontId), true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_HOME:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field) {
                    field->delete_text();
                    field->m_selection_start = 0;
                    field->m_selection_end = 0;

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_END:
        {
            if (doing_text_input)
            {
                auto field = get_active_ui().get_selected_text_field();
                if (field) {
                    field->delete_text();
                    field->m_selection_start = field->m_buffer.length;
                    field->m_selection_end = field->m_selection_start;

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_LEFT: {
            if (doing_text_input) {
                auto field = get_active_ui().get_selected_text_field();
                if (field) {
                    field->m_selection_start = MAX(0, field->m_selection_start - 1);
                    field->m_selection_end = field->m_selection_start;
                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_RIGHT: {
            if (doing_text_input) {
                auto field = get_active_ui().get_selected_text_field();
                if (field) {
                    field->m_selection_start = MIN(field->m_selection_start + 1, field->m_buffer.length);
                    field->m_selection_end = field->m_selection_start;
                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_F11:
        {
            SDL_SetWindowFullscreen(m_window.window, !is_fullscreen());
            return true;
        }
    }

    return false;
}

bool Application::keyboard_input_down_solar_system(KeyboardEvent keyboard)
{
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_SPACE:
        {
            gameInfo.wantPause = true;
            return true;
        }
    }

    return false;
}

bool Application::on_mouse_down()
{
    if (m_mode == ModeGame)
    {
        return mouse_input_game();
    }
    else if (m_mode == ModeMenu)
    {
        return mouse_input_menu();
    }
    else if (m_mode == ModeEditor)
    {
        return mouse_input_editor();
    }
    else if (m_mode == ModeSolarSystem)
    {
        return mouse_input_solar_system();
    }
    else {
        panic("Invalid application mode");
    }
}

bool Application::mouse_input_editor()
{
    return false;
}

bool Application::mouse_input_solar_system()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiSolarSystem];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (auto& button : ui.button)
        {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos))
            {
                switch (button.id)
                {
                    case BackButton:
                    {
                        switch_modes(ModeMenu);
                        switch_menu(MenuMain);
                        return true;
                    }
                    case LaunchButton:
                    {
                        if (button.info.visible)
                        {
                            switch_modes(ModeGame);
                        }
                        return true;
                    }
                }
            }
        }

        for (auto& slider : ui.discrete_slider)
        {
            Rectangle bounds = slider.get_bounds();
            if (bounds.contains_top_left(mouse_pos))
            {
                int index = 0;
                vec2 start = slider.get_start();
                vec2 step = slider.get_step();
                for (int i = 0; i < slider.element_count; i++)
                {
                    Rectangle area = Rectangle(start + i * step, slider.get_button_scale());

                    if (area.contains_centered(mouse_pos))
                    {
                        index = i;
                        break;
                    }
                }

                switch(slider.id)
                {
                    case TimeScale:
                    {
                        int ts = index + 4;
                        gameInfo.selectedTimescale = pow(10, ts);
                        break;
                    }
                    default:
                        break;
                }

                slider.selected = index;

                return true;
            }
        }

        {
            ValuePanel* panel = ui.get_value_panel(PlanetPanel);
            for (int i = 0; i < panel->tabs.size(); i++)
            {
                Rectangle area = panel->get_tab_header_area(i);
                if (area.contains_centered(mouse_pos))
                {
                    panel->activeTab = i;
                    break;
                }
            }

            ValuePanelTab& tab = panel->tabs.get_ref(panel->activeTab);
            float height = 0;
            for (int i = 0; i < tab.fields.size(); i++)
            {
                ValueField& field = tab.fields[i];
                Rectangle title_area = panel->get_field_title_area(panel->activeTab, i);
                title_area.y += height;
                height += title_area.h;

                Rectangle area = panel->get_field_area(panel->activeTab, i, &get_active_ui());
                area.y += height;
                height += area.h;

                switch (field.type)
                {
                    case ValueInteger: {
                        // fallthrough
                    }
                    case ValueNumber: {
                        // fallthrough
                    }
                    case ValueString: {
                        if (area.contains_centered(mouse_pos))
                        {
                            // @todo text field
                        }
                        break;
                    }
                    case ValueSelection: {
                        if (area.contains_centered(mouse_pos))
                        {
                            // @todo button group
                        }
                        break;
                    }
                    case ValueButton: {
                        if (title_area.contains_centered(mouse_pos))
                        {
                            switch (field.identifier)
                            {
                                case AddMission:
                                {
                                    switch_modes(ModeMenu);
                                    switch_menu(MenuMissionEditor);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }

                height += tab.field_margin;
            }
        }

        vec2 ws = get_window_size();
        bool found = false;
        for (int i = 0; i < game.starSystem.planets.size(); i++)
        {
            Planet& planet = game.starSystem.planets[i];
            Rectangle area = game.get_planet_screen_area(ws, i);
            if (area.contains_centered(mouse_pos))
            {
                gameInfo.selectedPlanet = i;
                found = true;

                String_Builder builder = {};

                ValuePanel* panel = ui.get_value_panel(PlanetPanel);
                ValuePanelTab& tab = panel->tabs.get_ref(PlanetPanelTabOrbit);
                Font font = m_catalog.get_font(ui.text_field.get_ref(tab.fields.get_ref(OrbitSemiMajorAxis).ui_element).fontId);
                Color color = ui.text_field.get_ref(tab.fields.get_ref(OrbitSemiMajorAxis).ui_element).text_color;
                builder.append_float(planet.body.parameters.semiMajorAxis);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitSemiMajorAxis).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);
                builder.clear_and_append_float(planet.body.parameters.eccentricity);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitEccentricity).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);
                builder.clear_and_append_float(planet.body.parameters.inclination);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitInclination).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);
                builder.clear_and_append_float(planet.body.parameters.argumentOfPeriapsis);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitArgumentOfPeriapsis).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);
                builder.clear_and_append_float(planet.body.parameters.longitudeOfAscendingNode);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitLongitudeOfTheAscendingNode).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);
                builder.clear_and_append_float(planet.body.parameters.trueAnomaly);
                ui.text_field.get_ref(tab.fields.get_ref(OrbitTrueAnomaly).ui_element).set_and_render_text(m_render.renderer, font, builder.to_string(), false);

                break;
            }
        }

        if (!found)
        {
            gameInfo.selectedPlanet = -1;
        }

        for (auto& control : ui.control)
        {
            if (control.visible)
            {
                bool hit = false;
                for (int i = 0; i < control.buttons.size(); i++)
                {
                    if (Rectangle(control.position + vec2(0, control.scale.y * i), control.scale).contains_centered(mouse_pos))
                    {
                        hit = true;
                        break;
                    }
                }

                if (!hit)
                {
                    control.anchorPosition = nullptr;
                    control.visible = false;
                }
            }
        }
    }
    else if (m_input.mouse.buttonFlags & MOUSE_RIGHT_MASK)
    {
        vec2 ws = get_window_size();
        for (int index = 0; index < game.starSystem.planets.size(); index += 1)
        {
            Rectangle boundingBox = game.get_planet_screen_area(ws, index);
            if (boundingBox.contains_centered(mouse_pos))
            {
                ControlMenu& control = ui.control.get_ref(index);
                control.position = boundingBox.get_position();
                control.visible = true;
                break;
            }
        }
    }

    return false;
}

bool Application::mouse_input_game()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiGame];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (int it = 0; it < ui.text_field.size(); it++)
        {
            auto& field = ui.text_field.get_ref(it);
            Rectangle area = field.m_area;
            if (area.contains_centered(mouse_pos)) {
                text_input_start();

                ui.text_input_target.index = it;
                ui.text_input_target.flags = TEXT_INPUT_TARGET_IS_VALID;

                vec2 relative = m_input.mouse.pos - area.get_top_left();
                Font font = m_catalog.get_font(field.fontId);
                field.m_selection_start = field.calculate_cursor_from_mouse(relative, field.get_string(), font, true);
                field.m_selection_end = field.m_selection_start;

                return true;
            }
        }

        for (int it = 0; it < ui.editor.size(); it++)
        {
            auto& editor = ui.editor.get_ref(it);
            auto& field = editor.field;
            Rectangle area = field.m_area;

            if (editor.drag.drag) {
                editor.drag.drag = false;
                continue;
            }

            Direction dir = area.on_edge(mouse_pos, 3);
            if (dir != DirNone)
            {
                editor.resize.resize = true;
                editor.resize.direction = dir;
                editor.resize.start = mouse_pos;
                editor.resize.initialArea = editor.get_text_area();
                return true;
            }

            if (area.contains_centered(mouse_pos))
            {
                text_input_start();

                ui.text_input_target.index = it;
                ui.text_input_target.flags = TEXT_INPUT_TARGET_IS_VALID | TEXT_INPUT_TARGET_IS_EDITOR;

                vec2 relative = m_input.mouse.pos - area.get_top_left();
                Font font = m_catalog.get_font(field.fontId);
                field.m_selection_start = field.calculate_cursor_from_mouse(relative, field.get_string(), font, true);
                field.m_selection_end = field.m_selection_start;

                return true;
            }

            if (editor.get_icon1_area().contains_centered(mouse_pos)) {
                // run
                editor.clicked_icon = 1;

                Script& script = game.scripts.get_ref(editor.user.number);

                String scriptSource = script.script.to_string();

                if (scriptSource.size == 0) {
                    // run a non existing program
                    return true;
                }

                int result = luaL_dostring(script.data.lua, script.script.c_string());

                if (result == LUA_OK) {
                    log_info("Okay program");
                }
                else {
                    log_info("Not okay program");
                    display_message(editor.get_title_area().get_position() + vec2(0, 100), vec2(100, 100), "Invalid program", 2, Color(0xAA, 0x44, 0x55), Color(0x44, 0x77, 0x55));
                }

                return true;
            }
            else if (editor.get_icon2_area().contains_centered(mouse_pos)) {
                // compile
                editor.clicked_icon = 2;

                Script& script = game.scripts.get_ref(editor.user.number);
                script.set_source(ScriptLanguage::LUA, editor.field.get_string());

                return true;
            }
            else if (editor.get_icon3_area().contains_centered(mouse_pos)) {
                // debug
                editor.clicked_icon = 3;

                // @todo

                return true;
            }

            Rectangle title_area = editor.get_title_area();
            if (title_area.contains_centered(mouse_pos))
            {
                editor.drag.drag = true;
                editor.drag.start = mouse_pos - title_area.get_top_left();
                return true;
            }
        }

        {
            ui.text_input_target = {};
            text_input_stop();
        }

        for (auto& button : ui.button) {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos)) {
                switch (button.id) {
                    case BackButton:
                    {
                        switch_modes(ModeMenu);
                        switch_menu(MenuMain);
                        break;
                    }
                }
            }
        }

        if (!game.get_active_vehicle().volume.contains_centered(mouse_pos))
        {
            PartId part = game.get_active_vehicle().getPartAt(mouse_pos);
            if (part.is_null())
            {
                for (auto& menu : ui.control) {
                    menu.visible = false;
                }
            }
        }
    }
    else if (m_input.mouse.buttonFlags & MOUSE_RIGHT_MASK)
    {
        if (game.get_active_vehicle().volume.contains_centered(mouse_pos))
        {
            PartId part = game.get_active_vehicle().getPartAt(mouse_pos);
            if (part.is_valid())
            {
                auto& menu = ui.control.get_ref(part.kind);
                menu.position = mouse_pos + menu.scale / 2;
                menu.visible = true;
            }
        }
    }

    return false;
}

bool Application::mouse_input_menu()
{
    switch (m_menu) {
        case MenuMain:          return mouse_input_main_menu();
        case MenuSettings:      return mouse_input_settings();
        case MenuLoad:          return mouse_input_load();
        case MenuMissionEditor: return mouse_input_mission_editor();
        default: panic("Invalid menu");
    }
}

bool Application::mouse_input_mission_editor()
{
    UiState& ui = m_ui[UiMissionEditor];
    vec2 mouse_pos = m_input.mouse.pos;

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (auto& button : ui.image_button)
        {
            if (Rectangle(button.position, button.scale).contains_centered(mouse_pos))
            {
                switch (button.id) {
                    case AddVehicle: {
                        switch_modes(ModeEditor);
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        for (auto& button : ui.button)
        {
            if (Rectangle(button.position, button.scale).contains_centered(mouse_pos))
            {
                switch (button.id)
                {
                    case LaunchButton:
                    {
                        if (edit_mission.is_valid())
                        {
                            int mission = game.mission.add(edit_mission);
                            load_mission(game.mission.get_ref(mission));
                            switch_modes(ModeGame);
                        }
                    }
                }
            }
        }

        for (auto& list : ui.drop_down)
        {
            Rectangle area = Rectangle(list.pos, list.scale);
            if (area.contains_centered(mouse_pos))
            {
                list.toggle();
            }
            else
            {
                if (list.open)
                {
                    int selected = -1;
                    for (int i = 0; i < list.options.size(); i++)
                    {
                        Rectangle area = list.get_option_area(i);
                        if (area.contains_centered(mouse_pos))
                        {
                            selected = i;
                            list.open = false;
                            break;
                        }
                    }

                    if (selected != -1)
                    {
                        switch (list.id)
                        {
                            case VehicleList:
                            {
                                edit_mission.vehicle = selected;
                                break;
                            }
                            case PlanetList:
                            {
                                edit_mission.planet = PlanetId(selected);
                                break;
                            }
                            default:
                                break;
                        }

                        list.selected = selected;
                    }
                }
            }
        }
    }

    return false;
}

bool Application::mouse_input_load() {
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiLoad];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (auto& button : ui.button) {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos)) {
                switch (button.id) {
                    case BackButton: {
                        switch_modes(ModeMenu);
                        switch_menu(MenuMain);
                        break;
                    }
                    case LoadButton: {
                        switch_modes(ModeSolarSystem);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    return true;
}

bool Application::mouse_input_main_menu()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiMainMenu];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (auto& button : ui.button) {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos)) {
                switch (button.id) {
                case PlayButton: {
                    switch_menu(MenuLoad);
                    break;
                }
                case SettingsButton: {
                    switch_menu(MenuSettings);
                    break;
                }
                case QuitButton: {
                    quit = true;  // quit at the end of frame
                    break;
                }
                }
            }
        }
    }

    return false;
}

bool Application::mouse_input_settings()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiSettings];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT_MASK)
    {
        for (auto& button : ui.button) {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos)) {
                switch (button.id) {
                case BackButton:
                {
                    switch_menu(MenuMain);
                    break;
                }
                }
            }
        }
    }

    return false;
}

void Application::update_keyboard_state()
{
    m_input.keyboard.keys = SDL_GetKeyboardState(&m_input.keyboard.num_keys);
    m_input.keyboard.mod_state = SDL_GetModState();
}

void Application::update()
{
    // update time
    SDL_Time time = SDL_GetTicks();
    double time_sec = (double)time / MILLISECONDS_PER_SECOND;
    m_time.deltaTime = time - m_time.time;
    m_time.deltaTimeSeconds = time_sec - m_time.timeSeconds;
    m_time.time = time;
    m_time.timeSeconds = time_sec;

    update_ui_pos();
    timeout();

    update_game_state();

    game.update(m_time);
}

void Application::timeout()
{
    for (int i = 0; i < ARRAY_SIZE(m_events); i++)
    {
        if (m_events[i].active)
        {
            if (m_events[i].event < m_time.time)
            {
                m_events[i].active = false;
            }
        }
    }

    for (int i = 0; i < messages.size(); i++) {
        if (messages[i].expire < m_time.timeSeconds) {
            messages.remove(i);
            i--;
        }
    }
}

void Application::update_ui_state(vec2 window_size) {
    for (int i = 0; i < UiCount; i++)
    {
        vec2 assumed = m_ui[i].assumed_window_size;
        float x_factor = window_size.x / assumed.x;
        float y_factor = window_size.y / assumed.y;
        if ((fabsf(x_factor - 1.0f) >= 0.1f) || (fabsf(y_factor - 1.0f) >= 0.1f)) {
            m_ui[i].update_state(window_size, m_render, m_catalog);
        }
    }
}

void Application::update_ui_pos()
{
    vec2 mouse_pos = m_input.mouse.pos;

    UiState& ui = get_active_ui();
    for (auto& editor : ui.editor)
    {
        if (editor.drag.drag)
        {
            Rectangle area = editor.get_text_area();
            vec2 half_scale = vec2(area.w / 2, area.h / 2);
            vec2 dst = (mouse_pos - editor.drag.start) + half_scale;
            dst.y += editor.title_height;
            editor.set_position(dst);
        }
    }
}

void Application::on_mouse_up(int button)
{
    UiState& ui = get_active_ui();

    if (button & MOUSE_LEFT_MASK)
    {
        // @todo maybe button interactions should be on button up

        for (auto& editor : ui.editor) {
            if (editor.drag.drag) {
                editor.drag.drag = false;
            }

            if (editor.resize.resize) {
                editor.resize = {};
            }

            editor.clicked_icon = 0;
        }
    }
}

void Application::set_event_active(int event_index, double timeout_seconds)
{
    s64 timeout = (s64)(timeout_seconds * MILLISECONDS_PER_SECOND);
    m_events[event_index].active = true;
    m_events[event_index].event = m_time.time + timeout;
}

void Application::set_event_deactive(int event_index)
{
    m_events[event_index].active = false;
}

int Application::display_message(vec2 where, vec2 scale, const char* message, float duration, Color color, Color background) {
    auto appMessage = ApplicationMessage(where, scale, message, background);
    Font font = m_catalog.get_font(m_font);
    Text text = create_text(m_render.renderer, String(message), font, color);
    appMessage.expire = m_time.timeSeconds + duration;
    appMessage.texture = text.texture;

    return messages.add(appMessage);
}

void Application::cleanup()
{
    MIX_Quit();
    SDL_Quit();
    TTF_Quit();
}

void Application::add_button(UiId ui, UiElementId id, TextButton button) {
    button.id = id;
    m_ui[ui].button.add(button);
}

void Application::add_label(UiId ui, UiElementId id, Label label) {
    label.id = id;
    m_ui[ui].label.add(label);
}

bool Application::init_render()
{
    if (!initialize_render_context(&m_render, m_window.window))
    {
        return false;
    }

    AssetId vertex_id = get_asset(String("VertexShader"), m_catalog);
    AssetId fragment_id = get_asset(String("FragmentShader"), m_catalog);
    SDL_GPUShader* vertex = m_catalog.get_shader(vertex_id);
    SDL_GPUShader* fragment = m_catalog.get_shader(fragment_id);
    if (!(vertex && fragment))
    {
        return false;
    }

    if (!init_gpu_renderer(&m_render, m_window.window, vertex, fragment)) {
        log_error("Couldn't initialize gpu renderer");
        return false;
    }

    const char* render_state_shader_name[RenderStateCount] = {
        "PlanetFrag"
    };

    for (int i = 0; i < RenderStateCount; i++)
    {
        AssetId shader = get_asset(String(render_state_shader_name[i]), m_catalog);
        if (!shader.is_valid())
        {
            return false;
        }
        SDL_GPURenderStateCreateInfo create_info = {};
        create_info.fragment_shader = m_catalog.get_shader(shader);
        SDL_GPURenderState* render_state = SDL_CreateGPURenderState(m_render.renderer, &create_info);
        if (!render_state)
        {
            SDL_Log(SDL_GetError());
            return false;
        }
        m_render.render_states.add(render_state);
    }

    return true;
}

bool Application::init_ui()
{
    vec2 ws = get_window_size();

    for (auto& ui : m_ui) { ui.assumed_window_size = ws; }

    vec2 button_scale = vec2(ws.x * 0.1, ws.y * 0.1);
    Font font = m_catalog.get_font(m_editor_font);

    Color button_color = Color(0x77, 0x55, 0x55);
    Color background = Color(0x33, 0x55, 0x66);

    // main menu
    add_button(UiMainMenu, PlayButton, TextButton(create_text(m_render.renderer, String("Play"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.2), button_scale, background, true));
    add_button(UiMainMenu, SettingsButton, TextButton(create_text(m_render.renderer, String("Settings"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.5), button_scale, background, true));
    add_button(UiMainMenu, QuitButton, TextButton(create_text(m_render.renderer, String("Quit"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.8), button_scale, background, true));

    // settings
    add_button(UiSettings, BackButton, TextButton(create_text(m_render.renderer, String("Back"), font, button_color), ws * 0.1, ws * 0.1, background, true));

    if (!init_game_ui()) return false;
    if (!init_load_ui()) return false;
    if (!init_editor_ui()) return false;
    if (!init_solar_system_ui()) return false;
    if (!init_mission_editor_ui()) return false;

    return true;
}

bool Application::init_mission_editor_ui()
{
    vec2 ws = get_window_size();
    UiState& ui = m_ui[UiMissionEditor];

    Font font = m_catalog.get_font(m_font);

    // @todo show pictures of what's selected maybe
    Drop_Down_List vehicle_list;
    Drop_Down_List planet_list;

    AssetId plusId = get_asset(String("plus"), m_catalog);
    SDL_Texture* plus = m_catalog.get_image(plusId);
    ImageButton add_vehicle (plus, vec2(ws.x * 0.2, ws.y * 0.1), vec2(ws.x * 0.05), Color(0x88, 0xAA, 0xAA));
    add_vehicle.id = AddVehicle;

    Color text_color(0x88, 0x88, 0x88);
    Color title_color(0x33, 0x22, 0x99);
    Color option_color(0x77, 0x33, 0x44);

    vec2 list_scale(ws.x * 0.1, ws.y * 0.1);
    vehicle_list.set_title(create_text(m_render.renderer, String("Vehicle"), font, text_color));
    vehicle_list.set_area(vec2(ws.x * 0.1, ws.y * 0.1), list_scale);
    vehicle_list.option_color = option_color;
    vehicle_list.title_color = title_color;
    vehicle_list.id = VehicleList;
    planet_list.set_title(create_text(m_render.renderer, String("Planet"), font, text_color));
    planet_list.set_area(vec2(ws.x * 0.3, ws.y * 0.1), list_scale);
    planet_list.option_color = option_color;
    planet_list.title_color = title_color;
    planet_list.id = PlanetList;

    for (int i = 0; i < game.vehicles.size(); i++)
    {
        Vehicle& vehicle = game.vehicles[i];
        vehicle_list.add_option(create_text(m_render.renderer, vehicle.name, font, text_color), i);
    }

    for (int i = 0; i < game.starSystem.planets.size(); i++)
    {
        Planet& planet = game.starSystem.planets[i];
        planet_list.add_option(create_text(m_render.renderer, planet.name, font, text_color), i);
    }

    TextButton launchMission = TextButton(create_text(m_render.renderer, String("Launch Mission"), font, Color(0x88, 0x44, 0x77)), vec2(ws.x * 0.85, ws.y * 0.85), vec2(ws.x * 0.1, ws.y * 0.1), Color(0x99, 0x11, 0x22));
    launchMission.id = LaunchButton;

    ui.drop_down.add(vehicle_list);
    ui.drop_down.add(planet_list);
    ui.image_button.add(add_vehicle);
    ui.button.add(launchMission);

    return true;
}

bool Application::init_solar_system_ui()
{
    vec2 ws = get_window_size();
    UiState& ui = m_ui[UiSolarSystem];

    Font font = m_catalog.get_font(m_font);
    Color button_color = Color(0x66, 0x33, 0x22);
    Color background = Color(0x44, 0x66, 0x22);
    add_button(UiSolarSystem, BackButton, TextButton(create_text(m_render.renderer, String("Main Menu"), font, button_color), ws * 0.05, ws * 0.1, background, true));

    // 1e4, 1e5, 1e6
    DiscreteSlider timescaleControl (TimeScale, vec2(ws.x / 2, 50), vec2(40, 50), 3,
        50, false, ColorF(0.3, 0.7, 0.3), ColorF(0.8, 0.3, 0.2), ColorF(0.6, 0.3, 0.2), ColorF(0.4, 0.7, 0.3), ColorF(0.7, 0.4, 0.2));

    AssetId tsIconId = get_asset(String("timescaleIcon"), m_catalog);
    if (tsIconId.is_valid())
    {
        SDL_Texture* texture = m_catalog.get_image(tsIconId);
        float w, h = {};
        SDL_GetTextureSize(texture, &w, &h);
        float aspectRatio = w / h;
        timescaleControl.texture = texture;
        timescaleControl.element_scale = 50 * vec2(aspectRatio, 1.0f);
    }

    ui.discrete_slider.add(timescaleControl);

    int planetIndex = 0;
    for (auto& planet : game.starSystem.planets)
    {
        ControlMenu menu = {};
        menu.scale = vec2(120, 80);
        menu.add_button(create_text(m_render.renderer, planet.name, font, Color(0x66, 0x33, 0x44)), planetIndex);
        ui.control.add(menu);

        planetIndex += 1;
    }

    AssetId orbitalTabId = get_asset(String("orbit"), m_catalog);
    AssetId missionTabId = get_asset(String("mission"), m_catalog);
    if (!(orbitalTabId.is_valid() && missionTabId.is_valid()))
    {
        return false;
    }
    SDL_Texture* orbitalParameterTab = m_catalog.get_image(orbitalTabId);
    SDL_Texture* missionTab = m_catalog.get_image(missionTabId);
    if (!(orbitalParameterTab && missionTab))
    {
        return false;
    }

    ValuePanel planet_panel (PlanetPanel, Rectangle(ws.x * 0.9, ws.y * 0.5, ws.x * 0.2, ws.y), 25, 50, DirWest);
    ValuePanelTab orbital_parameter_tab = {};
    ValuePanelTab missions_tab = {};
    orbital_parameter_tab.field_height = 20;
    orbital_parameter_tab.color = Color(0x44, 0x55, 0x33);
    orbital_parameter_tab.tabIcon = Icon(orbitalParameterTab, Color(0x99, 0x55, 0x33));

    missions_tab.field_height = 20;
    missions_tab.color = Color(0x88, 0x66, 0x77);
    missions_tab.tabIcon = Icon(missionTab, Color(0x88, 0x33, 0x22));

    Color valueBackground (0x77, 0x66, 0x44);
    Color valueText (0x33, 0x44, 0x88);

    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("SemiMajorAxis"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitSemiMajorAxis, ValueNumber));
    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("Eccentricity"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitEccentricity, ValueNumber));
    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("TrueAnomaly"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitTrueAnomaly, ValueNumber));
    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("LongitudeOfTheAscendingNode"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitLongitudeOfTheAscendingNode, ValueNumber));
    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("ArgumentOfPeriapsis"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitArgumentOfPeriapsis, ValueNumber));
    orbital_parameter_tab.fields.add(ValueField(create_text(m_render.renderer, String("Inclination"), font, Color(0x99, 0x66, 0x77)), ui.text_field.add(Text_Field(m_font, font.size, valueBackground, valueText)), OrbitInclination, ValueNumber));

    missions_tab.fields.add(ValueField(create_text(m_render.renderer, String("Add Mission"), font, Color(0xAA, 0xAA, 0xDD)), 0, AddMission, ValueButton));

    planet_panel.tabs.add(orbital_parameter_tab);
    planet_panel.tabs.add(missions_tab);
    ui.value_panel.add(planet_panel);

    return true;
}

bool Application::init_game_ui() {
    vec2 ws = get_window_size();
    UiState& ui = m_ui[UiGame];
    Font editor_font = m_catalog.get_font(m_editor_font);
    Font font = m_catalog.get_font(m_font);
    Color button_color = Color(0x77, 0x55, 0x55);
    Color background = Color(0x33, 0x55, 0x66);

    add_button(UiGame, BackButton, TextButton(create_text(m_render.renderer, String("Main Menu"), font, button_color), ws * 0.05, ws * 0.1, background, true));

    Color controlMenuButtonColor = Color(0x44, 0x66, 0x77);
    ControlMenu menus[PART_KIND_COUNT] = {};

    for (int i = 0; i < PART_KIND_COUNT; i++)
    {
        menus[i].scale = vec2(100, 50);
    }

    menus[PART_TIRE].add_button(create_text(m_render.renderer, String("Brake"), font, controlMenuButtonColor), 0);

    for (int i = 0; i < PART_KIND_COUNT; i++)
    {
        ui.control.add(menus[i]);
    }

    AssetId buildIcon = get_asset(String("buildIcon"), m_catalog);
    AssetId debugIcon = get_asset(String("debugIcon"), m_catalog);
    AssetId runIcon = get_asset(String("runIcon"), m_catalog);

    if (!(buildIcon.is_valid() && debugIcon.is_valid() && runIcon.is_valid()))
    {
        return false;
    }

    TextEditor editor = TextEditor(MainEditor, Rectangle(900, 300, 500, 500), m_editor_font,
                                    Color(0x22, 0x88, 0x22), Color(0x88, 0x22, 0x33), Color(0x55, 0x77, 0x44), Color(0x88, 0x33, 0x66),
                                    String("Program"), 30);
    Color icon_background(0x66, 0x11, 0x33);
    editor.icon1 = Icon(m_catalog.get_image(runIcon), icon_background);
    editor.icon2 = Icon(m_catalog.get_image(buildIcon), icon_background);
    editor.icon3 = Icon(m_catalog.get_image(debugIcon), icon_background);

    ui.editor.add(editor);

    return true;
}

bool Application::init_load_ui() {
    vec2 ws = get_window_size();
    Font font = m_catalog.get_font(m_editor_font);
    Color button_color = Color(0x77, 0x55, 0x55);
    Color background = Color(0x33, 0x55, 0x66);

    add_button(UiLoad, BackButton, TextButton(create_text(m_render.renderer, String("Back"), m_catalog.get_font(m_font), button_color), ws * 0.05, ws * 0.1, background));

    float buttonY = ws.y * 0.2;
    float buttonX = ws.x * 0.1;
    vec2 buttonScale = vec2(ws.x * 0.1, ws.y * 0.1);
    Color missionBackground = Color(0x44, 0x55, 0x55);
    Color missionTextColor = Color(0x66, 0x33, 0x77);
    TextButton testSave = TextButton(create_text(m_render.renderer, String("Test Save"), m_catalog.get_font(m_font), missionTextColor), vec2(buttonX, buttonY), buttonScale, missionBackground);
    testSave.data.number = 0;  // the id of the mission this represents
    add_button(UiLoad, LoadButton, testSave);

    return true;
}

bool Application::init_editor_ui() {
    vec2 ws = get_window_size();
    UiState& ui = m_ui[UiEditor];

    Rectangle panel_area = { 0, 0, ws.x * 0.3f, ws.y };
    Color panel_color = Color(0x33, 0x44, 0x44);

    Color iconColor = Color(0x77, 0x33, 0x44);
    Color tabIconColor = Color(0x33, 0x66, 0x44);

    AssetId tireIconId = get_asset(String("tireTabIcon"), m_catalog);
    if (!tireIconId.is_valid()) return false;
    Icon tireIcon = Icon(m_catalog.get_image(tireIconId), tabIconColor);
    DArray <IconButton> tireTabIcons;
    if (!load_tire_icons(tireTabIcons, iconColor, m_catalog)) return false;

    AssetId chasisIconId = get_asset(String("chasisTabIcon"), m_catalog);
    if (!chasisIconId.is_valid()) return false;
    Icon chasisIcon = Icon(m_catalog.get_image(chasisIconId), tabIconColor);
    DArray<IconButton> chasisTabIcons;
    if (!load_chasis_icons(chasisTabIcons, iconColor, m_catalog)) return false;

    AssetId controllerIconId = get_asset(String("controllerTabIcon"), m_catalog);
    if (!controllerIconId.is_valid()) return false;
    Icon controllerIcon = Icon(m_catalog.get_image(controllerIconId), tabIconColor);
    DArray<IconButton> controllerTabIcons;
    if (!load_controller_icons(controllerTabIcons, iconColor, m_catalog)) return false;

    return true;
}

void Application::draw()
{
    SDL_Renderer* renderer = m_render.renderer;

    if (SDL_GetWindowFlags(m_window.window) & SDL_WINDOW_MINIMIZED) {
        // don't draw anything if the window is minimized
        return;
    }

    Color edit_color = Color(0x77, 0x55, 0x66);
    Color background = doing_text_input ? edit_color : m_background_color;
    SDL_SetRenderDrawColor(renderer, COLOR_ARG(background));
    SDL_RenderClear(renderer);

    // SDL_FlushRenderer(m_render.renderer);

    SDL_SetRenderDrawBlendMode(m_render.renderer, SDL_BLENDMODE_BLEND);

    switch (m_mode)
    {
        case ModeGame: {
            draw_game();
            break;
        }
        case ModeSolarSystem: {
            draw_solar_system();
            break;
        }
        case ModeMenu: {
            break;
        }
        case ModeEditor: {
            break;
        }
    }

    SDL_SetRenderDrawBlendMode(m_render.renderer, SDL_BLENDMODE_NONE);

    draw_ui();
    draw_messages();

    SDL_RenderPresent(renderer);
}

void Application::do_gpu_frame()
{
    start_frame(m_render, m_window.window);

    m_render.start_copy_pass();

    for (auto& mesh : meshes)
    {
        m_render.add_mesh(mesh.data, mesh.ref);
    }

    m_render.end_copy_pass();

    m_render.start_render_pass();

    // m_render.draw_mesh(meshes[MeshType::Quad].ref);

    m_render.end_render_pass();
    end_frame(m_render);
}

bool Application::is_minimized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_MINIMIZED;
}

bool Application::is_maximized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_MAXIMIZED;
}

bool Application::is_fullscreen() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_FULLSCREEN;
}

vec2 Application::get_window_size() const {
    ivec2 s;
    SDL_GetWindowSize(m_window.window, &s.x, &s.y);
    return vec2(s.x, s.y);
}

void Application::draw_game()
{
    draw_vehicle_simulation(m_render, m_catalog, game);
}

void Application::draw_solar_system()
{
    draw_star_system(m_render, m_catalog, game);
    draw_orbits(m_render, m_catalog, game);
    if (gameInfo.selectedPlanet != -1)
    {
        draw_planet_outline(m_render, game, gameInfo.selectedPlanet);
    }
}

void Application::draw_ui()
{
    draw_ui_state(get_active_ui());
}

void Application::draw_messages() {
    for (auto msg : messages) {
        render_textured_rectangle(m_render, Rectangle(msg.where, msg.scale), msg.texture, msg.background, true);
    }
}

void Application::draw_ui_state(const UiState& state)
{
    for (const TextEditor& editor : state.editor)
    {
        render_text_editor(editor);
    }

    for (const Text_Field& field : state.text_field)
    {
        if (field.info.visible)
        {
            render_text_field(field);
        }
    }

    for (const Drop_Down_List& list : state.drop_down)
    {
        render_dropdown(list);
    }

    for (const TextButton& button : state.button)
    {
        if (button.info.visible)
        {
            render_textured_rectangle(m_render, Rectangle(button.position, button.scale), button.text.texture, button.background, true);
        }
    }

    for (const ImageButton& button : state.image_button)
    {
        if (button.info.visible)
        {
            render_textured_rectangle(m_render, Rectangle(button.position, button.scale), button.image, button.background, true);
        }
    }

    for (const Label& label : state.label)
    {
        render_textured_rectangle(m_render, Rectangle(label.position, label.scale), label.text.texture, label.background, false);
    }

    for (const ControlMenu& menu : state.control)
    {
        render_control_menu(menu);
    }

    for (const DiscreteSlider& slider : state.discrete_slider)
    {
        render_discrete_slider(slider);
    }

    for (const ValuePanel& panel : state.value_panel)
    {
        render_value_panel(state, panel);
    }

    for (const ButtonGroup& group : state.button_group)
    {
        if (group.info.visible)
        {
            render_button_group(group);
        }
    }
}

void Application::switch_modes(ApplicationMode mode) {
    switch (mode)
    {
        case ModeSolarSystem:
        {
            game.updateState = &m_update_states[UpdateStateSolarSystem];
            game.keyboard = keyboardStarSystem;
            break;
        }
        case ModeGame:
        {
            game.updateState = &m_update_states[UpdateStateVehicleSimulation];
            game.keyboard = keyboardVehicle;
            break;
        }
        default:
        {
            game.updateState = &m_update_states[UpdateStateIdle];
            game.keyboard = keyboardIdle;
            break;
        }
    }

    m_mode = mode;
}

void Application::switch_menu(MenuName menu) {
    m_menu = menu;
}

bool Application::load_mission(Mission& mission)
{
    if (!mission.is_valid()) return false;

    game.active_vehicle = mission.vehicle;
    game.active_planet = mission.planet;
    // @todo heightmaps etc.

    return true;
}

void Application::render_rectangle(Rectangle rect, Color color, bool center) const
{
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(m_render.renderer, &area);
}

void Application::render_rectangle_outline(Rectangle rect, Color color, bool center) const
{
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderRect(m_render.renderer, &area);
}

void Application::render_discrete_slider(const DiscreteSlider& slider) const
{
    vec2 start = slider.get_start();
    vec2 step = slider.get_step();

    Rectangle area = slider.get_bounds();
    render_rectangle_outline(area, slider.outlineColor, false);

    for (int i = 0; i < slider.element_count; i++)
    {
        Rectangle area (start + i * step, slider.element_scale);
        float t = float (i) / slider.element_count;
        ColorF color = i <= slider.selected ? mixColors(slider.startColor, slider.endColor, t) : slider.inactiveColor;

        if (slider.texture)
        {
            render_texture_with_tint(m_render, area, slider.texture, color, true);
        }
        else
        {
            render_rectangle(area, Color(color));
        }

        render_rectangle_outline(area, Color(slider.buttonColor));
    }
}

void Application::render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text) const
{
    float slider_knob_width = area.w * knob_scale.x;
    float slider_knob_height = area.h * knob_scale.y;

    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(slider_color));
    SDL_FRect slider = { area.x, area.y, area.w, area.h };
    SDL_RenderFillRect(m_render.renderer, &slider);
    float percentage = value;
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(knob_color));
    SDL_FRect slider_knob = {
        slider.x - (slider_knob_width / 2) + (slider.w * percentage), slider.y + slider.h / 2 - slider_knob_height / 2,
        slider_knob_width, slider_knob_height
    };
    SDL_RenderFillRect(m_render.renderer, &slider_knob);

    // text
    {
        const int margin = 10;
        render_text_scale(m_render.renderer, text,
            vec2(slider.x + slider.w / 2, slider.y + slider.h * 2 + margin), vec2(0.6, 0.6));
    }
}

void Application::render_value_panel(const UiState& ui, const ValuePanel& panel) const
{
    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color, true);

    float height = 0;
    for (int i = 0; i < tab.fields.size(); i++)
    {
        ValueField& value = tab.fields[i];

        Rectangle text_area = panel.get_field_title_area(panel.activeTab, i);
        text_area.y += height;
        height += text_area.h;

        render_texture(m_render, text_area, value.name.texture, true);

        Rectangle area = panel.get_field_area(panel.activeTab, i, &ui);
        area.y += height;

        switch (value.type)
        {
            case ValueInteger: {
                // fallthrough
            }
            case ValueNumber: {
                // fallthrough
            }
            case ValueString: {
                Text_Field& text_field = ui.text_field.get_ref(value.ui_element);
                int line_count = text_field.m_line_count;

                text_field.m_area = area;

                if (line_count == 0)
                {
                    render_rectangle(area, text_field.background);
                }
                else {
                    render_text_field(text_field);
                }

                height += area.h;
                break;
            }
            case ValueSelection: {
                ButtonGroup& group = ui.button_group.get_ref(value.ui_element);
                group.position = vec2(area.x, area.y);
                group.scale = vec2(area.w, area.h);
                render_button_group(group);

                height += group.scale.y;
                break;
            }
            case ValueButton: {
                render_rectangle_outline(text_area, Color(0x99, 0x55, 0x66));
                break;
            }
        }

        height += tab.field_margin;
    }

    for (int i = 0; i < panel.tabs.size(); i++) {
        Rectangle area = panel.get_tab_header_area(i);
        render_textured_rectangle(m_render, area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
    }
}

void Application::render_button_group(const ButtonGroup& group) const
{
    render_rectangle(Rectangle(group.position, group.scale), group.background);
    vec2 top_left = group.position - group.scale / 2;
    int numColumns = std::floor(group.scale.x / group.button_scale.x);
    int row = 0;
    int column = 0;
    for (auto& texture : group.buttons)
    {
        draw_texture(m_render, Rectangle(top_left + vec2(column * group.button_scale.x, row * group.button_scale.y) + group.button_scale / 2, group.button_scale), texture);
        column += 1;
        row = (column == numColumns) ? row + 1 : row;
    }
}

void Application::render_control_menu(const ControlMenu& menu) const
{
    if (menu.visible)
    {
        if (menu.anchorPosition)
        {
            draw_segment(m_render, *menu.anchorPosition, menu.position, 2, menu.background);
        }

        int index = 0;
        for (auto& button : menu.buttons)
        {
            render_textured_rectangle(m_render, Rectangle(menu.position + vec2(0, menu.scale.y * index), menu.scale), button.label.texture, menu.background, true);
            index += 1;
        }
    }
}

void Application::render_text_editor(const TextEditor& editor) const
{
    Rectangle text_area = editor.field.m_area;
    Rectangle title_area = editor.get_title_area();
    render_textured_rectangle(m_render, title_area, editor.title_texture, editor.title_bar_color);

    Rectangle area = editor.get_title_area();
    vec2 iconPos = area.get_position() + vec2(area.get_scale().x / 2, 0);
    vec2 iconScale = vec2(editor.title_height, editor.title_height);

    Color clicked_background = Color(0xAA, 0x55, 0x33);
    render_textured_rectangle(m_render, editor.get_icon1_area(), editor.icon1.texture, (editor.clicked_icon == 1) ? clicked_background : editor.icon1.background, true);
    render_textured_rectangle(m_render, editor.get_icon2_area(), editor.icon2.texture, (editor.clicked_icon == 2) ? clicked_background : editor.icon2.background, true);
    render_textured_rectangle(m_render, editor.get_icon3_area(), editor.icon3.texture, (editor.clicked_icon == 3) ? clicked_background : editor.icon3.background, true);

    render_text_field(editor.field);
}

void Application::render_text_field(const Text_Field& text_field) const
{
    Rectangle area = text_field.m_area;
    render_rectangle(area, text_field.background);

    SDL_Texture* text_texture = text_field.m_texture;

    if (text_texture)
    {
        vec2 top_left = area.get_top_left();
        vec2 text_scale = {};
        SDL_GetTextureSize(text_texture, &text_scale.x, &text_scale.y);

        int line_count = text_field.m_line_count;
        float font_size = text_field.m_font_size;

        SDL_Rect clip = {
            int(area.x - area.w / 2),
            int(area.y - area.h / 2),
            int(area.w),
            int(area.h)
        };
        SDL_SetRenderClipRect(m_render.renderer, &clip);

        draw_texture(m_render, Rectangle(top_left, text_scale), text_texture);

        SDL_SetRenderClipRect(m_render.renderer, nullptr);

        if (doing_text_input)
        {
            float cursor_width = area.w / 1000;
            render_rectangle(Rectangle(vec2(top_left.x + text_field.m_cursor_pixel_x - cursor_width / 2, top_left.y + text_field.m_cursor_pixel_y + font_size / 2), vec2(cursor_width, font_size)), TextCursorColor);
        }
    }
}

void Application::render_dropdown(const Drop_Down_List& list) const {
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(list.title_color));

    SDL_FRect header_area = {
        list.pos.x - list.scale.x/2, list.pos.y - list.scale.y / 2,
        list.scale.x, list.scale.y
    };
    SDL_RenderFillRect(m_render.renderer, &header_area);
    Text title_text = list.selected == DROP_DOWN_LIST_SELECTED_SENTINEL ? list.title : list.get_option_text(list.selected);
    render_text_size(m_render.renderer, title_text,
        vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(list.option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(m_render.renderer, &area);
            render_text_size(m_render.renderer, list.get_option_text(i),
                vec2(area.x + area.w/2, area.y + area.h/2), vec2(area.w, area.h));
        }
    }
}

Icon Application::create_icon(AssetId image, Color background) {
    SDL_Texture* texture = m_catalog.get_image(image);
    return Icon(texture, background);
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;
    m_input.keyboard.do_input = true;

    for (int i = 0; i < UiCount; i++)
    {
        m_ui[i].text_input_target = {};
    }

    m_background_color = DEFAULT_BACKGROUND_COLOR;
}

void Application::text_input_start()
{
    SDL_StartTextInput(m_window.window);
    doing_text_input = true;
    m_input.keyboard.do_input = false;

    m_background_color = {0, 0x44, 0x66, 0xff};
}

void Application::toggle_text_input()
{
    if (!doing_text_input)
    {
        text_input_start();
    }
    else
    {
        text_input_stop();
    }
}

void initialize_libraries()
{
    OpenSimplex2::initializeGradients2d();
    OpenSimplex2::initializeGradients3d();
    OpenSimplex2::initializeGradients4d();
}