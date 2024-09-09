#pragma once

#include "./base.h"
#include "./buffer.h"
#include "./command.h"

namespace gpu {
    struct GPUImage {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
        VkMemoryRequirements memory_requirements;
        VkMemoryPropertyFlags memory_flags;
        u32 width;
        u32 height;
        u32 mip_count;
        char* name;
        VkImageViewType type;

        void destroy() {
            if (sampler) {
                vkDestroySampler(device, sampler, nullptr);
                sampler = nullptr;
            }
            if (view) {
                vkDestroyImageView(device, view, nullptr);
                view = nullptr;
            }
            if (memory) {
                vkFreeMemory(device, memory, nullptr);
                memory = nullptr;
            }
            if (handle) {
                vkDestroyImage(device, handle, nullptr);
                handle = nullptr;
            }
        }

        void create(u32 image_width,
                    u32 image_height,
                    VkFormat format,
                    const char* image_name,
                    VkImageUsageFlags usage,
                    VkImageAspectFlags view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
                    bool mip_map = true,
                    bool create_view = true,
                    bool create_sampler = true,
                    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
                    VkMemoryPropertyFlags image_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D) {

            // Copy params
            width = image_width;
            height = image_height;
            memory_flags = image_memory_flags;
            name = (char*)image_name;
            type = view_type;
            mip_count = 1;
            if (mip_map)
                mip_count += static_cast<uint32_t>(floor(log2(Max(width, height))));

            // Creation info.
            VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

            image_create_info.imageType = VK_IMAGE_TYPE_2D;
            image_create_info.extent.width = width;
            image_create_info.extent.height = height;
            image_create_info.extent.depth = 1;                                 // TODO: Support configurable depth.
            image_create_info.mipLevels = mip_count;
            image_create_info.arrayLayers = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;  // TODO: Support number of layers in the image.
            image_create_info.format = format;
            image_create_info.tiling = tiling;
            image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_create_info.usage = usage;
            image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Configurable sample count.
            image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Configurable sharing mode.
            if (type == VK_IMAGE_VIEW_TYPE_CUBE) {
                image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }

            VK_CHECK(vkCreateImage(device, &image_create_info, nullptr, &handle));
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, handle, name);

            // Query memory requirements.
            vkGetImageMemoryRequirements(device, handle, &memory_requirements);
            i32 memory_type = _device::getMemoryIndex(memory_requirements.memoryTypeBits, memory_flags);
            if (memory_type == -1) SLIM_LOG_ERROR("Required memory type not found. Image not valid.");

