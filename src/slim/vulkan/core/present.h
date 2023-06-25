#pragma once

#include "./image.h"

namespace gpu {
    namespace present {
        void SwapchainFrame::regenerateFrameBuffer(u32 width, u32 height) {
            if (framebuffer.handle) framebuffer.destroy();
            VkImageView image_views[2] = {image.view, depth_image.view};
            framebuffer.create(width, height, main_render_pass.handle, image_views, 1);
        }

        void SwapchainFrame::create(u32 width, u32 height, VkImage image_handle, u32 index) {
            char name[32];
            char *src = (char*)"swapchain_image_0";
            char *dst = name;
            while (*src) *dst++ = *src++;
            *(--dst) = (char)((u8)'0' + index);

            image.handle = image_handle;
            image.width = width;
            image.height = height;
            image.name = name;
            image.createView(VK_IMAGE_VIEW_TYPE_2D, image_format.format, VK_IMAGE_ASPECT_COLOR_BIT);

            src = (char*)"swapchain_depth_image_0";
            dst = name;
            while (*src) *dst++ = *src++;
            *(--dst) = (char)((u8)'0' + index);

            // Create depth image and its view.
            depth_image.create(VK_IMAGE_VIEW_TYPE_2D, width, height, depth_format,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               true,
                               VK_IMAGE_ASPECT_DEPTH_BIT,
                               name);

            regenerateFrameBuffer(width, height);

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



//            if (!depth_textures) {
//                depth_textures = (texture*)kallocate(sizeof(texture) * image_count, MEMORY_TAG_RENDERER);
//            }


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
        }

        void SwapchainFrame::destroy() {
            vkDestroyImageView(device, image.view, nullptr);
            depth_image.destroy();
            framebuffer.destroy();
        }

        void regenerateFrameBuffers(u32 width, u32 height) {
            for (u32 i = 0; i < image_count; ++i)
                swapchain_frames[i].regenerateFrameBuffer(width, height);
        }

        void createSwapchain(u32 width, u32 height, bool turn_vsync_on, bool turn_power_saving_on) {
            vsync = turn_vsync_on;
            power_saving = turn_power_saving_on;

            // FIFO and MAILBOX support vsync, IMMEDIATE does not.
            // TODO: vsync seems to hold up the game update for some reason.
            // It theoretically should be post-update and pre-render where that happens.
            if (vsync) {
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                // Only try for mailbox mode if not in power-saving mode.
                if (!power_saving) {
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

            // Swapchain extent
            VkExtent2D swapchain_extent = {width, height};
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
            if (graphics_queue_family_index != present::queue_family_index) {
                const unsigned int queueFamilyIndices[] = {
                    graphics_queue_family_index,
                    present::queue_family_index
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
            swapchain_create_info.oldSwapchain = nullptr;

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain))

            // Start with a zero frame index.
            current_frame = 0;

            // Images
            image_count = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr))

            VkImage swapchain_images[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images));

            for (u32 i = 0; i < image_count; ++i)
                swapchain_frames[i].create(swapchain_extent.width,  swapchain_extent.height, swapchain_images[i], i);

            SLIM_LOG_INFO("Swap chain created");
        }

        void destroySwapchain() {
            vkDeviceWaitIdle(device);

            for (u32 i = 0; i < image_count; ++i)
                swapchain_frames[i].destroy();

            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        void _recreateSwapchain() {
            destroySwapchain();
            createSwapchain(framebuffer_width, framebuffer_height, vsync, power_saving);
        }

        bool recreateSwapchain() {
            // If already being recreated, do not try again.
            if (recreating_swapchain) {
                SLIM_LOG_DEBUG("recreate_swapchain called when already recreating. Booting.");
                return false;
            }

            // Detect if the window is too small to be drawn to
            if (framebuffer_width == 0 || framebuffer_height == 0) {
                SLIM_LOG_DEBUG("recreate_swapchain called when window is < 1 in a dimension. Booting.");
                return false;
            }

            // Clear these out just in case.
            for (u32 i = 0; i < image_count; ++i) {
                images_in_flight[i] = nullptr;
                graphics_command_pool.allocate(graphics_command_buffers[i],true);
            }

            // Mark as recreating if the dimensions are valid.
            recreating_swapchain = true;

            // Wait for any operations to complete.
            vkDeviceWaitIdle(device);

            // Requery support
            _device::querySupportForSurfaceFormatsAndPresentModes();
            _device::querySupportForDepthFormats();

            _recreateSwapchain();

            // Update framebuffer size generation.
            framebuffer_size_last_generation = framebuffer_size_generation;

            // Indicate to listeners that a render target refresh is required.
//            event_context event_context = {0};
//            event_fire(EVENT_CODE_DEFAULT_RENDERTARGET_REFRESH_REQUIRED, 0, event_context);

            // Clear the recreating flag.
            recreating_swapchain = false;

            return true;
        }

        bool acquireNextImageIndex(u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, unsigned int *out_image_index) {
            VkResult result = vkAcquireNextImageKHR(device, swapchain, timeout_ns, image_available_semaphore, fence, out_image_index);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // Trigger swapchain recreation, then boot out of the render loop.
                _recreateSwapchain();
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

            VkResult result = vkQueuePresentKHR(present::queue, &present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
                _recreateSwapchain();
                SLIM_LOG_DEBUG("Swapchain recreated because swapchain returned out of date or suboptimal.");
            } else if (result != VK_SUCCESS) {
                SLIM_LOG_FATAL("Failed to present swap chain image!");
            }

            // Increment (and loop) the index.
            current_frame = (current_frame + 1) % max_frames_in_flight;
        }
    }
}