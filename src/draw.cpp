#include "draw.hpp"
#include "math_util.hpp"
#include "common.hpp"

void draw_segment(const RenderContext& context, vec2 start, vec2 end, float thick, ColorF color)
{
    vec2 dir = (end - start).normalized();
    vec2 perp = vec2(-dir.y, dir.x);

    vec2 sleft = start + perp * thick;
    vec2 sright = start - perp * thick;
    vec2 eleft = end + perp * thick;
    vec2 eright = end - perp * thick;

    SDL_Vertex vertices[4];
    int indices[6];
    vertices[0].position = { sleft.x, sleft.y };
    vertices[0].color = { COLOR_ARG(color) };
    vertices[1].position = { sright.x, sright.y };
    vertices[1].color = { COLOR_ARG(color) };
    vertices[2].position = { eleft.x, eleft.y };
    vertices[2].color = { COLOR_ARG(color) };
    vertices[3].position = { eright.x, eright.y };
    vertices[3].color = { COLOR_ARG(color) };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 1;
    indices[5] = 3;

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 4, indices, 6);
}

void draw_arrow(const RenderContext& context, vec2 start, vec2 end, float thickness, ColorF color)
{
    // 3 for the arrow head, 4 for the quadrilateral below
    SDL_Vertex vertices[7];

    for (int i = 0; i < 7; i++) vertices[i].color = SDL_FColor {color.r, color.g, color.b, color.a};

    vec2 dir = end - start;
    float total_length = dir.magnitude();

    if (total_length < 1)
    {
        // subpixel arrow?
        return;
    }

    const float head_percentage = 0.2;  // 1 / 5 of the length is head
    const float head_width = thickness * 2;
    const float base_width = thickness;

    float head_size = total_length * head_percentage;
    dir = dir.normalized();
    vec2 ortho = vec2(-dir.y, dir.x);

    vec2 head_start = end - dir * head_size;
    vec2 arrow_left = head_start + ortho * head_width;
    vec2 arrow_right = head_start - ortho * head_width;
    vertices[0].position = SDL_FPoint { end.x, end.y };
    vertices[1].position = SDL_FPoint { arrow_left.x, arrow_left.y };
    vertices[2].position = SDL_FPoint { arrow_right.x, arrow_right.y };

    vec2 upper_base_left = head_start + ortho * base_width;
    vec2 upper_base_right = head_start - ortho * base_width;
    vec2 lower_base_left = upper_base_left - dir * total_length * (1.0 - head_percentage);
    vec2 lower_base_right = upper_base_right - dir * total_length * (1.0 - head_percentage);
    vertices[3].position = SDL_FPoint { upper_base_left.x, upper_base_left.y };
    vertices[4].position = SDL_FPoint { upper_base_right.x, upper_base_right.y };
    vertices[5].position = SDL_FPoint { lower_base_left.x, lower_base_left.y };
    vertices[6].position = SDL_FPoint { lower_base_right.x, lower_base_right.y };

    const int indices[9] = {
        0, 1, 2,  // head
        3, 5, 4,
        4, 5, 6
    };

    SDL_RenderGeometry(context.renderer, nullptr, vertices, 7, indices, ARRAY_SIZE(indices));
}

