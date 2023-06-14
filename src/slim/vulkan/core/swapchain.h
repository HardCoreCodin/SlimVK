#pragma once

#include "./image.h"

namespace gpu {
    namespace _swapchain {
        char* _image_names[VULKAN_MAX_FRAMES_IN_FLIGHTS] = {
            (char*)"swapchain_image_1",
            (char*)"swapchain_image_2",
            (char*)"swapchain_image_3"
        };
        char* _depth_image_names[VULKAN_MAX_FRAMES_IN_FLIGHTS] = {
            (char*)"swapchain_depth_image_1",
            (char*)"swapchain_depth_image_2",
            (char*)"swapchain_depth_image_3"
        };

        void create(u32 width, u32 height, SwapchainConfig desired_config) {
            VkExtent2D swapchain_extent = {width, height};

            // Choose a swap surface format.
            bool found = false;
            for (u32 i = 0; i < _device::surface_format_count; ++i) {
                VkSurfaceFormatKHR format = _device::surface_formats[i];
                // Preferred formats
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    image_format = format;
                    found = true;
                    break;
                }
            }

            if (!found) image_format = _device::surface_formats[0];

            // FIFO and MAILBOX support vsync, IMMEDIATE does not.
            // TODO: vsync seems to hold up the game update for some reason.
            // It theoretically should be post-update and pre-render where that happens.
            config = desired_config;

            if (config & SwapchainConfig_VSync) {
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                // Only try for mailbox mode if not in power-saving mode.
                if ((config & SwapchainConfig_PowerSaving) == 0) {
                    for (u32 i = 0; i < _device::present_mode_count; ++i) {
                        VkPresentModeKHR mode = _device::present_modes[i];
                        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                            present_mode = mode;
                            break;
                        }
                    }
                }
            } else
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

            _device::querySupportForSurfaceFormatsAndPresentModes();

            // Swapchain extent
            if (_device::surface_capabilities.currentExtent.width != UINT32_MAX)
                swapchain_extent = _device::surface_capabilities.currentExtent;

            // Clamp to the value allowed by the GPU.
            VkExtent2D min = _device::surface_capabilities.minImageExtent;
            VkExtent2D max = _device::surface_capabilities.maxImageExtent;
            swapchain_extent.width = clampedValue(swapchain_extent.width, min.width, max.width);
            swapchain_extent.height = clampedValue(swapchain_extent.height, min.height, max.height);

            unsigned int min_image_count = _device::surface_capabilities.minImageCount + 1;
            if (_device::surface_capabilities.maxImageCount > 0 &&
                min_image_count > _device::surface_capabilities.maxImageCount) {
                min_image_count = _device::surface_capabilities.maxImageCount;
            }

            max_frames_in_flight = min_image_count - 1;

            // Swapchain create info
            VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
            swapchain_create_info.surface = _device::surface;
            swapchain_create_info.minImageCount = min_image_count;
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
                swapchain_create_info.pQueueFamilyIndices = nullptr;
            }

            swapchain_create_info.preTransform = _device::surface_capabilities.currentTransform;
            swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchain_create_info.presentMode = present_mode;
            swapchain_create_info.clipped = VK_TRUE;
            swapchain_create_info.oldSwapchain = 0;

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain))

            // Start with a zero frame index.
            current_frame = 0;

            // Images
            image_count = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr))
//            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, images))

//            // Views
//            for (u32 i = 0; i < image_count; ++i) {
//                VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
//                view_info.image = images[i].handle;
//                view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
//                view_info.format = image_format.format;
//                view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//                view_info.subresourceRange.baseMipLevel = 0;
//                view_info.subresourceRange.levelCount = 1;
//                view_info.subresourceRange.baseArrayLayer = 0;
//                view_info.subresourceRange.layerCount = 1;
//
//                VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &images[i].view))
//            }

