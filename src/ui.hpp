#ifndef _UI_H
#define _UI_H

#include <SDL3/SDL.h>

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"

#define INIT_WINDOW_WIDTH  1440
#define INIT_WINDOW_HEIGHT 810

#define DEFAULT_BACKGROUND_COLOR Color{ 0x88, 0x33, 0x66, 0xff }

struct Window {
    SDL_Window* window;
};

enum UiElementId {
    UiElementSentinel = 0,
    PlayButton,
    SettingsButton,
    QuitButton,
    BackButton,
    MainEditor,
    PartsPanel,
    MissionButton,
    TimeScale,
    LaunchButton,
    PlanetPanel,
};

struct UiElementInfo {
    bool visible = false;
};

struct DragInfo {
    vec2 start = vec2();
    bool drag = false;
};

struct Label {
    UiElementId id = {};
    Text text = {};
    vec2 position = {};  // center
    vec2 scale = {};
    Color background = {};

    Label() {}
    Label(Text p_text, vec2 pos, vec2 sca, Color back) : text(p_text), position(pos), scale(sca), background(back) {}
};

struct TextButton {
    UiElementId id = {};
    UserData data = {};
    Text text = {};
    vec2 position = {};
    vec2 scale = {};
    Color background = {};

    TextButton() {}
    TextButton(Text p_text, vec2 pos, vec2 sca, Color back) : text(p_text), position(pos), scale(sca), background(back) {}
};

struct ImageButton {
    UiElementId id = {};
    UserData data = {};
    SDL_Texture* image = {};
    vec2 position = {};
    vec2 scale = {};
    Color background = {};

    ImageButton() {}
    ImageButton(SDL_Texture* image, vec2 pos, vec2 sca, Color back) : image(image), position(pos), scale(sca), background(back) {}
};

struct ButtonGroup {
    UiElementId id = {};
    UiElementInfo info = {};
    UserData user = {};
    DArray<SDL_Texture*> buttons = {};
    vec2 button_scale = {};
    vec2 position = {};
    vec2 scale = {};
    Color background = {};

    ButtonGroup() {}
    ButtonGroup(UiElementId ident, vec2 pos, vec2 sca, Color back) : id(ident), position(pos), scale(sca), background(back) {}
};

struct GapBuffer {
    char* buffer = nullptr;
    int buffer_size = 0;
    int length = 0;
    int gap_index = 0;
    int end_gap = 0;

    GapBuffer() {
        initialize(256);
    }

    GapBuffer(GapBuffer& other) = delete;
    void operator=(GapBuffer& other) = delete;
    GapBuffer(GapBuffer&& other) noexcept {
        if (buffer) { std::free(buffer); }
        buffer = other.buffer;
        buffer_size = other.buffer_size;
        length = other.length;
        gap_index = other.gap_index;
        end_gap = other.end_gap;

        other.clear_values();
    }
    void operator=(GapBuffer&& other) noexcept {
        if (buffer) { std::free(buffer); }
        buffer = other.buffer;
        buffer_size = other.buffer_size;
        length = other.length;
        gap_index = other.gap_index;
        end_gap = other.end_gap;

        other.clear_values();
    }

    ~GapBuffer() {
        reset();
    }

    void initialize(int init_buffer_size);
    void append(String string, int where);
    void remove(int where, int amount);
    char get_character(int index);
    void move_gap(int position);
    void resize(int size);
    void get_string(String_Builder& sb);

    void reset();
private:
    void clear_values() {
        buffer = nullptr;
        length = 0;
        gap_index = 0;
        end_gap = 0;
        buffer_size = 0;
    }
};

enum Text_Input_Target : u8 {
    NO_TARGET,
};

constexpr Color TextCursorColor (0x33, 0x56, 0x74, 0xaa);

struct Text_Field
{
    UiElementId id = {};
    UiElementInfo info = {};

    Rectangle m_area = {};
    Color background = {};
    Color text_color = {};

    GapBuffer m_buffer = {};
    String_Builder m_text = {};
    AssetId fontId = {};
    int m_cursor_pixel_x = 0;
    int m_cursor_pixel_y = 0;
    int m_cursor_line = 0;
    int m_line_count = 0;

    // character indexes for start and end of the selection region
    int m_selection_start = 0;
    int m_selection_end = 0;

    float m_font_size = 0.0;
    SDL_Texture* m_texture = nullptr;  // cached texture the text is rendered on, updated every text input event

    Text_Field() {}

