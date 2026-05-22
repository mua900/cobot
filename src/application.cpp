#include "application.hpp"
#include "log.hpp"

#include "external/open_simplex.hpp"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

bool Application::initialize()
{
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

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_HIDDEN;  // show the window after the initialization is complete
        SDL_Window* window = SDL_CreateWindow("game", INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, flags);
        if (!window)
        {
            SDL_Log("Failed to create window with SDL: %s\n", SDL_GetError());
            return false;
        }

        // minimum aspect ratio of 1 and maximum aspect ratio of 2 default 1.6
        SDL_SetWindowAspectRatio(window, 1.0, 2.0);

        SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer)
        {
            SDL_Log("Failed to create renderer with SDL: %s\n", SDL_GetError());
            return false;
        }

        SDL_ShowWindow(window);

        m_window = { window };

        int render_size_x, render_size_y;
        SDL_GetRenderOutputSize(renderer, &render_size_x, &render_size_y);
        m_render = { vec2(render_size_x, render_size_y), renderer };
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

    if (!load_assets())
    {
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

    // scripts
    {
        game.scripts.add(Script());
    }

    initialize_libraries();

    if (!init_game_state()) {
        return false;
    }

    quit = false;

    return true;
}

bool Application::init_game_state()
{
    if (!generate_map(m_render.renderer, &game.map, 32, 640, 640)) {
        return false;
    }
    game.vehicle = get_default_vehicle();

    return true;
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
                case MenuMissionSelect:
                    return m_ui[UiMissionSelect];
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

                keyboard_input(keyboard);

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
}

bool Application::keyboard_input(SDL_KeyboardEvent keyboard)
{
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
        {
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
            }

            return true;
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
            }

            return true;
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
            }
            return true;
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
            }

            return true;
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
            }

            return true;
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
            }

            return true;
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
            }

            return true;
        }
        case SDL_SCANCODE_F11:
        {
            SDL_SetWindowFullscreen(m_window.window, !is_fullscreen());
            return true;
        }
    }

    return false;
}

bool Application::on_mouse_down()
{
    vec2 mouse_pos = m_input.mouse.pos;
    vec2 render_size = m_render.render_size;

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
    else {
        panic("Invalid application mode");
    }
}

bool Application::mouse_input_editor()
{
    UiState& ui = m_ui[UiEditor];
    vec2 mouse_pos = m_input.mouse.pos;

    if (m_input.mouse.buttonFlags & MOUSE_LEFT) {
        for (Panel& panel : ui.panels) {
            if (panel.area.contains_top_left(mouse_pos))
            {
                PanelTab& tab = panel.tabs.get_ref(panel.activeTab);
                for (int i = 0; i < tab.icons.size(); i++) {
                    Rectangle area = panel.get_icon_area(i);
                    if (area.contains_centered(mouse_pos)) {
                        
                    }
                }

                return true;
            }

            for (int i = 0; i < panel.tabs.size(); i++) {
                if (panel.get_tab_header_area(i).contains_centered(mouse_pos)) {
                    panel.activeTab = i;
                    return true;
                }
            }
        }
    }

    return false;
}

bool Application::mouse_input_game()
{
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiGame];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT)
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

        for (int it = 0; it < ui.editor.size(); it++) {
            auto& editor = ui.editor.get_ref(it);
            auto& field = editor.field;
            Rectangle area = field.m_area;

            if (editor.drag.drag) {
                editor.drag.drag = false;
                continue;
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

                int result = luaL_dostring(script.lua, script.script.c_string());

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
    }

    return false;
}

bool Application::mouse_input_menu()
{
    switch (m_menu) {
        case MenuMain:          return mouse_input_main_menu();
        case MenuSettings:      return mouse_input_settings();
        case MenuMissionSelect: return mouse_input_mission_select();
        default: panic("Invalid menu");
    }
}

