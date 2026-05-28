#ifndef _DRAW_H
#define _DRAW_H

#include <SDL3/SDL.h>
#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "text.hpp"

static const ColorF DEBUG_COLOR =  ColorF(0.6, 0.5, 0.4, 1.0);

#define GRAPHICS_DEBUG 0

#ifdef _WIN32
#if GRAPHICS_DEBUG
#define GRAPHICS_DEBUG_DX
#endif
#endif

struct FrameRenderContext {
    SDL_GPUCommandBuffer* command_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
};

struct RenderContext {
    vec2 render_size = {};
    SDL_Renderer* renderer = nullptr;
    SDL_GPUTexture* render_target = nullptr;
    SDL_GPUDevice* device = nullptr;

    SDL_GPUBuffer* buffer = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUGraphicsPipeline* graphics = nullptr;
    FrameRenderContext frame = {};

    DArray<SDL_Vertex> vertex_scratch;
    DArray<int> index_scratch;
};

using Texture = SDL_Texture;

struct Vertex {
    float x;
    float y;
    float uvx;
    float uvy;
    float r;
    float g;
    float b;
    float a;
};

struct Mesh {
    DArray<vec2> points;
    DArray<int> indices;

    ~Mesh() {
        points.reset();
        indices.reset();
    }
};

enum ShaderStage {
    ShaderStageVertex = SDL_GPU_SHADERSTAGE_VERTEX,
    ShaderStageFragment = SDL_GPU_SHADERSTAGE_FRAGMENT,
};

struct Shader {
    SDL_GPUShader* shader = nullptr;
    ShaderStage stage = {};
    int numSamplers = 0;
    int numStorageTextures = 0;
    int numStorageBuffers = 0;
    int numUniformBuffers = 0;

    bool is_valid() const {
        return shader != nullptr;
    }
};

bool initialize_render_context(RenderContext* render, SDL_Window* window);
bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment);

void start_render(RenderContext& context);
void end_render(RenderContext& context);

bool loadShader(RenderContext& context, Shader& shader, const char* path);

void draw_circle(const RenderContext& context, vec2 center, float radius);
void draw_quadratic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, float thick, ColorF color);
void draw_cubic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, vec2 p3, float thick, ColorF color);

void draw_texture(const RenderContext& context, Rectangle area, SDL_Texture* texture);

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, Color color);
Text create_text(SDL_Renderer* renderer, String text, Font font, Color color);

void render_texture(SDL_Renderer* renderer, Rectangle area, Texture* texture, bool strech = false);
void render_textured_rectangle(SDL_Renderer* renderer, Rectangle rect, Texture* texture, Color color, bool strech = false, bool center = true);

void render_text_size(SDL_Renderer* renderer, Text text, vec2 where, vec2 absolute_scale = vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Text text, vec2 where, vec2 scale_factor = vec2(0,0));

#endif // _DRAW_H