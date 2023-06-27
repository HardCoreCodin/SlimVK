#pragma once

#include "./buffer.h"
#include "./command.h"
#include "./render_pass.h"
#include "./pipeline.h"

namespace gpu {
    struct VertexBuffer;
    struct IndexBuffer;

    struct GraphicsCommandBuffer : CommandBuffer {
        GraphicsCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, graphics_queue, graphics_queue_family_index) {}

        void setViewport(VkViewport *viewport) const;
        void setScissor(VkRect2D *scissor) const;
        bool beginRenderPass(RenderPass &renderpass, VkFramebuffer framebuffer); //, RenderTarget &render_target);
        bool endRenderPass();
        void bind(const GraphicsPipeline &graphics_pipeline) const;
        void bind(const VertexBuffer &vertex_buffer, u32 offset = 0) const;
        void bind(const IndexBuffer &index_buffer, u32 offset = 0) const;
        void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) const;
        void draw(const IndexBuffer &index_buffer, u32 instance_count = 1, u32 first_index = 0, i32 first_vertex = 0, u32 first_instance = 0) const;
    };

    struct GraphicsCommandPool : CommandPool<GraphicsCommandBuffer> {
        void create(bool transient = false) {
            _create(graphics_queue_family_index, transient);
            SLIM_LOG_INFO("Graphics command pool created.")
        }
    };

    GraphicsCommandPool graphics_command_pool;
    GraphicsCommandPool transient_graphics_command_pool;
    GraphicsCommandBuffer *graphics_command_buffer = nullptr;
    GraphicsCommandBuffer transient_graphics_command_buffer;

    struct GraphicsBuffer : Buffer {
        bool resize(u64 new_size) {
            return transient_graphics_command_buffer.resize(*this, new_size);
        }

        bool download(void** to, u64 size = 0, u64 from_offset = 0) {
            return transient_graphics_command_buffer.download(*this, to, size, from_offset);
        }

        bool upload(const void* from, u64 size = 0, u64 to_offset = 0) {
            return transient_graphics_command_buffer.upload(from, *this, size, to_offset);
        }

        bool copyTo(const Buffer &to_buffer, u64 size, u64 from_offset, u64 to_offset) const {
            return transient_graphics_command_buffer.copy(*this, to_buffer, size, from_offset, to_offset);
        }
    };

    struct VertexBuffer : GraphicsBuffer {
        u32 vertex_count = 0;

        bool create(u32 count, u8 vertex_size) {
            *this = {};
            vertex_count = count;
            return Buffer::create(BufferType::Vertex, (u64)count * vertex_size);
        }
    };

    struct IndexBuffer : GraphicsBuffer {
        VkIndexType index_type = VK_INDEX_TYPE_UINT32;
        u32 index_count = 0;

        bool create(u32 count, u8 index_size = 4) {
            *this = {};
            if (index_size == 4)
                index_type = VK_INDEX_TYPE_UINT32;
            else if (index_size == 2)
                index_type = VK_INDEX_TYPE_UINT16;
            else if (index_size == 1)
                index_type = VK_INDEX_TYPE_UINT8_EXT;
            else {
                SLIM_LOG_ERROR("Unknown index size")
                return false;
            }
            index_count = count;

            bool result = Buffer::create(BufferType::Index, (u64)count * index_size);
            return result;
        }
    };

    void GraphicsCommandBuffer::setViewport(VkViewport *viewport) const {
        vkCmdSetViewport(handle, 0, 1, viewport);
    }

    void GraphicsCommandBuffer::setScissor(VkRect2D *scissor) const {
        vkCmdSetScissor(handle, 0, 1, scissor);
    }

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

    void GraphicsCommandBuffer::draw(const IndexBuffer &index_buffer, u32 instance_count, u32 first_index, i32 first_vertex, u32 first_instance) const {
        vkCmdDrawIndexed(handle, index_buffer.index_count, instance_count, first_index, first_vertex, first_instance);
    }
}