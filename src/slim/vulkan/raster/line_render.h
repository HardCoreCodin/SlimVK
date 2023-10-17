#pragma once

#include "../core/gpu.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"

namespace line_rendering {
    using namespace gpu;

    const char* vertex_shader_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 out_position;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec3 color;
} push_constant;

void main() {
    gl_Position = push_constant.mvp * vec4(in_position, 1.0);
    gl_Position.w += 0.00001f;
})VERTEX_SHADER";

    const char* fragment_shader_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;
layout(push_constant) uniform PushConstant {
    mat4 mvp;
    vec3 color;
} push_constant;

void main() {
    out_color = vec4(push_constant.color, 1.0f);
})FRAGMENT_SHADER";

    struct PushConstant {
        alignas(16) mat4 mvp;
        alignas(16) vec3 color;
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

        if (!pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, fragment_shader, true))
            return false;

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
    }

    void setModel(const GraphicsCommandBuffer &command_buffer, const mat4 &mvp_matrix, const Color &line_color = White) {
        push_constant.mvp = mvp_matrix;
        push_constant.color = line_color;
        pipeline_layout.pushConstants(command_buffer, push_constant_spec.ranges[0], &push_constant);
    }
}