#include "ui.hpp"
#include "log.hpp"

void GapBuffer::initialize(int init_buffer_size) {
    buffer = (char*) std::malloc(init_buffer_size);
    buffer_size = init_buffer_size;
    gap_index = 0;
    end_gap = init_buffer_size;
    length = 0;
}

void GapBuffer::reset() {
    std::free(buffer);
    buffer = nullptr;
    buffer_size = 0;
    length = 0;
    gap_index = 0;
    end_gap = 0;
}

void GapBuffer::append(String string, int where)
{
    if (!(where <= length && where >= 0)) {
        return;
    }

    if (length + string.size > buffer_size) {
        resize(buffer_size * 2);
    }

    if (where != gap_index)
    {
        move_gap(where);
    }

    for (int i = 0; i < string.size; i++)
    {
        buffer[gap_index + i] = string.data[i];
    }

    length += string.size;
    gap_index += string.size;
}

void GapBuffer::remove(int where, int amount)
{
    if (where + amount > length)
    {
        amount = length - where;
    }

    if (!(where < length && where >= 0))
        return;

    move_gap(where);
    end_gap += amount;
    length -= amount;
}

char GapBuffer::get_character(int index)
{
    if (index < gap_index)
    {
        return buffer[index];
    }
    else
    {
        return buffer[end_gap + index - gap_index];
    }
}

void GapBuffer::move_gap(int position)
{
    if (!(position <= length && position >= 0)) {
        return;
    }

    int start = 0;
    int dest = 0;
    int amount = 0;
    if (position < gap_index) {
        amount = gap_index - position;
        start = position;
        dest = end_gap - amount;
    }
    else {
        amount = position - gap_index;
        start = end_gap;
        dest = gap_index;
    }

    std::memmove(buffer + dest, buffer + start, amount);

    int gap_size = end_gap - gap_index;
    gap_index = position;
    end_gap = gap_index + gap_size;
}

void GapBuffer::resize(int size)
{
    if (size < length) {
        return;  // failure
    }

    int start_chars = gap_index;
    int end_chars = buffer_size - end_gap;

    char* nbuffer = (char*) std::malloc(size);
    ASSERT(nbuffer);
    std::memcpy(nbuffer, buffer, start_chars);
    std::memcpy(nbuffer + (size - end_chars), buffer + end_gap, end_chars);
    std::free(buffer);

    buffer = nbuffer;
    end_gap = size - end_chars;
    buffer_size = size;
}

void GapBuffer::get_string(String_Builder& sb)
{
	sb.clear_and_append(String(buffer, gap_index));
	sb.append(String(buffer + end_gap, length - gap_index));
}

void Text_Field::calculate_cursor_from_selection(String string, Font font, bool wrapped)
{
    int line_skip = TTF_GetFontLineSkip(font.font);

    // calculate cursor position
    int cursor_line = 0;
    int cursor_pixel_x = 0;
    size_t cursor_byte = 0;

    Rectangle area = m_area;

    if (wrapped)
    {
        while (cursor_byte < m_selection_start)
        {
            size_t to_next_newline = 0;
            size_t cursor_character_this_line = 0;

            // measure distance to linebreak
            while (cursor_byte + to_next_newline < m_selection_start && string.data[cursor_byte + to_next_newline] != '\n') {
                to_next_newline += 1;
            }

            if (to_next_newline == 0) {
                // a line that is just a newline character
                cursor_byte += 1;
                cursor_line += 1;
            }

            if (string.data[cursor_byte + to_next_newline] == '\n') {
                to_next_newline += 1;  // skip the newline character as well. It is not rendered and don't leave the cursor on it because then it will cause the loop to break
            }

            // measure distance to end of text render area
            TTF_MeasureString(font.font, string.data + cursor_byte, m_selection_start - cursor_byte, area.w, &cursor_pixel_x, &cursor_character_this_line);

            // take the minimum
            cursor_character_this_line = MIN(cursor_character_this_line, to_next_newline);

            if (cursor_character_this_line == 0)
            {
                break;
            }

            cursor_byte += cursor_character_this_line;

            cursor_line += 1;
        }

        if (cursor_line)
        {
            cursor_line -= 1;  // 0 based indexing instead of 1 based indexing
        }

        int cursor_pixel_y = cursor_line * line_skip;

        m_cursor_line = cursor_line;
        m_cursor_pixel_x = cursor_pixel_x;
        m_cursor_pixel_y = cursor_pixel_y;
    }
    else {
        TTF_MeasureString(font.font, string.data, m_selection_start, MAX_INTEGER, &cursor_pixel_x, nullptr);

        m_cursor_line = 0;
        m_cursor_pixel_x = cursor_pixel_x;
        m_cursor_pixel_y = 0;
    }
}

