#pragma once

#include "./base.h"

namespace gpu {
    bool GraphicsCommandBuffer::beginRenderPass(RenderPass &renderpass, VkFramebuffer framebuffer) { //, RenderTarget &render_target) {
        // Begin the render pass.
        VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        begin_info.renderPass = renderpass.handle;
        begin_info.framebuffer = framebuffer;
        begin_info.renderArea = renderpass.config.rect;
        begin_info.clearValueCount = 0;
        VkClearValue clear_values[2] = {{}, {}};

        for (u32 i = 0; i < renderpass.config.attachment_count; ++i)
            if (renderpass.config.attachment_configs[i].flags & (u8)Attachment::Flag::Clear) {
                begin_info.clearValueCount = renderpass.config.attachment_count;

                switch (renderpass.config.attachment_configs[i].type) {
                    case Attachment::Type::Color: {
                        clear_values[i].color.float32[0] = renderpass.config.clear_color.r;
                        clear_values[i].color.float32[1] = renderpass.config.clear_color.g;
                        clear_values[i].color.float32[2] = renderpass.config.clear_color.b;
                        clear_values[i].color.float32[3] = 1.0f;
                    } break;
                    case Attachment::Type::Depth: {
                        clear_values[i].depthStencil.depth = renderpass.config.depth;
                        clear_values[i].depthStencil.stencil = 0;
                    } break;
                    case Attachment::Type::Stencil: {
                        clear_values[i].depthStencil.depth = renderpass.config.depth;
                        clear_values[i].depthStencil.stencil = renderpass.config.stencil;
                    }
                }
            }

        begin_info.pClearValues = begin_info.clearValueCount > 0 ? clear_values : nullptr;

        vkCmdBeginRenderPass(handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        state = State::InRenderPass;

        Color color{0.0f, 0.5f, 0.3f};
        VK_BEGIN_DEBUG_LABEL(handle, renderpass.config.name, color);

        return true;
    }

    bool GraphicsCommandBuffer::endRenderPass() {
        vkCmdEndRenderPass(handle);
        VK_END_DEBUG_LABEL(handle);

        state = State::Recording;

        return true;
    }

    void GraphicsCommandBuffer::bind(const GraphicsPipeline &graphics_pipeline) const {
        vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.handle);
    }

    void GraphicsCommandBuffer::bind(const VertexBuffer &vertex_buffer, u32 offset) const {
        VkDeviceSize offsets[1] = {offset};
        vkCmdBindVertexBuffers(handle, 0, 1, &vertex_buffer.handle, (VkDeviceSize*)offsets);
    }

    void GraphicsCommandBuffer::bind(const IndexBuffer &index_buffer, u32 offset) const {
        vkCmdBindIndexBuffer(handle, index_buffer.handle, offset, index_buffer.index_type);
    }

    void GraphicsCommandBuffer::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) const {
        vkCmdDraw(handle, vertex_count, instance_count, first_vertex, first_instance);
    }
}