//            if (!render_textures) {
//                render_textures = (texture*)kallocate(sizeof(texture) * swapchain->image_count, MEMORY_TAG_RENDERER);
//                // If creating the array, then the internal texture objects aren't created yet either.
//                for (u32 i = 0; i < image_count; ++i) {
//                    void* internal_data = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);
//
//                    char tex_name[38] = "__internal_vulkan_swap chain_image_0__";
//                    tex_name[34] = '0' + (char)i;
//
//                    texture_system_wrap_internal(tex_name, swapchain_extent.width, swapchain_extent.height, 4,
//                        false, true, false, internal_data, &render_textures[i]);
//                    if (!render_textures[i].internal_data) {
//                        SLIM_LOG_FATAL("Failed to generate new swap chain image texture!");
//                        return;
//                    }
//                }
//            } else {
//                for (u32 i = 0; i < image_count; ++i) {
//                    // Just update the dimensions.
//                    texture_system_resize(&render_textures[i], swapchain_extent.width, swapchain_extent.height, false);
//                }
//            }

            VkImage swapchain_images[VULKAN_MAX_FRAMES_IN_FLIGHTS];
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images));
            for (u32 i = 0; i < image_count; ++i) {
//                // Update the internal image for each.
//                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
                images[i].handle = swapchain_images[i];
                images[i].width = swapchain_extent.width;
                images[i].height = swapchain_extent.height;
                images[i].name = _image_names[i];
            }

            // Views
            for (u32 i = 0; i < image_count; ++i) {
//                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;

                VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                view_info.image = images[i].handle;
                view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view_info.format = image_format.format;
                view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view_info.subresourceRange.baseMipLevel = 0;
                view_info.subresourceRange.levelCount = 1;
                view_info.subresourceRange.baseArrayLayer = 0;
                view_info.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &images[i].view));
            }

            // Depth resources
            if (!_device::querySupportForDepthFormats()) {
                _device::depth_format = VK_FORMAT_UNDEFINED;
                SLIM_LOG_FATAL("Failed to find a supported format!")
            }

//            if (!depth_textures) {
//                depth_textures = (texture*)kallocate(sizeof(texture) * image_count, MEMORY_TAG_RENDERER);
//            }

            for (u32 i = 0; i < image_count; ++i) {
                // Create depth image and its view.
//                char formatted_name[TEXTURE_NAME_MAX_LENGTH] = {0};
//                string_format(formatted_name, "swapchain_image_%u", i);

//                vulkan_image* image = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);
                depth_images[i].create(
                    TextureType_2D,
                    swapchain_extent.width,
                    swapchain_extent.height,
                    _device::depth_format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    true,
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    _depth_image_names[i]);

//                // Wrap it in a texture.
//                texture_system_wrap_internal(
//                    "__kohi_default_depth_texture__",
//                    swapchain_extent.width,
//                    swapchain_extent.height,
//                    depth_channel_count,
//                    false,
//                    true,
//                    false,
//                    image,
//                    &depth_textures[i]);
            }

            SLIM_LOG_INFO("Swap chain created");
        }

        void destroy() {
            vkDeviceWaitIdle(device);

            for (u32 i = 0; i < image_count; ++i) depth_images[i].destroy();
//            {
//                vulkan_image_destroy((vulkan_image*)swapchain->depth_textures[i].internal_data);
//                depth_textures[i].internal_data = 0;
//            }

            // Only destroy the views, not the images, since those are owned by the swapchain and are thus
            // destroyed when it is.
            for (u32 i = 0; i < image_count; ++i) vkDestroyImageView(device, images[i].view, nullptr);
//            {
//                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
//                vkDestroyImageView(device, image->view, nullptr);
//            }

            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        void recreate(u32 width, u32 height) {
            destroy();
            create(width, height, config);
        }

        bool acquireNextImageIndex(u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, unsigned int *out_image_index) {
            VkResult result = vkAcquireNextImageKHR(device, nullptr, timeout_ns, image_available_semaphore, fence, out_image_index);
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

        void present(VkSemaphore render_complete_semaphore, unsigned int present_image_index) {
            // Return the image to the swapchain for presentation.
            VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &render_complete_semaphore;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &swapchain;
            present_info.pImageIndices = &present_image_index;
            present_info.pResults = nullptr;

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