size_t Text_Field::calculate_cursor_from_mouse(vec2 position, String string, Font font, bool wrapped)
{
    int line_skip = TTF_GetFontLineSkip(font.font);
    Rectangle area = m_area;
    int line_count = m_line_count;

    int cursor_line = position.y / line_skip;

    if (cursor_line >= line_count)
    {
        m_cursor_line = m_line_count - 1;
        return m_buffer.length;
    }

    size_t cursor_character = 0;
    int pixel_x = 0;
    int pixel_y = cursor_line * line_skip;

    if (wrapped)
    {
        // calculate what the lines above us add up to in character count
        for (int i = 0; i < cursor_line; i++)
        {
            size_t cursor_character_this_line = 0;

            TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, area.w, nullptr, &cursor_character_this_line);

            cursor_character += cursor_character_this_line;
        }
    }

    size_t last_line_character = 0;
    TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, position.x, &pixel_x, &last_line_character);

    if (cursor_character + last_line_character < string.size)
    {
        int next_pixel_x = 0;

        TTF_MeasureString(font.font, string.data + cursor_character, last_line_character + 1, area.w, &next_pixel_x, NULL);

        if (position.x > (pixel_x + next_pixel_x) / 2)
        {
            last_line_character += 1;
            pixel_x = next_pixel_x;
        }
    }

    cursor_character += last_line_character;

    m_cursor_line = cursor_line;
    m_cursor_pixel_x = pixel_x;
    m_cursor_pixel_y = pixel_y;

    return cursor_character;
}

bool Text_Field::render_text_field_texture(SDL_Renderer* renderer, Font font, Color color, bool wrapped)
{
    SDL_DestroyTexture(m_texture);  // old texture
    m_texture = nullptr;

    String str = get_string();
    if (str.size == 0)
    {
        return true;
    }

    const SDL_Color text_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* text_surface;

    if (wrapped) {
        text_surface = TTF_RenderText_Solid_Wrapped(font.font, str.data, str.size, text_color, m_area.w);
    } else {
        text_surface = TTF_RenderText_Solid(font.font, str.data, str.size, text_color);
    }

    if (!text_surface)
    {
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_DestroySurface(text_surface);

    if (!texture)
    {
        return false;
    }

    float texture_width, texture_height;
    SDL_GetTextureSize(texture, &texture_width, &texture_height);
    int line_skip = TTF_GetFontLineSkip(font.font);
    int line_count = (wrapped) ? MAX(1, (int)(texture_height / line_skip)) : (1);

    calculate_cursor_from_selection(str, font, wrapped);

    m_line_count = line_count;
    m_texture = texture;
    m_font_size = font.size;

    return true;
}

void Text_Field::insert_tab(int tab_width)
{
    if (tab_width > 16)
    {
        tab_width = 16;
        log_warning("Tab width set to larger than 16");
    }

    char buffer[16] = {};
    for (int i = 0; i < tab_width; i++)
    {
        buffer[i] = ' ';
    }

    delete_text();
    m_buffer.append(String(buffer, tab_width), m_selection_start);

    m_selection_start += 1;
    m_selection_end = m_selection_start;
}

void Text_Field::insert_line()
{
    delete_text();
    m_buffer.append(String("\n"), m_selection_start);

    m_selection_start += 1;
    m_selection_end = m_selection_start;
}

void UiState::update_state(vec2 window_size, SDL_Renderer* renderer, const AssetCatalog& catalog) {
    float y_factor = window_size.y / assumed_window_size.y;
    float x_factor = window_size.x / assumed_window_size.x;

    for (auto& ed : editor) {
        Font font = catalog.get_font(ed.field.fontId);

        ed.title_height *= y_factor;
        if (ed.title_texture) {
            SDL_DestroyTexture(ed.title_texture);
            ed.title_texture = render_text(renderer, ed.name.to_string(), font, ed.title_color);
        }

        ed.field.m_area.x *= x_factor;
        ed.field.m_area.y *= y_factor;
        ed.field.m_area.w *= x_factor;
        ed.field.m_area.h *= y_factor;
        ed.field.render_text_field_texture(renderer, font, ed.field.text_color, true);
    }

    for (auto& field : text_field) {
        Font font = catalog.get_font(field.fontId);

        field.m_area.x *= x_factor;
        field.m_area.y *= y_factor;
        field.m_area.w *= x_factor;
        field.m_area.h *= y_factor;
        field.render_text_field_texture(renderer, font, field.text_color, true);
    }

    for (auto& drop : drop_down) {
        drop.pos.x *= x_factor;
        drop.pos.y *= y_factor;
        drop.scale.x *= x_factor;
        drop.scale.y *= y_factor;
    }

    for (auto& but : button) {
        but.position.x *= x_factor;
        but.position.y *= y_factor;
        but.scale.x *= x_factor;
        but.scale.y *= y_factor;
    }

    for (auto& lbl : label) {
        lbl.position.x *= x_factor;
        lbl.position.y *= y_factor;
        lbl.scale.x *= x_factor;
        lbl.scale.y *= y_factor;
    }

    assumed_window_size = window_size;
}

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size)
{
    path.append(font_folder);
    path.append(PathSeparator);

    path.append(font_file);

    TTF_Font* ttf_font = TTF_OpenFont(path.c_string(), size);

    bool success = load_font_file(font, path.c_string(), size);

    path.remove(font_folder.size + 1 + font_file.size);

    return success;
}

