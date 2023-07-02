#include "./shaders.h"
#include "../slim/core/transform.h"
#include "../slim/math/utils.h"
#include "../slim/viewport/navigation.h"
#include "../slim/viewport/frustum.h"
#include "../slim/app.h"
using namespace gpu;

struct ExampleVulkanApp : SlimApp {
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
    GraphicsPipeline graphics_pipeline;

    DescriptorPool descriptor_pool_for_uniform_buffers;
    DescriptorSetLayout vertex_shader_descriptor_set_layout;
    DescriptorSets descriptor_sets_for_vertex_shader;
    PipelineLayout graphics_pipeline_layout;
    PushConstantsLayout push_constants_layout;

    Camera camera{{}, {0, 0, -3.5}}, *cameras{&camera};
    Navigation navigation;
    Frustum frustum;

    struct PushConstants {
        alignas(16) vec3 offset;
    };
    PushConstants push_constants;

    struct UniformBufferObject {
        alignas(16) mat4 model;
        alignas(16) mat4 view;
        alignas(16) mat4 proj;
    };
    UniformBufferObject ubo;

    Transform xform{};

    void OnInit() override {
        push_constants_layout.addForVertexAndFragment(sizeof(PushConstants));
        vertex_buffer.create(VERTEX_COUNT, sizeof(TriangleVertex));
        index_buffer.create(INDEX_COUNT, sizeof(u16));

        vertex_shader_descriptor_set_layout.createForVertexUniformBuffer();
        graphics_pipeline_layout.create(&vertex_shader_descriptor_set_layout, &push_constants_layout);
//        graphics_pipeline_layout.create(nullptr, &push_constants_layout);

        descriptor_pool_for_uniform_buffers.createForUniformBuffers(VULKAN_MAX_FRAMES_IN_FLIGHT);

        vertex_buffer.upload(vertices);
        index_buffer.upload(indices);

        VkDescriptorSetLayout descriptor_set_layouts[VULKAN_MAX_FRAMES_IN_FLIGHT];
        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            descriptor_set_layouts[i] = vertex_shader_descriptor_set_layout.handle;
            vertex_uniform_buffers[i].create(sizeof(UniformBufferObject));
        }
        descriptor_pool_for_uniform_buffers.allocate(VULKAN_MAX_FRAMES_IN_FLIGHT, descriptor_set_layouts, descriptor_sets_for_vertex_shader);

        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++)
            vertex_uniform_buffers[i].update(descriptor_sets_for_vertex_shader.handles[i]);

        graphics_pipeline.createFromSourceStrings(
            present::render_pass,
            graphics_pipeline_layout,
            vertex_descriptor,
            vertex_shader_source_string,
            fragment_shader_source_string);
    }

    void OnUpdate(f32 delta_time) override {
        if (!controls::is_pressed::alt) navigation.update(camera, delta_time);
//        xform.orientation.rotateAroundZ(delta_time * DEG_TO_RAD*90.0f);
        frustum.updateProjection(camera.focal_length, (f32)present::swapchain_rect.extent.height / (f32)present::swapchain_rect.extent.width);

        ubo = {
            Mat4(xform.orientation, xform.scale, xform.position),
            Mat4(camera.orientation, camera.position).inverted(),
            mat4{frustum.projection.scale.x, 0, 0, 0,
             0, -frustum.projection.scale.y, 0, 0,
             0, 0, frustum.projection.scale.z, 1,
             0, 0, frustum.projection.shear, 0}
//            mat4(frustum.projection.scale, frustum.projection.shear)
        };
//        push_constants.view = ubo.view;
//        push_constants.proj = ubo.proj;
        vertex_uniform_buffers[present::current_frame].upload(&ubo);
    }

    void OnRenderMainPass(GraphicsCommandBuffer &command_buffer) override {
        graphics_pipeline.bind(command_buffer);
        descriptor_sets_for_vertex_shader.bind(present::current_frame, graphics_pipeline_layout, command_buffer);
        vertex_buffer.bind(command_buffer);
        index_buffer.bind(command_buffer);
        for (int i = -2; i < 3; i++) {
            push_constants.offset = {(f32)i, (f32)i, (f32)i};
            graphics_pipeline_layout.pushConstants(command_buffer, push_constants_layout.ranges[0], &push_constants);
            index_buffer.draw(command_buffer);
        }
    }

    void OnMouseButtonDown(mouse::Button &mouse_button) override {
        mouse::pos_raw_diff_x = mouse::pos_raw_diff_y = 0;
    }
    void OnMouseButtonDoubleClicked(mouse::Button &mouse_button) override {
        if (&mouse_button == &mouse::left_button) {
            mouse::is_captured = !mouse::is_captured;
            os::setCursorVisibility(!mouse::is_captured);
            os::setWindowCapture(    mouse::is_captured);
            OnMouseButtonDown(mouse_button);
        }
    }
    void OnKeyChanged(u8 key, bool is_pressed) override {
        Move &move = navigation.move;
        Turn &turn = navigation.turn;
        if (key == 'Q') turn.left     = is_pressed;
        if (key == 'E') turn.right    = is_pressed;
        if (key == 'R') move.up       = is_pressed;
        if (key == 'F') move.down     = is_pressed;
        if (key == 'W') move.forward  = is_pressed;
        if (key == 'S') move.backward = is_pressed;
        if (key == 'A') move.left     = is_pressed;
        if (key == 'D') move.right    = is_pressed;
    }

    void OnShutdown() override {
        graphics_pipeline.destroy();
        graphics_pipeline_layout.destroy();
        index_buffer.destroy();
        vertex_buffer.destroy();
        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) vertex_uniform_buffers[i].destroy();

        vertex_shader_descriptor_set_layout.destroy();
        descriptor_pool_for_uniform_buffers.destroy();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}