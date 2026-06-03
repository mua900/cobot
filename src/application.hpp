#ifndef _APPLICATION_H
#define _APPLICATION_H

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "ui.hpp"
#include "asset.hpp"
#include "input.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "editor.hpp"
#include "time.hpp"

enum ApplicationMode {
    ModeMenu,
    ModeEditor,
    ModeSolarSystem,
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
    UiSolarSystem,
    UiGame,
    UiCount,
};

struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

// about 11 and a half days
#define EVENT_TIMEOUT_LONG 1000000.0

enum Events {
    EVENT_DUMMY,
    EVENT_COUNT,
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

enum UpdateStateId {
    Idle,
    VehicleSimulation,
    StarSystem,
    UpdateStateCount,
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

    TimeInfo m_time = {};

    Event_Timeout m_events[EVENT_COUNT] = {};

    DArray<Text> m_rendered_text = {};

    UpdateState m_update_states[UpdateStateCount];

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
    bool init_solar_system_ui();

    void do_gpu_frame();
    
    bool load_assets();

    UiState& get_active_ui();

    void timeout();
    void update_ui_state(vec2 window_size);
    void update_ui_pos();

    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    int display_message(vec2 where, vec2 scale, const char* message, float duration, Color color, Color background);

    void draw_game();
    void draw_solar_system();
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
    bool mouse_input_solar_system();

    void update_keyboard_state();
    bool keyboard_input_down(KeyboardEvent keyboard);
    bool keyboard_input_up(KeyboardEvent keyboard);

    bool gen_static_text(Color color);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();

    bool read_asset_catalog(String_Builder& path);

    void render_rectangle_outline(Rectangle rect, Color color, bool center = true) const;
    void render_rectangle(Rectangle rect, Color color, bool center = true) const;

    Icon create_icon(AssetId image, Color background);

    void render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text) const;
    void render_text_field(const Text_Field& text_field) const;
    void render_text_editor(const TextEditor& editor) const;
    void render_dropdown(const Drop_Down_List& list) const;
    void render_panel(const Panel& panel) const;
    void render_control_menu(const ControlMenu& menu) const;
    void render_discrete_slider(const DiscreteSlider& slider) const;

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

#endif // _APPLICATION_H