void draw_circle(const RenderContext& context, vec2 position, float radius, ColorF color)
{
    // change the number of vertices to use to configure how fine of an approximation we get
    #define NVERTICES 32
    SDL_Vertex vertices[NVERTICES + 1];

    SDL_Vertex center;
    center.position = SDL_FPoint {.x = position.x, .y = position.y};
    center.color = SDL_FColor { COLOR_ARG(color) };

    vertices[0] = center;

    // the angle between vertices and it's sin and cos
    const float angle = CONSTANT_TAU / float(NVERTICES);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    float xcomp = 1.0;
    float ycomp = 0.0;
    for (int i = 1; i <= NVERTICES; i++)
    {
        vertices[i].position.x = center.position.x + xcomp * radius;
        vertices[i].position.y = center.position.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };

        // rotate the vector
        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;
        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    int indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    SDL_RenderGeometry(context.renderer, NULL, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_capsule(const RenderContext& context, vec2 center0, vec2 center1, float radius, ColorF color)
{
    // total number of vertices used for either half circle sides of the capsule shape
    #define NVERTICES 32
    SDL_Vertex vertices[NVERTICES + 1];

    vec2 midpoint = (center0 + center1) / 2;

    vertices[0].position = { midpoint.x, midpoint.y };
    vertices[0].color = SDL_FColor { COLOR_ARG(color) };

    // the angle between vertices and it's sin and cos
    const float angle = CONSTANT_TAU / float(NVERTICES);
    const float c = std::cosf(angle);
    const float s = std::sinf(angle);

    vec2 axis = (center1 - center0).normalized();

    // perpendicular vector
    float xcomp = -axis.y;
    float ycomp = axis.x;

    for (int i = 1; i <= NVERTICES / 2; i++)
    {
        vertices[i].position.x = center0.x + xcomp * radius;
        vertices[i].position.y = center0.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    for (int i = NVERTICES / 2 + 1; i <= NVERTICES; i++)
    {
        vertices[i].position.x = center1.x + xcomp * radius;
        vertices[i].position.y = center1.y + ycomp * radius;
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };

        float n_xcomp = xcomp * c - ycomp * s;
        float n_ycomp = xcomp * s + ycomp * c;

        xcomp = n_xcomp;
        ycomp = n_ycomp;
    }

    int indices[NVERTICES * 3];
    for (int i = 0; i < NVERTICES - 1; i++)
    {
        indices[i * 3 + 0] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    indices[(NVERTICES - 1) * 3 + 0] = 0;
    indices[(NVERTICES - 1) * 3 + 1] = NVERTICES;
    indices[(NVERTICES - 1) * 3 + 2] = 1;

    SDL_RenderGeometry(context.renderer, NULL, vertices, ARRAY_SIZE(vertices), indices, ARRAY_SIZE(indices));
    #undef NVERTICES
}

void draw_quadratic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, float thick, ColorF color)
{
    vec2 prev = p0;

    const int resolution = 32;

    for (int i = 0; i < resolution; i++)
    {
        float t = float(i) / float(resolution);
        float it = 1.0f - t;
        vec2 p = (it * it * p0) + (2.0f * it * t * p1) + (t * t * p2);
        draw_segment(context, prev, p, thick, color);
        prev = p;
    }
}

void draw_cubic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, vec2 p3, float thick, ColorF color)
{
    vec2 prev = p0;

    const int resolution = 32;

    for (int i = 0; i < resolution; i++)
    {
        float t = float(i) / float(resolution);
        float it = 1.0f - t;
        vec2 p = (it * it * it * p0) + (3.0f * it * it * t * p1) + (3.0f * it * t * t * p2) + (t * t * t * p3);
        draw_segment(context, prev, p, thick, color);
        prev = p;
    }
}

void draw_mesh(const RenderContext& context, Mesh mesh, float scale, vec2 translate, ColorF color)
{
    for (int i = 0; i < mesh.points.size(); i++)
    {
        mesh.points[i] *= scale;
        mesh.points[i] += translate;

        context.vertex_scratch[i].position.x = mesh.points[i].x;
        context.vertex_scratch[i].position.y = mesh.points[i].y;
        context.vertex_scratch[i].color = SDL_FColor {color.r, color.g, color.b, color.a};
    }
    for (int i = 0; i < mesh.indices.size(); i ++)
    {
        context.index_scratch[i] = mesh.indices[i];
    }
    SDL_RenderGeometry(context.renderer, nullptr, context.vertex_scratch.data(), context.vertex_scratch.size(), context.index_scratch.data(), context.index_scratch.size());
}

void draw_texture(const RenderContext& context, Rectangle area, SDL_Texture* texture)
{
    SDL_FRect dst = { area.x, area.y, area.w, area.h };
    SDL_RenderTexture(context.renderer, texture, NULL, &dst);
}

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, Color color) {
    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Solid(font.font, text.data, text.size, sdl_color);

    if (!surface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (!texture) {
        SDL_DestroySurface(surface);
        return nullptr;
    }

    return texture;
}

Text create_text(SDL_Renderer* renderer, String text, Font font, Color color)
{
    SDL_Texture* texture = render_text(renderer, text, font, color);
    if (!texture) return Text();
    return Text(texture, text);
}

void render_text_size(SDL_Renderer* renderer, Text text, vec2 where, vec2 absolute_scale)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!absolute_scale.x)
    {
        absolute_scale = vec2(tex_w, tex_h);
    }

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - absolute_scale.x/2, where.y - absolute_scale.y/2, absolute_scale.x, absolute_scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

void render_text_scale(SDL_Renderer* renderer, Text text, vec2 where, vec2 scale_factor)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!scale_factor.x)
    {
        scale_factor = vec2(1,1);
    }

    vec2 scale = vec2(tex_w * scale_factor.x, tex_h * scale_factor.y);

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - scale.x/2, where.y - scale.y/2, scale.x, scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

bool loadShader(RenderContext& context, Shader& shader, const char* path)
{
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    SDL_GPUShaderStage shaderStage = SDL_GPUShaderStage(shader.stage);

    BinaryData code = {};
    if (!load_file(path, code)) {
        return false;
    }

    SDL_GPUShaderCreateInfo info = {};
    info.code_size = code.size;
    info.code = code.data;
    info.entrypoint = "main";
    info.format = format;
    info.stage = shaderStage;
    info.num_samplers = shader.numSamplers;
    info.num_storage_textures = shader.numStorageTextures;
    info.num_storage_buffers = shader.numStorageBuffers;
    info.num_uniform_buffers = shader.numUniformBuffers;
    
    SDL_GPUShader* shaderObj = SDL_CreateGPUShader(context.device, &info);
    if (!shaderObj) {
        return false;
    }

    shader.shader = shaderObj;

    return true;
}
