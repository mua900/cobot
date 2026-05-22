#ifndef _TEXT_H
#define _TEXT_H

#include "math_util.hpp"
#include <SDL3_ttf/SDL_ttf.h>

#define FONT_SIZE_SMALL   18.0
#define FONT_SIZE_MEDIUM  32.0
#define FONT_SIZE_LARGE   72.0
#define FONT_SIZE_EDITOR  24.0

struct Font {
    TTF_Font* font = NULL;
    float size = 0;
};

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size);
bool load_font_file(Font* font, const char* path, float size);

union UserData {
    s64 number;
    void* ptr;
};

struct Text {
    SDL_Texture* texture = NULL;
    String string = {};

    Text() {}
    Text(SDL_Texture* p_texture, String p_string) : texture(p_texture), string(p_string) {}

    void clear()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }

        string.data = NULL;
        string.size = 0;
    }
};

struct Icon {
    SDL_Texture* texture = nullptr;
    Color background = {};

    Icon () {}
    Icon (SDL_Texture* tex, Color bground) : texture(tex), background(bground) {}
};

struct IconButton {
    Icon icon = {};
    UserData data = {};

    IconButton() {}
    IconButton(SDL_Texture* tex, Color background) : icon(tex, background) {}
    IconButton(SDL_Texture* tex, Color background, s64 n) : icon(tex, background) {
        data.number = n;
    }
    IconButton(SDL_Texture* tex, Color background, void* ptr) : icon(tex, background) {
        data.ptr = ptr;
    }
};

#endif // _TEXT_H