    // height -> empty height
    Text_Field(AssetId font, float height, Color background_color, Color textColor)
    {
        m_font_size = height;
        fontId = font;
        background = background_color;
        text_color = textColor;
    }

    Text_Field(Rectangle area, AssetId font, Color background_color, Color textColor)
    {
        fontId = font;
        background = background_color;
        text_color = textColor;
        m_area = area;
    }

    Text_Field(Rectangle area, AssetId font, Color background_color, Color textColor, UiElementId ident) : id(ident)
    {
        fontId = font;
        background = background_color;
        text_color = textColor;
        m_area = area;
    }

    Text_Field(Text_Field&& other) = default;
    Text_Field& operator=(Text_Field&& other) = default;

    String get_string()
    {
        m_buffer.get_string(m_text);
        return m_text.to_string();
    }

    bool set_and_render_text(SDL_Renderer* renderer, Font font, String s, bool wrapped)
    {
        set_string(s);
        return update_text(renderer, font, wrapped);
    }

    void set_string(String s)
    {
        clear();
        m_buffer.append(s, 0);
    }

    void append_string(String s)
    {
        if (m_selection_start != m_selection_end)
        {
            m_buffer.remove(m_selection_start, m_selection_end - m_selection_start);
            m_buffer.append(s, m_selection_start);
        }
        else
        {
            m_buffer.append(s, m_selection_start);
            m_selection_start += s.size;
        }

        m_selection_end = m_selection_start;
    }

    bool update_text(SDL_Renderer* renderer, Font font, bool wrapped)
    {
        return render_text_field_texture(renderer, font, text_color, wrapped);
    }

    void clear() {
        m_buffer.remove(0, m_buffer.length);
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
        m_cursor_pixel_x = 0;
        m_cursor_pixel_y = 0;
        m_cursor_line = 0;
        m_line_count = 0;
        m_selection_start = 0;
        m_selection_end = 0;
        m_font_size = 0;
    }

    void reset()
    {
        clear();
        m_buffer.reset();
        m_text.free_buffer();
    }

    void delete_text()
    {
        if (m_selection_end < m_selection_start)
            return;
        int amount = m_selection_end - m_selection_start;
        m_buffer.remove(m_selection_start, amount);

        m_selection_end = m_selection_start;
    }

    void delete_at_cursor()
    {
        if (m_selection_start != m_selection_end)
            return;
        if (m_selection_start == 0)
            return;

        m_selection_end = m_selection_start;
        m_selection_start -= 1;

        delete_text();
    }

    void delete_after_cursor()
    {
        if (m_selection_start != m_selection_end)
            return;
        if (m_selection_start == m_buffer.length)
            return;

        m_selection_end = m_selection_start + 1;

        delete_text();
    }

    void delete_at_character(int character)
    {
        if (character >= m_buffer.length)
            return;

        m_selection_start = character;
        m_selection_end = character + 1;
        delete_text();
    }

    void insert_tab(int tab_width);
    void insert_line();

    void set_text_input_area(SDL_Window* window, int line_skip)
    {
        const SDL_Rect area = { int(m_area.x), int(m_area.y) + m_cursor_line * line_skip, int(m_area.w), line_skip};
        SDL_SetTextInputArea(window, &area, m_cursor_pixel_x);
    }

    void calculate_cursor_from_selection(String string, Font font, bool wrapped);
    size_t calculate_cursor_from_mouse(vec2 mouse_position, String string, Font font, bool wrapped);

    bool render_text_field_texture(SDL_Renderer* renderer, Font font, Color color, bool wrapped);
};

struct TextEditor {
    Text_Field field = {};
    MutableString name = {};
    SDL_Texture* title_texture = nullptr;  // rendered name
    float title_height = 0;
    Color title_color = Color();  // color of the title text
    Color title_bar_color = Color();
    
    Icon icon1 = {};
    Icon icon2 = {};
    Icon icon3 = {};
    int clicked_icon = 0;

    DragInfo drag = {};
    UserData user = {};

    TextEditor() {}
    TextEditor(Rectangle area, AssetId font, Color background_color, Color textColor, Color titleColor, Color titleBarColor, String editor_name, float title_height)
        :
        field(area, font, background_color, textColor),
        name(editor_name),
        title_height(title_height),
        title_color(titleColor),
        title_bar_color(titleBarColor)
    {}
    TextEditor(UiElementId ident, Rectangle area, AssetId font, Color background_color, Color textColor, Color titleColor, Color titleBarColor, String editor_name, float title_height)
        :
        field(area, font, background_color, textColor, ident),
        name(editor_name),
        title_height(title_height),
        title_color(titleColor),
        title_bar_color(titleBarColor)
    {}

