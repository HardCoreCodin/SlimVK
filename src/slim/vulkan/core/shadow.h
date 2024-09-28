#pragma once

#include "./image.h"
#include "./framebuffer.h"
#include "./graphics.h"
#include "./render_pass.h"
#include "./pipeline.h"


namespace gpu {	
	namespace shadow_pass {
		const char* vertex_shader_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;

layout(push_constant) uniform PushConstant {
	mat4 mvp;
} push_constant;

out gl_PerVertex 
{
    vec4 gl_Position;   
};
void main() {
	gl_Position = push_constant.mvp * vec4(in_position, 1.0);
	gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
})VERTEX_SHADER";
/*
		const char* fragment_shader_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

void main() 
{	
	color = vec4(1.0, 0.0, 0.0, 1.0);
})FRAGMENT_SHADER";
*/
		struct PushConstant {
			alignas(16) mat4 mvp;
		};
		PushConstant push_constant;
		PushConstantSpec push_constant_spec{{PushConstantRangeForVertex(sizeof(PushConstant))}};



		struct TriangleVertex {
			vec3 position{};
			vec3 normal;
			vec3 tangent;
			vec2 uv;
		};
		VertexDescriptor vertex_descriptor{
			sizeof(TriangleVertex),
			1, {
				_vec3,
			}
		};
		VertexShader vertex_shader{};
		//FragmentShader fragment_shader{};

		PipelineLayout pipeline_layout{};
		GraphicsPipeline pipeline{};

		RenderPass render_pass;

		bool create() {
			render_pass.createShadowPass();

			if (vertex_shader.handle)
				return true;

			if (!vertex_shader.createFromSourceString(vertex_shader_string, &vertex_descriptor, "shadow_pass_vertex_shader"))
				return false;

			//if (!fragment_shader.createFromSourceString(fragment_shader_string, "shadow_pass_fragment_shader"))
			//	return false;

			if (!pipeline_layout.create(nullptr, 0, &push_constant_spec))
				return false;

			if (!pipeline.create(render_pass.handle, pipeline_layout.handle, &vertex_shader));//, &fragment_shader))
				return false;

			return true;
		}

		void destroy() {
			pipeline.destroy();
			pipeline_layout.destroy();
			//fragment_shader.destroy();
			vertex_shader.destroy();
			render_pass.destroy();
		}

		void setModel(const GraphicsCommandBuffer &command_buffer, const mat4 &mvp_matrix) {
			push_constant.mvp = mvp_matrix;
			pipeline_layout.pushConstants(command_buffer, push_constant_spec.ranges[0], &push_constant);
		}
	}

	struct ShadowMap {
		FrameBuffer frame_buffer;
		GPUImage image;
		f32 bias_constant;
		f32 bias_slope;

        bool create(u32 width, u32 height, f32 bias_constant = 1.25f, f32 bias_slope = 1.75f, const char* image_name = "shadow_map") {
			this->bias_constant = bias_constant;
			this->bias_slope = bias_slope;

			image.create(
				width, 
				height,
				present::depth_format,
				(
					(u8)GPUImageFlag::SAMPLE | 
					(u8)GPUImageFlag::FILTER | 
					(u8)GPUImageFlag::VIEW
				),
				image_name,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT
			);

			frame_buffer.create(width, height, shadow_pass::render_pass.handle, &image.view, 1);

			return true;
		}

		void beginWrite(GraphicsCommandBuffer &command_buffer) {
			VkRect2D rect;
			rect.extent.height = image.height;
			rect.extent.width = image.width;
			rect.offset.x = 0;
			rect.offset.y = 0;

			command_buffer.beginRenderPass(shadow_pass::render_pass, frame_buffer, rect, false);
			
			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artifacts
			vkCmdSetDepthBias(command_buffer.handle, bias_constant, 0.0f, bias_slope);

			shadow_pass::pipeline.bind(command_buffer);
		}

		void endWrite(GraphicsCommandBuffer &command_buffer) {
			command_buffer.endRenderPass();
		}

		void destroy() {
			frame_buffer.destroy();
			image.destroy();
		}
	};
}