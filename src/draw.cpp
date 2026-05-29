#include "draw.hpp"
#include "math_util.hpp"
#include "common.hpp"
#include "log.hpp"

#ifdef GRAPHICS_DEBUG_DX

#pragma comment(lib, "d3d12.lib")

#define Rectangle d3dRectangle
#include <d3d12.h>
#undef Rectangle
#endif

void start_render(RenderContext& context, SDL_Window* window) {
    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(context.device);
    if (!command_buffer) {
        return;
    }

    SDL_GPUTexture* swapchain = nullptr;
    u32 swapchain_width = 0;
    u32 swapchain_height = 0;
    SDL_WaitAndAcquireGPUSwapchainTexture(context.frame.command_buffer, window, &swapchain, &swapchain_width, &swapchain_height);

    context.frame.command_buffer = command_buffer;
    context.frame.swapchain = { swapchain, swapchain_width, swapchain_height };
}

void end_render(RenderContext& context) {
    if (context.frame.command_buffer)
    {
        SDL_SubmitGPUCommandBuffer(context.frame.command_buffer);
    }
}

void RenderContext::start_render_pass() {
    SDL_GPURenderPass* render_pass = nullptr;

    if (frame.swapchain.texture)
    {
        // render to the swapchain
        SDL_GPUColorTargetInfo color_targets[1] = {};
        color_targets[0].texture = frame.swapchain.texture;
        color_targets[0].mip_level = 0;
        color_targets[0].layer_or_depth_plane = 0;
        color_targets[0].clear_color = SDL_FColor{ DEBUG_COLOR.r, DEBUG_COLOR.g, DEBUG_COLOR.b, DEBUG_COLOR.a };
        color_targets[0].load_op = SDL_GPU_LOADOP_CLEAR;
        color_targets[0].store_op = SDL_GPU_STOREOP_STORE;
        color_targets[0].resolve_texture = nullptr;
        color_targets[0].resolve_mip_level = 0;
        color_targets[0].resolve_layer = 0;
        color_targets[0].cycle = false;
        color_targets[0].cycle_resolve_texture = false;

        render_pass = SDL_BeginGPURenderPass(frame.command_buffer, color_targets, 1, nullptr);

        SDL_BindGPUGraphicsPipeline(render_pass, graphics);
    }

    frame.render_pass = render_pass;
}

void RenderContext::end_render_pass() {
    if (frame.render_pass)
    {
        SDL_EndGPURenderPass(frame.render_pass);
        frame.render_pass = nullptr;
    }
}

void RenderContext::start_copy_pass() {
    SDL_BeginGPUCopyPass(frame.command_buffer);
}

void RenderContext::end_copy_pass() {
    SDL_EndGPUCopyPass(frame.copy_pass);
    frame.copy_pass = nullptr;
}

bool initialize_render_context(RenderContext* render, SDL_Window* window)
{
#ifdef GRAPHICS_DEBUG_DX
    ID3D12Debug* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, nullptr);

    SDL_Renderer* renderer = SDL_CreateGPURenderer(device, window);
    if (!renderer)
    {
        SDL_Log("Failed to create renderer with SDL: %s\n", SDL_GetError());
        return false;
    }

    int render_size_x, render_size_y;
    if (!SDL_GetRenderOutputSize(renderer, &render_size_x, &render_size_y)) {
        return false;
    }

    u32 render_width = u32(render_size_x);
    u32 render_height = u32(render_size_y);

    SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device, window);
    SDL_PixelFormat pixel_format = SDL_GetPixelFormatFromGPUTextureFormat(format);
    log_info("Swapchain pixel format: %s", SDL_GetPixelFormatName(pixel_format));
    log_info("Render size: %d %d", render_width, render_height);

    render->device = device;
    render->renderer = renderer;
    render->render_size = vec2(render_size_x, render_size_y);

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        log_error("Could not claim window for gpu device");
        return false;
    }

    mat4x4 orthographic = orthographic_projection_matrix(-1.0, 1.0, -1.0, 1.0, 0.0, 1.0);
    mat4x4 camera = camera_matrix(vec2(0, 0), vec2(1,1));
    mat4mul(&render->mvp, &orthographic, &camera);

    return true;
}