bool load_font_file(Font* font, const char* path, float size)
{
    TTF_Font* ttf_font = TTF_OpenFont(path, size);
    if (!ttf_font)
    {
        fprintf(stderr, "Could not load font %s\n", path);
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }

    font->font = ttf_font;
    font->size = size;

    return true;
}

UiState::~UiState()
{
    drop_down.reset();
    button.reset();

    for (auto& l : label)
    {
        l.text.clear();
    }
}

Text_Field* UiState::get_selected_text_field()
{
    if (!(text_input_target.flags & TEXT_INPUT_TARGET_IS_VALID)) {
        return nullptr;
    }
    else if (text_input_target.flags & TEXT_INPUT_TARGET_IS_EDITOR) {
        return &editor.get_ref(text_input_target.index).field;
    }
    else {
        return text_field.get_ptr(text_input_target.index);
    }
}

TextEditor* UiState::get_editor(UiElementId id)
{
    for (auto& element : editor) {
        if (element.field.id == id) {
            return &element;
        }
    }

    return nullptr;
}

Text_Field* UiState::get_text_field(UiElementId id) {
    for (auto& element : text_field)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Drop_Down_List* UiState::get_drop_down(UiElementId id) {
    for (auto& element : drop_down)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Button* UiState::get_button(UiElementId id) {
    for (auto& element : button)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Label* UiState::get_label(UiElementId id) {
    for (auto& element : label)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Rectangle Panel::get_icon_area(int index) const {
    float length_per_icon = iconSize + iconMargin;
    int rowCount = area.h / length_per_icon;
    int row = index % rowCount;
    int column = index / rowCount;
    return Rectangle(area.get_position() + vec2(column, row) * length_per_icon,
                     vec2(iconSize));
}

Rectangle Panel::get_tab_header_area(int index) const {
    int rowCount = area.h / (tabHeaderSize * 2);
    int row = index % rowCount;
    int column = index / rowCount;
    return Rectangle(area.x + area.w + tabHeaderSize / 2 + column * tabHeaderSize,
                     area.y + tabHeaderSize / 2 + row * tabHeaderSize * 2,
                     tabHeaderSize, tabHeaderSize);
}