            // Allocate memory
            VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            memory_allocate_info.allocationSize = memory_requirements.size;
            memory_allocate_info.memoryTypeIndex = memory_type;
            VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory));

            // Bind the memory
            VK_CHECK(vkBindImageMemory(device, handle, memory, 0))  // TODO: configurable memory offset.

            // Create view
            if (create_view) {
                view = nullptr;
                createView(type, format, view_aspect_flags);
            }
            // Create Sampler
            if (create_sampler) {
                sampler = nullptr;
                createSampler();
            }
        }

        void createView(VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags) {
            VkImageViewCreateInfo view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view_create_info.image = handle;
            view_create_info.viewType = view_type;
            view_create_info.subresourceRange.layerCount = view_type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
            view_create_info.format = format;
            view_create_info.subresourceRange.aspectMask = aspect_flags;
            view_create_info.subresourceRange.levelCount = mip_count;
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.baseArrayLayer = 0;

            VK_CHECK(vkCreateImageView(device, &view_create_info, nullptr, &view))

            static char formatted_name[TEXTURE_NAME_MAX_LENGTH + 5] = {0};
            int i = 0;
            while (name[i]) {
                formatted_name[i] = name[i];
                i++;
            }
            formatted_name[i++] = '_';
            formatted_name[i++] = 'v';
            formatted_name[i++] = 'i';
            formatted_name[i++] = 'e';
            formatted_name[i++] = 'w';
            formatted_name[i] = '\0';
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, view, formatted_name);
        }

        void createSampler(bool mip_map = true) {
            VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = _device::properties.limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            if (mip_map) samplerInfo.maxLod = (float)mip_count;
//            samplerInfo.minLod = 0.0f;
//            samplerInfo.mipLodBias = 0.0f;

            VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &sampler))
        }

        void transitionLayout(VkImageLayout old_layout,
                              VkImageLayout new_layout,
                              VkPipelineStageFlags source_stage,
                              VkPipelineStageFlags dest_stage,
                              VkAccessFlags source_access_mask,
                              VkAccessFlags dest_access_mask,
                              const CommandBuffer &command_buffer,
                              u32 base_mip_level = 0,
                              u32 mip_level_count = 1) const {
            VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            barrier.oldLayout = old_layout;
            barrier.newLayout = new_layout;
            barrier.srcQueueFamilyIndex = command_buffer.queue_family_index;
            barrier.dstQueueFamilyIndex = command_buffer.queue_family_index;
            barrier.image = handle;
            barrier.srcAccessMask = source_access_mask;
            barrier.dstAccessMask = dest_access_mask;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = base_mip_level;
            barrier.subresourceRange.levelCount = mip_level_count;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            vkCmdPipelineBarrier(command_buffer.handle, source_stage, dest_stage, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }

        void transitionLayout(VkImageLayout old_layout, VkImageLayout new_layout, const CommandBuffer &command_buffer, bool mip_map = true) const {
            VkPipelineStageFlags source_stage, dest_stage;
            VkAccessFlags source_access_mask, dest_access_mask;

            // Transition from an initial layout into transfer layout for copying to/from:
            if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
                 new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // Don't care what stage the pipeline is at initially
                dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT; // Used for copying
                source_access_mask = 0; // Don't care about the old layout - transition to optimal layout
                dest_access_mask = new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ?
                        VK_ACCESS_TRANSFER_WRITE_BIT : // Transition into transfer dst so need to write to it
                        VK_ACCESS_TRANSFER_READ_BIT;   // Transition into transfer src so need to read from it
            } else if ( // Transition from transfer layout (completed copying to/from) into reading in fragment shader:
                new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && (
                old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
                old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
                source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT; // From a copying stage to...
                dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // The fragment stage
                source_access_mask = old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ?
                    VK_ACCESS_TRANSFER_READ_BIT : // Transitioning from a transfer src so was read from
                    VK_ACCESS_TRANSFER_WRITE_BIT; // Transitioning from a transfer dst so was written to
                dest_access_mask = VK_ACCESS_SHADER_READ_BIT; // Transition into reading from a shader
            } else {
                SLIM_LOG_FATAL("unsupported layout transition!");
                return;
            }
            transitionLayout(old_layout, new_layout, source_stage, dest_stage, source_access_mask, dest_access_mask, command_buffer, 0, mip_map ? mip_count : 1);
        }

        void copyFromBuffer(const Buffer &from_buffer, const CommandBuffer &command_buffer, u32 x = -1, u32 y = -1) const {
            VkBufferImageCopy region = getCopyRegion(x, y);
            vkCmdCopyBufferToImage(command_buffer.handle, from_buffer.handle, handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,&region);
        }

        void copyToBuffer(const Buffer &to_buffer, const CommandBuffer &command_buffer, u32 x = -1, u32 y = -1) const {
            VkBufferImageCopy region = getCopyRegion(x, y);
            vkCmdCopyImageToBuffer(command_buffer.handle, handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   to_buffer.handle, 1,&region);
        }
        
        static VkFormat imageFormat(ImageFlags flags) {
            return flags.normal ? (
                flags.alpha ? 
                VK_FORMAT_B8G8R8A8_UNORM : 
                VK_FORMAT_B8G8R8_UNORM
                ) : (
                    flags.alpha ? 
                    VK_FORMAT_B8G8R8A8_SRGB : 
                    VK_FORMAT_B8G8R8_SRGB
                    );
        }

        bool createTexture(
            const u8 *data, 
            u32 texel_size, 
            u32 image_width, 
            u32 image_height,
            const char *texture_name,
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, 
            VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D,
            VkImageAspectFlags view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags image_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            CommandBuffer *command_buffer = nullptr ,
            bool mip_map = true,
            bool create_view = true,
            bool create_sampler = true
        ) {
            if (!command_buffer) command_buffer = &transient_graphics_command_buffer;

            Buffer staging{};
            if (!staging.createAsStaging(texel_size * image_width * image_height * (view_type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1), data))
                return false;

            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (mip_map)
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            create(
                image_width, 
                image_height, 
                format, 
                texture_name, 
                usage,
                view_aspect_flags,
                mip_map,
                create_view,
                create_sampler,
                tiling,
                image_memory_flags,
                view_type
            );

            command_buffer->beginSingleUse();
            transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, *command_buffer);
            copyFromBuffer(staging, *command_buffer);
            if (mip_map) {
                // Check if image format supports linear blitting
//                VkFormatProperties formatProperties;
//                vkGetPhysicalDeviceFormatProperties(physical_device, format, &formatProperties);
//                if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
//                    throw std::runtime_error("texture image format does not support linear blitting!");
//                }




                i32 mipWidth = (i32)width;
                i32 mipHeight = (i32)height;
                for (u32 src_mip_level = 0, dst_mip_level = 1; dst_mip_level < mip_count; src_mip_level++, dst_mip_level++) {
                    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                                     *command_buffer, src_mip_level);

                    VkImageBlit blit{};
                    blit.srcOffsets[0] = {0, 0, 0};
                    blit.dstOffsets[0] = {0, 0, 0};
                    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blit.srcSubresource.baseArrayLayer = 0;
                    blit.dstSubresource.baseArrayLayer = 0;
                    blit.srcSubresource.layerCount = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
                    blit.dstSubresource.layerCount = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
                    blit.srcSubresource.mipLevel = src_mip_level;
                    blit.dstSubresource.mipLevel = dst_mip_level;

                    blit.srcOffsets[1] = {mipWidth,
                                          mipHeight,
                                          1};
                    blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
                                           mipHeight > 1 ? mipHeight / 2 : 1,
                                           1 };

                    vkCmdBlitImage(command_buffer->handle,
                                   handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &blit, VK_FILTER_LINEAR);

                    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                                     *command_buffer, blit.srcSubresource.mipLevel);

                    if (mipWidth > 1) mipWidth /= 2;
                    if (mipHeight > 1) mipHeight /= 2;
                }

                transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                 *command_buffer, mip_count - 1);
            } else
                transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, *command_buffer);

            command_buffer->endSingleUse();

            staging.destroy();

            return true;
        }

        bool createTexture(
            const RawImage &image, 
            const char *name = "",
            VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D,
            VkImageAspectFlags view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags image_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            CommandBuffer *command_buffer = nullptr,
            bool mip_map = true,
            bool create_view = true,
            bool create_sampler = true
        ) {
            u8 *content = image.content;
            if (!image.flags.alpha) {
                content = new u8[image.size * 4];
                u8 *src = image.content;
                u8 *trg = content;

                for (u32 i = 0; i < image.size; i++) {
                    *(trg++) = *(src++); 
                    *(trg++) = *(src++); 
                    *(trg++) = *(src++); 
                    *(trg++) = 255;
                }
            }

            const bool result = createTexture(
                content,
                4,
                image.width,
                image.height,
                name,
                image.flags.normal ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB,
                view_type,
                view_aspect_flags,
                tiling,
                image_memory_flags,
                command_buffer,
                mip_map,
                create_view,
                create_sampler
            );

            if (!image.flags.alpha) delete[] content;

            return result;
        }

        bool createCubeMap(
            const CubeMapImages &images,
            const char *name = "", 
            VkImageAspectFlags view_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            VkMemoryPropertyFlags image_memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            CommandBuffer *command_buffer = nullptr,
            bool mip_map = true,
            bool create_view = true,
            bool create_sampler = true
        ) {
            RawImage image = images.array[0];
            u32 src_channel_count = image.flags.alpha ? 4 : 3;
            u32 src_content_size = image.size * src_channel_count;
            u32 trg_content_size = image.size * 4;
            image.content = new u8[trg_content_size * 6];
            u8 *trg = image.content;
            for (u32 side = 0; side < 6; side++) {
                u8 *src;
                switch (side) {
                    case 0: src = images.pos_x.content; break;
                    case 1: src = images.neg_x.content; break;
                    case 2: src = images.pos_y.content; break;
                    case 3: src = images.neg_y.content; break;
                    case 4: src = images.pos_z.content; break;
                    case 5: src = images.neg_z.content; break;
                }

                for (u32 i = 0; i < image.size; i++) {
                    *(trg++) = *(src++); 
                    *(trg++) = *(src++); 
                    *(trg++) = *(src++); 
                    *(trg++) = 255;
                }
            }
            const bool result = createTexture(
                image.content,
                4,
                image.width,
                image.height,
                name,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_VIEW_TYPE_CUBE,
                view_aspect_flags,
                tiling,
                image_memory_flags,
                command_buffer,
                mip_map,
                create_view,
                create_sampler
            );

            delete[] image.content;
            return result;
        }

        void writeDescriptor(VkDescriptorSet descriptor_set, u32 binding_index) const {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = view;
            imageInfo.sampler = sampler;

            VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptorWrite.dstSet = descriptor_set;
            descriptorWrite.dstBinding = binding_index;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }

    private:
        VkBufferImageCopy getCopyRegion(u32 x = -1, u32 y = -1) const {
            VkBufferImageCopy region = {};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            region.imageExtent.width = x == -1 ? width : x;
            region.imageExtent.height = y == -1 ? height : y;
            region.imageExtent.depth = 1;

            return region;
        }
    };
}