    Rectangle get_title_area() const {
        return Rectangle(field.m_area.x, field.m_area.y - (field.m_area.h + title_height) / 2, field.m_area.w, title_height);
    }

    Rectangle get_text_area() const {
        return field.m_area;
    }

    Rectangle get_icon1_area() const {
        float iconScale = title_height;
        return Rectangle(get_title_area().get_position() + vec2(get_title_area().w / 2, 0) - vec2(iconScale, 0) * 1, vec2(iconScale));
    }
    Rectangle get_icon2_area() const {
        float iconScale = title_height;
        return Rectangle(get_title_area().get_position() + vec2(get_title_area().w / 2, 0) - vec2(iconScale, 0) * 3, vec2(iconScale));
    }
    Rectangle get_icon3_area() const {
        float iconScale = title_height;
        return Rectangle(get_title_area().get_position() + vec2(get_title_area().w / 2, 0) - vec2(iconScale, 0) * 5, vec2(iconScale));
    }

    void set_position(vec2 pos) {
        field.m_area.x = pos.x;
        field.m_area.y = pos.y;
    }
};

// owns the text object inside it
struct Entry {
    Text label = {};
    union {
        void* data;
        int index;
        float number;
    };

    Entry() : label(), data(nullptr) {}
    Entry(Text text, void* p_data) : label(text), data(p_data) {}
    Entry(Text text, int p_index) : label(text), index(p_index) {}
};

#define DROP_DOWN_LIST_SELECTED_SENTINEL -1

struct Drop_Down_List {

    UiElementId id = {};

    vec2 pos = {};
    vec2 scale = {};
    int selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
    Text title = {};
    DArray<Entry> options = {};
    Color title_color = {};
    Color option_color = {};
    bool open = false;

    void toggle() {
        open = !open;
    }

    void set_area(vec2 p_pos, vec2 p_scale) {
        pos = p_pos; scale = p_scale;
    }

    void set_title(Text text) {
        title = text;
    }

    void add_option(Text text, void* data) {
        options.add(Entry(text, data));
    }

    void add_option(Text text, int index) {
        options.add(Entry(text, index));
    }

    Text get_option_text(int index) const {
        return options.get(index).label;
    }

    String get_option_name(int index) const {
        return options.get(index).label.string;
    }

    String get_selected_option_name() const {
        if (selected == DROP_DOWN_LIST_SELECTED_SENTINEL)
        {
            return String();
        }

        return get_option_name(selected);
    }

    void* get_option_data(int index) const {
        return options.get(index).data;
    }

    int get_option_data_index(int index) const {
        return options.get(index).index;
    }

    void remove_option(int index) {
        if (index == selected)
        {
            selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
        }
        options.get_ref(index).label.clear();
        options.remove_shift(index);
    }

    Drop_Down_List() {}
    Drop_Down_List(vec2 p_pos, vec2 p_scale) : pos(p_pos), scale(p_scale) {}
    ~Drop_Down_List() {
        title.clear();
        for (auto entry : options)
        {
            entry.label.clear();
        }
        options.reset();
    }
};

enum ValueType {
    ValueInteger,
    ValueNumber,
    ValueString,
    ValueSelection,
};

struct ValueField {
    Text name = {};
    int ui_element = 0;
    int identifier = 0;  // user data
    ValueType type = {};
    union {
        String string;
        u64 integer;
        double number;
        int selection = 0;
    } value = {};

    ValueField() : value{} {}
    ValueField(Text text, int ui, int ident, ValueType type) : name(text), ui_element(ui), identifier(ident), type(type), value{} {}
};

enum PlanetPanelTabs {
    PlanetPanelTabOrbit = 0,
};

struct ValuePanelTab {
    Icon tabIcon = {};
    Color color = {};
    float field_height = 0;
    DArray<ValueField> fields = {};
};

struct ValuePanel {
    UiElementId id = {};
    Rectangle area = {};
    int activeTab = 0;
    float fieldSize = 0;
    float tabHeaderSize = 0;
    Direction direction = {};
    DArray<ValuePanelTab> tabs = {};