bool init_gpu_renderer(RenderContext* render, SDL_Window* window, SDL_GPUShader* vertex, SDL_GPUShader* fragment)
{
    SDL_GPUVertexBufferDescription vertex_buffer_description[1] = {};
    SDL_GPUVertexAttribute vertex_attributes[3] = {};
    vertex_buffer_description[0].slot = 0;                        /**< The binding slot of the vertex buffer. */
    vertex_buffer_description[0].pitch = sizeof(Vertex);                       /**< The size of a single element + the offset between elements. */
    vertex_buffer_description[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;  /**< Whether attribute addressing is a function of the vertex index or instance index. */
    vertex_buffer_description[0].instance_step_rate = 0;          /**< Reserved for future use. Must be set to 0. */

    vertex_attributes[0].location = 0;                    /**< The shader input location index. */
    vertex_attributes[0].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;  /**< The size and type of the attribute data. */
    vertex_attributes[0].offset = 0;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    vertex_attributes[1].location = 1;                    /**< The shader input location index. */
    vertex_attributes[1].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;  /**< The size and type of the attribute data. */
    vertex_attributes[1].offset = sizeof(float) * 2;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    vertex_attributes[2].location = 2;                    /**< The shader input location index. */
    vertex_attributes[2].buffer_slot = 0;                 /**< The binding slot of the associated vertex buffer. */
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;  /**< The size and type of the attribute data. */
    vertex_attributes[2].offset = sizeof(float) * 4;                      /**< The byte offset of this attribute relative to the start of the vertex element. */

    SDL_GPUVertexInputState vertex_input = {
        vertex_buffer_description,  /**< A pointer to an array of vertex buffer descriptions. */
        ARRAY_SIZE(vertex_buffer_description),                          /**< The number of vertex buffer descriptions in the above array. */
        vertex_attributes,                   /**< A pointer to an array of vertex attribute descriptions. */
        ARRAY_SIZE(vertex_attributes)                          /**< The number of vertex attribute descriptions in the above array. */
    };
    SDL_GPURasterizerState rasterizer = {};
    rasterizer.fill_mode = SDL_GPU_FILLMODE_FILL;         /**< Whether polygons will be filled in or drawn as lines. */
    rasterizer.cull_mode = SDL_GPU_CULLMODE_NONE;         /**< The facing direction in which triangles will be culled. */
    rasterizer.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;       /**< The vertex winding that will cause a triangle to be determined as front-facing. */
    // rasterizer.depth_bias_constant_factor;  /**< A scalar factor controlling the depth value added to each fragment. */
    // rasterizer.depth_bias_clamp;            /**< The maximum depth bias of a fragment. */
    // rasterizer.depth_bias_slope_factor;     /**< A scalar factor applied to a fragment's slope in depth calculations. */
    // rasterizer.enable_depth_bias;            /**< true to bias fragment depth values. */
    // rasterizer.enable_depth_clip;            /**< true to enable depth clip, false to enable depth clamp. */

    SDL_GPUMultisampleState multisample = {};
    multisample.sample_count = SDL_GPU_SAMPLECOUNT_1;  /**< The number of samples to be used in rasterization. */
    multisample.sample_mask = 0;               /**< Reserved for future use. Must be set to 0. */
    multisample.enable_mask = false;                 /**< Reserved for future use. Must be set to false. */
    multisample.enable_alpha_to_coverage = false;    /**< true enables the alpha-to-coverage feature. */

    SDL_GPUDepthStencilState stencil = {};
    stencil.enable_depth_test = false;                     /**< true enables the depth test. */
    stencil.enable_depth_write = false;                    /**< true enables depth writes. Depth writes are always disabled when enable_depth_test is false. */
    stencil.enable_stencil_test = false;                   /**< true enables the stencil test. */

    SDL_GPUColorTargetBlendState blend_state = {};
    blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;     /**< The value to be multiplied by the source RGB value. */
    blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;     /**< The value to be multiplied by the destination RGB value. */
    blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;                /**< The blend operation for the RGB components. */
    blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;     /**< The value to be multiplied by the source alpha. */
    blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;     /**< The value to be multiplied by the destination alpha. */
    blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;                /**< The blend operation for the alpha component. */
    // blend_state.color_write_mask = 0;  /**< A bitmask specifying which of the RGBA components are enabled for writing. Writes to all channels if enable_color_write_mask is false. */
    blend_state.enable_blend = true;                            /**< Whether blending is enabled for the color target. */
    blend_state.enable_color_write_mask = false;                 /**< Whether the color write mask is enabled. */


    SDL_GPUColorTargetDescription color_target_description[1] = {};

    color_target_description[0].format = SDL_GetGPUSwapchainTextureFormat(render->device, window);               /**< The pixel format of the texture to be used as a color target. */
    color_target_description[0].blend_state = blend_state;  /**< The blend state to be used for the color target. */

    SDL_GPUGraphicsPipelineTargetInfo target_info = {};
    target_info.color_target_descriptions = color_target_description;  /**< A pointer to an array of color target descriptions. */
    target_info.num_color_targets = ARRAY_SIZE(color_target_description);                                        /**< The number of color target descriptions in the above array. */
    target_info.depth_stencil_format = {};                       /**< The pixel format of the depth-stencil target. Ignored if has_depth_stencil_target is false. */
    target_info.has_depth_stencil_target = false;                                   /**< true specifies that the pipeline uses a depth-stencil target. */

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertex;
    pipelineInfo.fragment_shader = fragment;
    pipelineInfo.vertex_input_state = vertex_input;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizer;
    pipelineInfo.multisample_state = multisample,
    pipelineInfo.depth_stencil_state = stencil;
    pipelineInfo.target_info = target_info;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(render->device, &pipelineInfo);
    if (!pipeline) {
        log_error("Failed to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_GPUBuffer* vertex_buffer = nullptr;
    SDL_GPUBuffer* index_buffer = nullptr;

    SDL_GPUBufferCreateInfo vertex_info = {};
    vertex_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_info.size = 1024;  // @todo
    SDL_GPUBufferCreateInfo index_info = {};
    index_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_info.size = 1024;  // @todo
    vertex_buffer = SDL_CreateGPUBuffer(render->device, &vertex_info);
    index_buffer = SDL_CreateGPUBuffer(render->device, &index_info);

    if (!(vertex_buffer && index_buffer)) {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = 1024;  // @todo
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(render->device, &transferInfo);

    render->graphics = pipeline;
    render->vertex_buffer = { vertex_buffer, vertex_info.size, 0 };
    render->index_buffer = { index_buffer, index_info.size, 0 };
    render->transfer_buffer = { transfer_buffer, transferInfo.size };

    return true;
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
        log_error("Could not load shader %s", path);
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
        log_error("%s", SDL_GetError());
        return false;
    }

    shader.shader = shaderObj;

    return true;
}

void RenderContext::set_viewport(Viewport viewport)
{
    SDL_SetGPUViewport(frame.render_pass, &viewport);
}

bool RenderContext::add_mesh(MeshData& meshData, Mesh& mesh)
{
    u32 vBufferUsage = meshData.vertices.size() * sizeof(Vertex);
    u32 iBufferUsage = meshData.indices.size() * sizeof(u16);

    if (vertex_buffer.used + vBufferUsage >= vertex_buffer.size)
    {
        return false;
    }
    if (index_buffer.used + iBufferUsage >= index_buffer.size)
    {
        return false;
    }

    u32 nVertices = meshData.vertices.size();
    u32 nIndices = meshData.indices.size();

    u8* memory = (u8*) SDL_MapGPUTransferBuffer(device, transfer_buffer.buffer, true);

    u32 vertexSize = meshData.vertices.size() * sizeof(Vertex);
    u32 indexSize = meshData.indices.size() * sizeof(u16);

    memcpy(memory + 0, meshData.vertices.data(), vertexSize);
    memcpy(memory + vertexSize, meshData.indices.data(), indexSize);

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer.buffer);

    SDL_GPUTransferBufferLocation vertexSource = {};
    SDL_GPUTransferBufferLocation indexSource = {};
    vertexSource.transfer_buffer = transfer_buffer.buffer;
    vertexSource.offset = 0;
    indexSource.transfer_buffer = transfer_buffer.buffer;
    indexSource.offset = vertexSize;

    SDL_GPUBufferRegion vertexDestination = {};
    SDL_GPUBufferRegion indexDestination = {};
    vertexDestination.buffer = vertex_buffer.buffer;
    vertexDestination.offset = vertex_buffer.used * sizeof(Vertex);
    vertexDestination.size = (vertex_buffer.size - vertex_buffer.used) * sizeof(Vertex);

    indexDestination.buffer = index_buffer.buffer;
    indexDestination.offset = index_buffer.used * sizeof(u16);
    indexDestination.size = (index_buffer.size - index_buffer.used) * sizeof(u16);

    SDL_UploadToGPUBuffer(frame.copy_pass, &vertexSource, &vertexDestination, true);
    SDL_UploadToGPUBuffer(frame.copy_pass, &indexSource, &indexDestination, true);

    vertex_buffer.used += vBufferUsage;
    index_buffer.used += iBufferUsage;
    return true;
}

// old code
/*
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
*/
