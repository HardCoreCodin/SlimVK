#pragma once

#include "../core/gpu.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"
#include "../../core/transform.h"

namespace line_rendering {
    using namespace gpu;

    static Mesh box_mesh{CubeEdgesType::Quad};
    static GPUMesh box{};

    const char* vertex_shader_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 out_position;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec4 color;
    float z_offset;
} push_constant;

void main() {
    gl_Position = push_constant.mvp * vec4(in_position, 1.0);
    gl_Position.w += push_constant.z_offset;
})VERTEX_SHADER";

    const char* fragment_shader_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec4 color;
    float z_offset;
} push_constant;

void main() {
    out_color = push_constant.color;
})FRAGMENT_SHADER";

    struct PushConstant {
        alignas(16) mat4 mvp;
        alignas(16) vec4 color;
        alignas(16) f32 z_offset;
    };
    PushConstant push_constant;
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertexAndFragment(sizeof(PushConstant))}};

    VertexDescriptor vertex_descriptor{sizeof(vec3),1, { _vec3 }};
    VertexShader vertex_shader{};
    FragmentShader fragment_shader{};

    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};

    bool create(RenderPass *render_pass = &present::render_pass) {
        if (vertex_shader.handle)
            return true;

        if (!vertex_shader.createFromSourceString(vertex_shader_string, &vertex_descriptor, "line_vertex_shader"))
            return false;

        if (!fragment_shader.createFromSourceString(fragment_shader_string, "line_fragment_shader"))
            return false;

        if (!pipeline_layout.create(nullptr, 0, &push_constant_spec))
            return false;

        if (!pipeline.create(render_pass->handle, pipeline_layout.handle, &vertex_shader, &fragment_shader, true))
            return false;

        if (!box.create(box_mesh))
            return false;

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
        box.destroy();
    }

    void setModel(const GraphicsCommandBuffer &command_buffer, const mat4 &mvp_matrix, const Color &line_color = White, f32 line_opacity = 1.0f, f32 z_offset = 0.00001f) {
        push_constant.mvp = mvp_matrix;
        push_constant.color = Vec4(line_color, line_opacity);
        push_constant.z_offset = z_offset;
        pipeline_layout.pushConstants(command_buffer, push_constant_spec.ranges[0], &push_constant);
    }

    void drawBox(const GraphicsCommandBuffer &command_buffer, const mat4 &mvp_matrix, const Color &line_color = White, f32 line_opacity = 1.0f, f32 z_offset = 0.00001f, u8 sides = BOX__ALL_SIDES) {
        pipeline.bind(command_buffer);
        setModel(command_buffer, mvp_matrix, line_color, line_opacity, z_offset);

        box.edge_buffer.bind(command_buffer);
        if (sides == BOX__ALL_SIDES)
            box.edge_buffer.draw(command_buffer);
        else {
            if (sides & BoxSide_Left  )
                box.edge_buffer.draw(command_buffer, 8, 3 * 8);
            if (sides & BoxSide_Right )
                box.edge_buffer.draw(command_buffer, 8, 1 * 8);
            if (sides & BoxSide_Bottom)
                box.edge_buffer.draw(command_buffer, 8, 5 * 8);
            if (sides & BoxSide_Top   )
                box.edge_buffer.draw(command_buffer, 8, 4 * 8);
            if (sides & BoxSide_Back  )
                box.edge_buffer.draw(command_buffer, 8, 0 * 8);
            if (sides & BoxSide_Front )
                box.edge_buffer.draw(command_buffer, 8, 2 * 8);
        }
    }
}