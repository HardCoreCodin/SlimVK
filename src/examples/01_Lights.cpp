#include "./shaders.h"
#include "../slim/app.h"
using namespace gpu;

struct ExampleVulkanApp : SlimApp {
    VertexBuffer vertex_buffer;
    IndexBuffer index_buffer;
    GraphicsPipeline graphics_pipeline;

    void OnInit() override {
        vertex_buffer.create(VERTEX_COUNT, sizeof(Vertex));
        index_buffer.create(INDEX_COUNT, sizeof(u16));
        vertex_buffer.upload(vertices);
        index_buffer.upload(indices);
        graphics_pipeline.createFromSourceStrings(vertex_descriptor,
            vertex_shader_source_string,
            fragment_shader_source_string);
    }
    void OnShutdown() override {
        graphics_pipeline.destroy();
        index_buffer.destroy();
        vertex_buffer.destroy();
    }
    void OnRender() override {
            beginRenderPass();
                graphics_command_buffer->bind(graphics_pipeline);
                graphics_command_buffer->bind(vertex_buffer);
                graphics_command_buffer->bind(index_buffer);
                graphics_command_buffer->draw(index_buffer);
            endRenderPass();
    }
};

SlimApp* createApp() {
    return (SlimApp*)new ExampleVulkanApp();
}