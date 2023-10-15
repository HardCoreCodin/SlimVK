#pragma once

#include "./buffer.h"
#include "./command.h"
#include "./framebuffer.h"
#include "./render_pass.h"


namespace gpu {
    struct VertexBuffer;
    struct IndexBuffer;

    struct GraphicsCommandBuffer : CommandBuffer {
        GraphicsCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, graphics_queue, graphics_queue_family_index) {}

        void setViewport(const VkViewport &viewport) const {
            vkCmdSetViewport(handle, 0, 1, &viewport);
        }

        void setScissor(const VkRect2D &rect) const {
            vkCmdSetScissor(handle, 0, 1, &rect);
        }

        void setViewportAndScissor(const VkRect2D &rect) {
            VkViewport viewport{};
            viewport.x = (float)rect.offset.x;
            viewport.y = (float)rect.offset.y;
            viewport.width = (float)rect.extent.width;
            viewport.height = (float)rect.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            setViewport(viewport);
            setScissor(rect);
        }

        bool beginRenderPass(const RenderPass &renderpass, const FrameBuffer &framebuffer, const VkRect2D &rect) { //, RenderTarget &render_target) {
            // Begin the render pass.
            VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            begin_info.renderPass = renderpass.handle;
            begin_info.framebuffer = framebuffer.handle;
            begin_info.renderArea = rect;
            begin_info.clearValueCount = 0;
            VkClearValue clear_values[2] = {{},
                                            {}};

            for (u32 i = 0; i < renderpass.config.attachment_count; ++i)
                if (renderpass.config.attachment_configs[i].flags & (u8) Attachment::Flag::Clear) {
                    begin_info.clearValueCount = renderpass.config.attachment_count;

                    switch (renderpass.config.attachment_configs[i].type) {
                        case Attachment::Type::Color: {
                            clear_values[i].color.float32[0] = renderpass.config.clear_color.r;
                            clear_values[i].color.float32[1] = renderpass.config.clear_color.g;
                            clear_values[i].color.float32[2] = renderpass.config.clear_color.b;
                            clear_values[i].color.float32[3] = 1.0f;
                        }
                            break;
                        case Attachment::Type::Depth: {
                            clear_values[i].depthStencil.depth = renderpass.config.depth;
                            clear_values[i].depthStencil.stencil = 0;
                        }
                            break;
                        case Attachment::Type::Stencil: {
                            clear_values[i].depthStencil.depth = renderpass.config.depth;
                            clear_values[i].depthStencil.stencil = renderpass.config.stencil;
                        }
                    }
                }

            begin_info.pClearValues = begin_info.clearValueCount > 0 ? clear_values : nullptr;

            setViewportAndScissor(rect);

            vkCmdBeginRenderPass(handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
            state = State::InRenderPass;

            Color color{0.0f, 0.5f, 0.3f};
            VK_BEGIN_DEBUG_LABEL(handle, renderpass.config.name, color);

            return true;
        }

        bool endRenderPass() {
            vkCmdEndRenderPass(handle);
            VK_END_DEBUG_LABEL(handle);

            state = State::Recording;

            return true;
        }

        void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) const {
            vkCmdDraw(handle, vertex_count, instance_count, first_vertex, first_instance);
        }
    };

    struct GraphicsCommandPool : CommandPool<GraphicsCommandBuffer> {
        void create(bool transient = false) {
            _create(graphics_queue_family_index, transient);
            SLIM_LOG_INFO("Graphics command pool created.")
        }
    };

    struct GraphicsCommandPool graphics_command_pool;
    struct GraphicsCommandPool transient_graphics_command_pool;
    struct GraphicsCommandBuffer *graphics_command_buffer = nullptr;
    struct GraphicsCommandBuffer transient_graphics_command_buffer;

    struct GraphicsBuffer : Buffer {
        bool resize(u64 new_size) {
            return Buffer::resize(new_size, transient_graphics_command_buffer);
        }

        bool download(void **to, u64 size = 0, u64 from_offset = 0) const {
            return Buffer::download(to, transient_graphics_command_buffer, size, from_offset);
        }

        bool upload(const void *from, u64 size = 0, u64 to_offset = 0) const {
            return Buffer::upload(from, transient_graphics_command_buffer, size, to_offset);
        }

        bool copyTo(const Buffer &to_buffer, u64 size, u64 from_offset, u64 to_offset) const {
            return Buffer::copyTo(to_buffer, transient_graphics_command_buffer, size, from_offset, to_offset);
        }
    };

//    struct GraphicsImage : GPUImage {
//        void copyFromBuffer(const Buffer &from_buffer, u32 x = -1, u32 y = -1) const {
//            GPUImage::copyFromBuffer(from_buffer, transient_graphics_command_buffer, x, y);
//        }
//
//        void copyToBuffer(const Buffer &to_buffer, u32 x = -1, u32 y = -1) const {
//            GPUImage::copyToBuffer(to_buffer, transient_graphics_command_buffer, x, y);
//        }
//    };

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

            bool result = Buffer::create(BufferType::Index, (u64) count * index_size);
            return result;
        }

        void bind(const GraphicsCommandBuffer &command_buffer, u32 offset = 0) const {
            vkCmdBindIndexBuffer(command_buffer.handle, handle, offset, index_type);
        }

        void draw(const GraphicsCommandBuffer &command_buffer, u32 instance_count = 1, u32 first_index = 0, i32 first_vertex = 0, u32 first_instance = 0) const {
            vkCmdDrawIndexed(command_buffer.handle, index_count, instance_count, first_index, first_vertex, first_instance);
        }
    };

    struct VertexBuffer : GraphicsBuffer {
        u32 count = 0;

        bool create(u32 vertex_count, u8 vertex_size) {
            *this = {};
            count = vertex_count;
            return Buffer::create(BufferType::Vertex, (u64) count * vertex_size);
        }

        void bind(const GraphicsCommandBuffer &command_buffer, u32 offset = 0) const {
            VkDeviceSize offsets[1] = {offset};
            vkCmdBindVertexBuffers(command_buffer.handle, 0, 1, &handle, (VkDeviceSize *) offsets);
        }

        void draw(const GraphicsCommandBuffer &command_buffer, u32 vertex_count = 0, u32 first_vertex = 0, u32 instance_count = 1, u32 first_instance = 0) const {
            if (vertex_count == 0) vertex_count = count;
            vkCmdDraw(command_buffer.handle, vertex_count, instance_count, first_vertex, first_instance);
        }
    };

    struct UniformBuffer : GraphicsBuffer {
        bool create(u64 size) {
            *this = {};
            return Buffer::create(BufferType::Uniform, size);
        }

        void writeDescriptor(VkDescriptorSet descriptor_set, u32 binding_index) const {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = handle;
            bufferInfo.offset = 0;
            bufferInfo.range = total_size;

            VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrite.dstSet = descriptor_set;
            descriptorWrite.dstBinding = binding_index;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    };
}