    ValuePanel() {}
    ValuePanel(UiElementId ident, Rectangle area, float field_size, float tab_header_size, Direction dir)
        :
        id(ident),
        area(area),
        fieldSize(field_size),
        tabHeaderSize(tab_header_size),
        direction(dir)
    {}

    Rectangle get_tab_header_area(int index) const;
};

struct PanelTab {
    Icon tabIcon = {};
    DArray<IconButton> icons = {};
    Color color = {};

    PanelTab() {}
    PanelTab(Icon tab, DArray<IconButton> icons, Color color) : tabIcon(tab), icons(icons), color(color) {}
};

struct Panel {
    UiElementId id = {};
    Rectangle area = {};
    int activeTab = 0;
    float tabHeaderSize = 0;
    float iconSize = 0;
    float iconMargin = 0;
    DArray<PanelTab> tabs = {};

    Panel() {}
    Panel(UiElementId id, Rectangle area, float headerSize, float icoSize, float margin) : id(id), area(area), tabHeaderSize(headerSize), iconSize(icoSize), iconMargin(margin) {}

    Rectangle get_icon_area(int index) const;
    Rectangle get_tab_header_area(int index) const;
};

struct ControlMenu {
    DragInfo drag = {};
    vec2* anchorPosition = nullptr;
    vec2 position = {};
    vec2 scale = {};
    DArray<Entry> buttons = {};
    Color background = {};
    bool visible = false;

    void add_button(Text text, void* data) {
        buttons.add(Entry(text, data));
    }

    void add_button(Text text, int index) {
        buttons.add(Entry(text, index));
    }

    Text get_button_text(int index) const {
        return buttons.get(index).label;
    }

    String get_button_name(int index) const {
        return buttons.get(index).label.string;
    }

    void* get_button_data(int index) const {
        return buttons.get(index).data;
    }

    int get_button_data_index(int index) const {
        return buttons.get(index).index;
    }

    void remove_button(int index) {
        buttons.get_ref(index).label.clear();
        buttons.remove_shift(index);
    }
};

struct DiscreteSlider {
    UiElementId id = {};

    vec2 position = vec2();
    vec2 element_scale = {};
    int element_count = 0;
    int selected = 0;
    float element_gap = 0;
    bool vertical = false;
    Texture* texture = nullptr;
    ColorF outlineColor = {};
    ColorF buttonColor = {};
    ColorF inactiveColor = {};
    ColorF startColor = {};
    ColorF endColor = {};

    DiscreteSlider() {}
    DiscreteSlider(UiElementId ident, vec2 pos, vec2 elem_scale, int elem_count, float elem_gap, bool vert, ColorF outline_color, ColorF button_color, ColorF inactive_color, ColorF start_color, ColorF end_color)
        :
        id(ident),
        position(pos),
        element_scale(elem_scale),
        element_count(elem_count),
        element_gap(elem_gap),
        vertical(vert),
        outlineColor(outline_color),
        buttonColor(button_color),
        inactiveColor(inactive_color),
        startColor(start_color),
        endColor(end_color)
    {}

    Rectangle get_bounds() const;
    vec2 get_start() const;
    vec2 get_step() const;
    vec2 get_button_scale() const;
};

#define TEXT_INPUT_TARGET_IS_VALID     BIT(0)
#define TEXT_INPUT_TARGET_IS_EDITOR    BIT(1)

struct TextInputTarget {
    u16 index = 0;
    u16 flags = 0;
};

struct UiState {
    DArray<TextEditor> editor = {};
    DArray<Text_Field> text_field = {};
    DArray<Drop_Down_List> drop_down = {};
    DArray<TextButton> button = {};
    DArray<ImageButton> image_button = {};
    DArray<Label> label = {};
    DArray<Panel> panels = {};
    DArray<ValuePanel> value_panel = {};
    DArray<ControlMenu> control = {};
    DArray<DiscreteSlider> discrete_slider = {};
    DArray<ButtonGroup> button_group = {};

    TextInputTarget text_input_target = {};
    vec2 assumed_window_size = {};

    void update_state(vec2 window_size, SDL_Renderer* renderer, const AssetCatalog& catalog);

    Text_Field* get_selected_text_field();

    TextEditor* get_editor(UiElementId id);
    Text_Field* get_text_field(UiElementId id);
    Drop_Down_List* get_drop_down(UiElementId id);
    TextButton* get_button(UiElementId id);
    Label* get_label(UiElementId id);
    ValuePanel* get_value_panel(UiElementId id);

    ~UiState();
};

#endif // _UI_H