#pragma once

#include "./base.h"

namespace gpu {
    namespace _swapchain {
        void create(u32 width, u32 height) {
            VkExtent2D swapchain_extent = {width, height};

            // Choose a swap surface format.
            bool found = false;
            for (u32 i = 0; i < surface_format_count; ++i) {
                VkSurfaceFormatKHR format = surface_formats[i];
                // Preferred formats
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    image_format = format;
                    found = true;
                    break;
                }
            }

            if (!found) image_format = surface_formats[0];

            // FIFO and MAILBOX support vsync, IMMEDIATE does not.
            // TODO: vsync seems to hold up the game update for some reason.
            // It theoretically should be post-update and pre-render where that happens.
            swapchain->flags = flags;

            VkPresentModeKHR present_mode;
            if (flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) {
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                // Only try for mailbox mode if not in power-saving mode.
                if ((flags & RENDERER_CONFIG_FLAG_POWER_SAVING_BIT) == 0) {
                    for (u32 i = 0; i < present_mode_count; ++i) {
                        VkPresentModeKHR mode = present_modes[i];
                        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                            present_mode = mode;
                            break;
                        }
                    }
                }
            } else
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

            // Swapchain extent
            if (surface_capabilities.currentExtent.width != UINT32_MAX)
                swapchain_extent = surface_capabilities.currentExtent;

            // Clamp to the value allowed by the GPU.
            VkExtent2D min = surface_capabilities.minImageExtent;
            VkExtent2D max = surface_capabilities.maxImageExtent;
            swapchain_extent.width = clampedValue(swapchain_extent.width, min.width, max.width);
            swapchain_extent.height = clampedValue(swapchain_extent.height, min.height, max.height);

            u32 image_count = surface_capabilities.minImageCount + 1;
            if (surface_capabilities.maxImageCount > 0 &&
                image_count > surface_capabilities.maxImageCount) {
                image_count = surface_capabilities.maxImageCount;
            }

            max_frames_in_flight = image_count - 1;

            // Swapchain create info
            VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
            swapchain_create_info.surface = surface;
            swapchain_create_info.minImageCount = image_count;
            swapchain_create_info.imageFormat = image_format.format;
            swapchain_create_info.imageColorSpace = image_format.colorSpace;
            swapchain_create_info.imageExtent = swapchain_extent;
            swapchain_create_info.imageArrayLayers = 1;
            swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            // Setup the queue family indices
            if (graphics_queue_family_index != present_queue_family_index) {
                const unsigned int queueFamilyIndices[] = {
                    graphics_queue_family_index,
                    present_queue_family_index
                };
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchain_create_info.queueFamilyIndexCount = 2;
                swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchain_create_info.queueFamilyIndexCount = 0;
                swapchain_create_info.pQueueFamilyIndices = 0;
            }

            swapchain_create_info.preTransform = surface::capabilities.currentTransform;
            swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchain_create_info.presentMode = present_mode;
            swapchain_create_info.clipped = VK_TRUE;
            swapchain_create_info.oldSwapchain = 0;

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, 0, &handle));

            // Start with a zero frame index.
            current_frame = 0;

            // Images
            image_count = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, handle, &image_count, 0));
            if (!render_textures) {
                render_textures = (texture*)kallocate(sizeof(texture) * swapchain->image_count, MEMORY_TAG_RENDERER);
                // If creating the array, then the internal texture objects aren't created yet either.
                for (u32 i = 0; i < image_count; ++i) {
                    void* internal_data = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);

                    char tex_name[38] = "__internal_vulkan_swap chain_image_0__";
                    tex_name[34] = '0' + (char)i;

                    texture_system_wrap_internal(tex_name, swapchain_extent.width, swapchain_extent.height, 4,
                        false, true, false, internal_data, &render_textures[i]);
                    if (!render_textures[i].internal_data) {
                        SLIM_LOG_FATAL("Failed to generate new swap chain image texture!");
                        return;
                    }
                }
            } else {
                for (u32 i = 0; i < image_count; ++i) {
                    // Just update the dimensions.
                    texture_system_resize(&render_textures[i], swapchain_extent.width, swapchain_extent.height, false);
                }
            }
            VkImage swapchain_images[32];
            VK_CHECK(vkGetSwapchainImagesKHR(device, handle, &image_count, swapchain_images));
            for (u32 i = 0; i < image_count; ++i) {
                // Update the internal image for each.
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
                image->handle = swapchain_images[i];
                image->width = swapchain_extent.width;
                image->height = swapchain_extent.height;
            }

            // Views
            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;

                VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                view_info.image = image->handle;
                view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view_info.format = image_format.format;
                view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view_info.subresourceRange.baseMipLevel = 0;
                view_info.subresourceRange.levelCount = 1;
                view_info.subresourceRange.baseArrayLayer = 0;
                view_info.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(device, &view_info, 0, &image->view));
            }

            if (!depth_textures) {
                depth_textures = (texture*)kallocate(sizeof(texture) * image_count, MEMORY_TAG_RENDERER);
            }

            for (u32 i = 0; i < image_count; ++i) {
                // Create depth image and its view.
                char formatted_name[TEXTURE_NAME_MAX_LENGTH] = {0};
                string_format(formatted_name, "swapchain_image_%u", i);

                vulkan_image* image = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);
                vulkan_image_create(
                    TEXTURE_TYPE_2D,
                    swapchain_extent.width,
                    swapchain_extent.height,
                    device.depth_format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    true,
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    formatted_name,
                    image);

                // Wrap it in a texture.
                texture_system_wrap_internal(
                    "__kohi_default_depth_texture__",
                    swapchain_extent.width,
                    swapchain_extent.height,
                    depth_channel_count,
                    false,
                    true,
                    false,
                    image,
                    &depth_textures[i]);
            }

            SLIM_LOG_INFO("Swap chain created");
        }

        void destroy() {
            vkDeviceWaitIdle(device);

            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image_destroy((vulkan_image*)swapchain->depth_textures[i].internal_data);
                depth_textures[i].internal_data = 0;
            }

            // Only destroy the views, not the images, since those are owned by the swapchain and are thus
            // destroyed when it is.
            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
                vkDestroyImageView(device, image->view, 0);
            }

            vkDestroySwapchainKHR(device, handle, 0);
        }

        void recreate(u32 width, u32 height) {
            destroy();
            create(width, height);
        }

        bool acquireNextImageIndex(u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, u32* out_image_index) {
            VkResult result = vkAcquireNextImageKHR(device, handle, timeout_ns, image_available_semaphore, fence, out_image_index);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // Trigger swapchain recreation, then boot out of the render loop.
                recreate(framebuffer_width, framebuffer_height);

                return false;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                SLIM_LOG_FATAL("Failed to acquire swapchain image!");
                return false;
            }

            return true;
        }

        void present(VkSemaphore render_complete_semaphore, u32 present_image_index) {
            // Return the image to the swapchain for presentation.
            VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &render_complete_semaphore;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &handle;
            present_info.pImageIndices = &present_image_index;
            present_info.pResults = 0;

            VkResult result = vkQueuePresentKHR(present_queue, &present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
                recreate(framebuffer_width, framebuffer_height);
                SLIM_LOG_DEBUG("Swapchain recreated because swapchain returned out of date or suboptimal.");
            } else if (result != VK_SUCCESS) {
                SLIM_LOG_FATAL("Failed to present swap chain image!");
            }

            // Increment (and loop) the index.
            current_frame = (current_frame + 1) % max_frames_in_flight;
        }
    }
}