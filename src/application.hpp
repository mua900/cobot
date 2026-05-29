#pragma once

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "ui.hpp"
#include "asset.hpp"
#include "input.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "editor.hpp"

enum ApplicationMode {
    ModeMenu,
    ModeEditor,
    ModeGame,

    ModeCount,
};

enum MenuName {
    MenuMain,
    MenuSettings,
    MenuMissionSelect,
};

enum UiId {
    UiMainMenu,
    UiSettings,
    UiEditor,
    UiMissionSelect,
    UiGame,
    UiCount,
};

struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

#define NANOSECONDS_PER_SECOND  1'000'000'000
#define MICROSECONDS_PER_SECOND 1'000'000
#define MILLISECONDS_PER_SECOND 1'000

// about 11 and a half days
#define EVENT_TIMEOUT_LONG 1000000.0

enum Events {
    EVENT_DUMMY,
    EVENT_COUNT,
};

struct Time {
    s64 time = 0;  // miliseconds
    s64 deltaTime = 0;
    double timeSeconds = 0;
    double deltaTimeSeconds = 0;
};

struct ApplicationMessage {
    vec2 where = {};
    vec2 scale = {};
    const char* message = nullptr;
    float expire = 0;
    SDL_Texture* texture = nullptr;
    Color background = {};

    ApplicationMessage() {}
    ApplicationMessage(vec2 w, vec2 s, const char* m, Color color) : where(w), scale(s), message(m), background(color) {}
};

enum MeshType {
    Quad,
    Count,
};

class Application {
public:
    ApplicationMode m_mode = ModeMenu;
    MenuName m_menu = MenuMain;

    Window m_window = {};
    RenderContext m_render = {};
    AudioPlayer m_audio_player = {};
    Input m_input = {};
    AssetCatalog m_catalog = {};

    UiState m_ui[UiCount];
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    Time m_time = {};

    Event_Timeout m_events[EVENT_COUNT] = {};

    DArray<Text> m_rendered_text = {};

    AssetId m_font = {};
    AssetId m_editor_font = {};

    Mesh meshes[MeshType::Count] = {};

    GameState game = {};
    VehicleEditor editor = {};

    DArray<ApplicationMessage> messages = {};

    bool quit = false;
    bool doing_text_input = false;

    bool initialize();

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    void run_program();

    bool init_game_state();

    bool init_render();

    bool init_ui();
    bool init_game_ui();
    bool init_mission_ui();
    bool init_editor_ui();

    bool load_assets();

    UiState& get_active_ui();

    void timeout();
    void update_ui_state(vec2 window_size);
    void update_ui_pos();

    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    int display_message(vec2 where, vec2 scale, const char* message, float duration, Color color, Color background);

    void draw_game();
	void draw_ui();
    void draw_messages();

    void draw_ui_state(const UiState& state);

    bool on_mouse_down();
    void on_mouse_up(int button);
    void mouse_hover();

    bool mouse_input_game();
    bool mouse_input_menu();
    bool mouse_input_mission_select();
    bool mouse_input_main_menu();
    bool mouse_input_settings();
    bool mouse_input_editor();

    void update_keyboard_state();
    bool keyboard_input_down(KeyboardEvent keyboard);
    bool keyboard_input_up(KeyboardEvent keyboard);

    bool gen_static_text(Color color);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();

    bool read_asset_catalog(String_Builder& path);

    void render_rectangle(Rectangle rect, Color color, bool center = true) const;

    Icon create_icon(AssetId image, Color background);

    void render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text) const;
    void render_text_field(const Text_Field& text_field) const;
    void render_text_editor(const TextEditor& editor) const;
    void render_dropdown(const Drop_Down_List& list) const;
    void render_panel(const Panel& panel) const;
    void render_control_menu(const ControlMenu& menu) const;

    void clear_text_input_selection();

    void switch_modes(ApplicationMode mode);
    void switch_menu(MenuName menu);

    void add_button(UiId ui, UiElementId id, Button button);
    void add_label(UiId ui, UiElementId id, Label label);

    bool is_fullscreen() const;
    vec2 get_window_size() const;
};

void get_base_path(String_Builder& builder);
lua_State* init_lua();

void initialize_libraries();
