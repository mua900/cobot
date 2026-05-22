#ifndef _UI_H
#define _UI_H

#include <SDL3/SDL.h>

#include "common.hpp"
#include "template.hpp"
#include "math_util.hpp"
#include "asset.hpp"
#include "text.hpp"

#define INIT_WINDOW_WIDTH  1440.0f
#define INIT_WINDOW_HEIGHT 810.0f

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
};

union UserData {
    s64 number;
    void* ptr;
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

struct Button {
    UiElementId id = {};
    UserData data = {};
    Text text = {};
    vec2 position = {};
    vec2 scale = {};
    Color background = {};

    Button() {}
    Button(Text p_text, vec2 pos, vec2 sca, Color back) : text(p_text), position(pos), scale(sca), background(back) {}
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

struct Text_Field
{
    UiElementId id = {};

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

#define DROP_DOWN_LIST_SELECTED_SENTINEL -1

struct Drop_Down_List {
	// owns the text object inside it
    struct Entry {
        Text label = {};
        union {
            void* data;
            int index;
        };

        Entry() : label(), data(nullptr) {}
        Entry(Text text, void* p_data) : label(text), data(p_data) {}
        Entry(Text text, int p_index) : label(text), index(p_index) {}
    };

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

    Text get_option_label(int index) const {
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

struct PanelTab {
    Icon tabIcon = {};
    DArray<Icon> icons = {};
    Color color = {};

    PanelTab() {}
    PanelTab(Icon tab, DArray<Icon> icons, Color color) : tabIcon(tab), icons(icons), color(color) {}
};

struct Panel {
    UiElementId id = {};
    Rectangle area = {};
    DArray<PanelTab> tabs = {};

    Panel() {}
    Panel(UiElementId id, Rectangle area) : id(id), area(area) {}
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
    DArray<Button> button = {};
    DArray<Label> label = {};
    DArray<Panel> panels = {};

    TextInputTarget text_input_target = {};
    vec2 assumed_window_size = {};

    void update_state(vec2 window_size, SDL_Renderer* renderer, const AssetCatalog& catalog);

    Text_Field* get_selected_text_field();

    TextEditor* get_editor(UiElementId id);
    Text_Field* get_text_field(UiElementId id);
    Drop_Down_List* get_drop_down(UiElementId id);
    Button* get_button(UiElementId id);
    Label* get_label(UiElementId id);

    ~UiState();
};

#endif // _UI_H