bool Application::mouse_input_mission_select() {
    vec2 mouse_pos = m_input.mouse.pos;
    UiState& ui = m_ui[UiMissionSelect];

    if (m_input.mouse.buttonFlags & MOUSE_LEFT)
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
                    case MissionButton: {
                        switch_modes(ModeGame);
                        break;
                    }
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

    if (m_input.mouse.buttonFlags & MOUSE_LEFT)
    {
        for (auto& button : ui.button) {
            Rectangle area = Rectangle(button.position, button.scale);
            if (area.contains_centered(mouse_pos)) {
                switch (button.id) {
                case PlayButton: {
                    switch_menu(MenuMissionSelect);
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

    if (m_input.mouse.buttonFlags & MOUSE_LEFT)
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

void Application::mouse_hover() {

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
            m_ui[i].update_state(window_size, m_render.renderer, m_catalog);
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
    if (button == MOUSE_LEFT)
    {
        // @todo maybe button interactions should take action on button up

        UiState& ui = get_active_ui();
        for (auto& editor : ui.editor) {
            if (editor.drag.drag) {
                editor.drag.drag = false;
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

void Application::add_button(UiId ui, UiElementId id, Button button) {
    button.id = id;
    m_ui[ui].button.add(button);
}

void Application::add_label(UiId ui, UiElementId id, Label label) {
    label.id = id;
    m_ui[ui].label.add(label);
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
    add_button(UiMainMenu, PlayButton, Button(create_text(m_render.renderer, String("Play"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.2), button_scale, background));
    add_button(UiMainMenu, SettingsButton, Button(create_text(m_render.renderer, String("Settings"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.5), button_scale, background));
    add_button(UiMainMenu, QuitButton, Button(create_text(m_render.renderer, String("Quit"), font, button_color), vec2(ws.x * 0.5, ws.y * 0.8), button_scale, background));

    // settings
    add_button(UiSettings, BackButton, Button(create_text(m_render.renderer, String("Back"), font, button_color), ws * 0.1, ws * 0.1, background));

    if (!init_game_ui()) return false;
    if (!init_mission_ui()) return false;
    if (!init_editor_ui()) return false;

    return true;
}

bool Application::init_game_ui() {
    vec2 ws = get_window_size();
    Font font = m_catalog.get_font(m_editor_font);
    Color button_color = Color(0x77, 0x55, 0x55);
    Color background = Color(0x33, 0x55, 0x66);

    add_button(UiGame, BackButton, Button(create_text(m_render.renderer, String("Main Menu"), font, button_color), ws * 0.05, ws * 0.1, background));

    return true;
}

bool Application::init_mission_ui() {
    vec2 ws = get_window_size();
    Font font = m_catalog.get_font(m_editor_font);
    Color button_color = Color(0x77, 0x55, 0x55);
    Color background = Color(0x33, 0x55, 0x66);
    
    add_button(UiMissionSelect, BackButton, Button(create_text(m_render.renderer, String("Back"), m_catalog.get_font(m_font), button_color), ws * 0.05, ws * 0.1, background));
    
    // @todo layouts would be useful here
    float buttonY = ws.y * 0.2;
    float buttonX = ws.x * 0.1;
    vec2 buttonScale = vec2(ws.x * 0.1, ws.y * 0.1);
    Color missionBackground = Color(0x44, 0x55, 0x55);
    Color missionTextColor = Color(0x66, 0x33, 0x77);
    Button testMission = Button(create_text(m_render.renderer, String("Test Mission"), m_catalog.get_font(m_font), missionTextColor), vec2(buttonX, buttonY), buttonScale, missionBackground);
    testMission.data.number = 0;  // the id of the mission this represents
    add_button(UiMissionSelect, MissionButton, testMission);

    return true;
}

bool Application::init_editor_ui() {
    vec2 ws = get_window_size();
    UiState& ui = m_ui[UiEditor];

    Rectangle panel_area = { 0, 0, ws.x * 0.3f, ws.y };
    Color panel_color = Color(0x33, 0x44, 0x44);
    Panel partsPanel (PartsPanel, panel_area, 32, 32, 16);

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

    partsPanel.tabs.add(PanelTab(tireIcon, tireTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(chasisIcon, chasisTabIcons, panel_color));
    partsPanel.tabs.add(PanelTab(controllerIcon, controllerTabIcons, panel_color));

    ui.panels.add(partsPanel);

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

    switch (m_mode)
    {
        case ModeGame: {
            draw_game();
            break;
        }
        case ModeMenu: {
            break;
        }
        case ModeEditor: {
            break;
        }
    }

    draw_ui();

    draw_messages();

    SDL_RenderPresent(renderer);
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
    vec2 ws = get_window_size();
    if (game.map.texture) {
        SDL_FRect dst = { 0, 0, ws.x, ws.y };
        SDL_RenderTexture(m_render.renderer, game.map.texture, nullptr, &dst);
    }
}

void Application::draw_ui()
{
    draw_ui_state(get_active_ui());
}

void Application::draw_messages() {
    for (auto msg : messages) {
        render_textured_rectangle(Rectangle(msg.where, msg.scale), msg.texture, msg.background, true);
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
        render_text_field(field);
    }

    for (const Drop_Down_List& list : state.drop_down)
    {
        render_dropdown(list);
    }

    for (const Button& button : state.button)
    {
        render_textured_rectangle(Rectangle(button.position, button.scale), button.text.texture, button.background, true);
    }

    for (const Label& label : state.label)
    {
        render_textured_rectangle(Rectangle(label.position, label.scale), label.text.texture, label.background, false);
    }

    for (const Panel& panel : state.panels)
    {
        render_panel(panel);
    }
}

void Application::switch_modes(ApplicationMode mode) {
    m_mode = mode;
}

void Application::switch_menu(MenuName menu) {
    m_menu = menu;
}

void Application::render_rectangle(Rectangle rect, Color color, bool center) const
{
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(m_render.renderer, &area);
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

void Application::render_panel(const Panel& panel) const
{
    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color, false);

    const float margin = 16;
    const float iconSize = 32;
    for (int i = 0; i < tab.icons.size(); i++) {
        Rectangle area = panel.get_icon_area(i);
        render_textured_rectangle(area, tab.icons.get(i).icon.texture, tab.icons.get(i).icon.background, true, false);
    }

    for (int i = 0; i < panel.tabs.size(); i++) {
        Rectangle area = panel.get_tab_header_area(i);
        render_textured_rectangle(area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
    }
}

void Application::render_text_editor(const TextEditor& editor) const
{
    Rectangle text_area = editor.field.m_area;
    Rectangle title_area = editor.get_title_area();
    render_textured_rectangle(title_area, editor.title_texture, editor.title_bar_color);

    Rectangle area = editor.get_title_area();
    vec2 iconPos = area.get_position() + vec2(area.get_scale().x / 2, 0);
    vec2 iconScale = vec2(editor.title_height, editor.title_height);
    
    Color clicked_background = Color(0xAA, 0x55, 0x33);
    render_textured_rectangle(editor.get_icon1_area(), editor.icon1.texture, (editor.clicked_icon == 1) ? clicked_background : editor.icon1.background, true);
    render_textured_rectangle(editor.get_icon2_area(), editor.icon2.texture, (editor.clicked_icon == 2) ? clicked_background : editor.icon2.background, true);
    render_textured_rectangle(editor.get_icon3_area(), editor.icon3.texture, (editor.clicked_icon == 3) ? clicked_background : editor.icon3.background, true);

    render_text_field(editor.field);
}

void Application::render_text_field(const Text_Field& text_field) const
{
    SDL_FRect tf_area = { text_field.m_area.x - text_field.m_area.w / 2, text_field.m_area.y - text_field.m_area.h / 2, text_field.m_area.w, text_field.m_area.h };
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(text_field.background));
    SDL_RenderFillRect(m_render.renderer, &tf_area);

    SDL_Texture* text_texture = text_field.m_texture;
    float texture_width;
    float texture_height;
    SDL_GetTextureSize(text_texture, &texture_width, &texture_height);

    if (text_texture)
    {
        int line_count = text_field.m_line_count;
        float font_size = text_field.m_font_size;

        SDL_FRect string_area = { tf_area.x, tf_area.y, texture_width, texture_height };
        SDL_FRect texture_area = { 0, 0, texture_width, texture_height };
        SDL_RenderTexture(m_render.renderer, text_texture, &texture_area, &string_area);

        if (doing_text_input) {
            const u8 cursorAlpha = 0xaa;
            SDL_SetRenderDrawColor(m_render.renderer, 0x33, 0x56, 0x74, cursorAlpha);

            float cursor_width = tf_area.w / 1000;
            SDL_FRect cursor = SDL_FRect{ tf_area.x + text_field.m_cursor_pixel_x - cursor_width / 2,
                                            tf_area.y + text_field.m_cursor_pixel_y,
                                            cursor_width,
                                            font_size };

            SDL_RenderFillRect(m_render.renderer, &cursor);
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
    render_text_size(m_render.renderer, list.title,
        vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(list.option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(m_render.renderer, &area);
            render_text_size(m_render.renderer, list.get_option_label(i),
                vec2(area.x + area.w/2, area.y + area.h/2), vec2(area.w, area.h));
        }
    }
}

Icon Application::create_icon(AssetId image, Color background) {
    SDL_Texture* texture = m_catalog.get_image(image);
    return Icon(texture, background);
}

void Application::render_texture(Rectangle area, Texture* texture, bool strech)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h };
    float width = strech ? area.w : tex_w;
    float height = strech ? area.h : tex_h;
    SDL_FRect dst = { area.x, area.y, width, height };
    SDL_RenderTexture(m_render.renderer, texture, &src, &dst);
}

void Application::render_textured_rectangle(Rectangle rect, SDL_Texture* texture, Color color, bool strech, bool center) const {
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
        SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
        SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(m_render.renderer, &area);

    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = { 0, 0, tex_w, tex_h};
    float width = strech ? area.w : tex_w;
    float height = strech ? area.h : tex_h;
    SDL_FRect dst = { area.x, area.y, width, height };
    SDL_RenderTexture(m_render.renderer, texture, &src, &dst);
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;

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

void Application::run_program() {
    Script script = game.scripts.get(0);
    
}

bool Script::set_source(ScriptLanguage language, String source) {
    if (language == ScriptLanguage::LANGUAGE) {
        Interp* interp = interp_create();
        if (!interp) return false;
        if (!interp_set_program(interp, source.data, source.size)) return false;

        script.clear_and_append(source);

        return true;
    }
    else if (language == ScriptLanguage::LUA) {
        String_Builder buffer = {};
        buffer.append(source);
        lua_State* state = init_lua();


        if (lua) {
            lua_close(lua);
        }
        
        lua = state;

        script.clear_and_append(source);
        return true;
    }
    else {
        ASSERT(false);
        return false;
    }
}

lua_State* init_lua()
{
    lua_State* state = luaL_newstate();
    if (!state) return nullptr;

    // @todo register the functions we want etc.

    return state;
}

void initialize_libraries()
{
    OpenSimplex2::initializeGradients2d();
    OpenSimplex2::initializeGradients3d();
    OpenSimplex2::initializeGradients4d();
}