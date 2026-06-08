#ifndef _DRAW_H
#define _DRAW_H

#include <SDL3/SDL.h>
#include "common.hpp"
#include "math_util.hpp"
#include "template.hpp"
#include "text.hpp"

#define GRAPHICS_DEBUG 1

static const ColorF DEBUG_COLOR =  ColorF(0.6, 0.5, 0.4, 1.0);

#ifdef _WIN32
#if GRAPHICS_DEBUG
#define GRAPHICS_DEBUG_DX
#endif
#endif

enum RenderStates {
    RenderStatePlanet,
    RenderStateCount,
};

using Texture = SDL_Texture;
using Viewport = SDL_GPUViewport;

struct Vertex {
    float x = 0;
    float y = 0;
    float uvx = 0;
    float uvy = 0;
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    Vertex() {}
    Vertex(float x, float y, float uvx, float uvy, float r, float g, float b, float a)
        :
        x(x), y(y), uvx(uvx), uvy(uvy), r(r), g(g), b(b), a(a)
    {}
};

struct GPUTexture {
    SDL_GPUTexture* texture = nullptr;
    u32 width = 0;
    u32 height = 0;
};

struct MeshData {
    DArray<Vertex> vertices = {};
    DArray<u16> indices = {};
};

struct MeshReference {
    u32 numVertices = 0;
    u32 numIndices = 0;
    u32 vertex_offset = 0;
    u32 index_offset = 0;
};

struct Mesh {
    MeshData data = {};
    MeshReference ref = {};
};

struct GPUBuffer {
    SDL_GPUBuffer* buffer = nullptr;
    u32 size = 0;
    u32 used = 0;
};

struct TransferBuffer {
    SDL_GPUTransferBuffer* buffer = nullptr;
    u32 size = 0;
};

struct FrameContext {
    SDL_GPUCommandBuffer* command_buffer = nullptr;
    SDL_GPURenderPass* render_pass = nullptr;
    SDL_GPUCopyPass* copy_pass = nullptr;
    GPUTexture swapchain = {};
};

struct RenderContext {
    vec2 render_size = {};
    SDL_Renderer* renderer = nullptr;
    DArray<SDL_GPURenderState*> render_states = {};

    SDL_Texture* target_texture = nullptr;
    SDL_GPUTexture* render_target = nullptr;

    SDL_GPUDevice* device = nullptr;

    mat4x4 mvp = {};

    GPUBuffer vertex_buffer = {};
    GPUBuffer index_buffer = {};
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUGraphicsPipeline* graphics = nullptr;

    TransferBuffer transfer_buffer = {};

    FrameContext frame = {};

    DArray<SDL_Vertex> vertex_scratch = {};
    DArray<int> index_scratch = {};

    bool start_render_pass();
    void end_render_pass();
    bool start_copy_pass();
    void end_copy_pass();

    void set_viewport(Viewport viewport);

    bool add_mesh(MeshData& meshData, MeshReference& mesh);

    void draw_mesh(MeshReference mesh);
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

void start_frame(RenderContext& context, SDL_Window* window);
void end_frame(RenderContext& context);

bool loadShader(RenderContext& context, Shader& shader, const char* path);

void draw_segment(const RenderContext& context, vec2 start, vec2 end, float thick, ColorF color);
void draw_circle(const RenderContext& context, vec2 position, float radius, ColorF color);
void draw_arc(const RenderContext& context, vec2 center, float inner_radius, float outer_radius, float start_angle, float arc, ColorF color);
void draw_capsule(const RenderContext& context, vec2 center0, vec2 center1, float radius, ColorF color);
void draw_polygon(RenderContext& context, vec2 points[], int numPoints, ColorF color);
void draw_path(RenderContext& context, vec2 points[], int numPoints, float thick, ColorF color);
void draw_closed_path(RenderContext& context, vec2 points[], int numPoints, float thick, ColorF color);
void draw_quadratic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, float thick, ColorF color);
void draw_cubic_bezier(const RenderContext& context, vec2 p0, vec2 p1, vec2 p2, vec2 p3, float thick, ColorF color);

void draw_texture(const RenderContext& context, Rectangle area, SDL_Texture* texture);

SDL_Texture* render_text(SDL_Renderer* renderer, String text, Font font, Color color);
Text create_text(SDL_Renderer* renderer, String text, Font font, Color color);

void render_texture(SDL_Renderer* renderer, Rectangle area, Texture* texture, bool strech = false);
void render_textured_rectangle(SDL_Renderer* renderer, Rectangle rect, Texture* texture, Color color, bool strech = false, bool center = true);
void render_texture_with_tint(SDL_Renderer* renderer, Rectangle area, Texture* texture, ColorF tint, bool strech = false);


void render_text_size(SDL_Renderer* renderer, Text text, vec2 where, vec2 absolute_scale = vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Text text, vec2 where, vec2 scale_factor = vec2(0,0));

#endif // _DRAW_H