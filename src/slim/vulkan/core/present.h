#pragma once

#include "./image.h"
#include "./framebuffer.h"
#include "./graphics.h"
#include "./render_pass.h"


namespace gpu {
    namespace present {
        VkSwapchainKHR swapchain;
        RenderPass render_pass;

        bool vsync = true;
        bool power_saving = false;

        VkRect2D swapchain_rect{{}, {DEFAULT_WIDTH, DEFAULT_HEIGHT}};

        u64 framebuffer_size_generation = 0; // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation, a new one should be generated
        u64 framebuffer_size_last_generation = 0; // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation when updated

        VkPresentModeKHR mode;

        // The maximum number of "images in flight" (images simultaneously being rendered to).
        // Typically one less than the total number of images available.
        u8 max_frames_in_flight = VULKAN_MAX_FRAMES_IN_FLIGHT;
        VkFence in_flight_fences[VULKAN_MAX_FRAMES_IN_FLIGHT];
        VkFence images_in_flight[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];

        VkSemaphore image_available_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHT];
        VkSemaphore render_complete_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHT];

        unsigned int image_count = 0; // The number of swapchain images

//        texture* render_textures; // An array of render targets, which contain swapchain images
//        texture* depth_textures; // An array of depth textures, one per frame

        //Render targets used for on-screen rendering, one per frame. The images contained in these are created and owned by the swapchain
//        render_target render_targets[3];
//        render_target world_render_targets[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Render targets used for world rendering. One per frame

        u32 current_frame = 0;
        bool recreating_swapchain = false;
        bool render_flag_changed = false;

        unsigned int current_image_index = 0;   // The current image index

        struct SwapchainFrame {
            Image image{};
            Image depth_image{};
            FrameBuffer framebuffer{};

            void regenerateFrameBuffer(u32 width, u32 height) {
                framebuffer.destroy();
                VkImageView image_views[2] = {image.view, depth_image.view};
                framebuffer.create(width, height, render_pass.handle, image_views, 2);
            }

            void create(u32 width, u32 height, VkImage image_handle, u32 index) {
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

            void destroy() {
                vkDestroyImageView(device, image.view, nullptr);
                depth_image.destroy();
                framebuffer.destroy();
            }
        };

        SwapchainFrame swapchain_frames[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];
        GraphicsCommandBuffer _graphics_command_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];

        void createSwapchain(u32 width, u32 height, bool turn_vsync_on, bool turn_power_saving_on) {
            vsync = turn_vsync_on;
            power_saving = turn_power_saving_on;

            if (vsync) {
                mode = VK_PRESENT_MODE_FIFO_KHR;
                if (!power_saving)
                    for (u32 i = 0; i < _device::present_mode_count; ++i)
                        if (_device::present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                            mode = _device::present_modes[i];
                            break;
                        }
            } else
                mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

            swapchain_rect = {{}, {width, height}};
            if (_device::surface_capabilities.currentExtent.width != UINT32_MAX)
                swapchain_rect.extent = _device::surface_capabilities.currentExtent;

            // Clamp to the value allowed by the GPU.
            VkExtent2D min = _device::surface_capabilities.minImageExtent;
            VkExtent2D max = _device::surface_capabilities.maxImageExtent;
            swapchain_rect.extent.width = clampedValue(swapchain_rect.extent.width, min.width, max.width);
            swapchain_rect.extent.height = clampedValue(swapchain_rect.extent.height, min.height, max.height);

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
            swapchain_create_info.imageExtent = swapchain_rect.extent;
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
            swapchain_create_info.presentMode = mode;
            swapchain_create_info.clipped = VK_TRUE;
            swapchain_create_info.oldSwapchain = nullptr;

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain))

            current_frame = 0;

            image_count = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr))

            VkImage swapchain_images[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];
            VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images));

            for (u32 i = 0; i < image_count; ++i)
                swapchain_frames[i].create(swapchain_rect.extent.width,  swapchain_rect.extent.height, swapchain_images[i], i);

            SLIM_LOG_INFO("Swap chain created");
        }

        void destroySwapchain() {
            vkDeviceWaitIdle(device);

            for (u32 i = 0; i < image_count; ++i)
                swapchain_frames[i].destroy();

            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        bool recreateSwapchain() {
            if (recreating_swapchain) {
                SLIM_LOG_DEBUG("recreate_swapchain called when already recreating. Booting.");
                return false;
            }

            for (u32 i = 0; i < image_count; ++i) images_in_flight[i] = nullptr;

            recreating_swapchain = true;
            vkDeviceWaitIdle(device);

            _device::querySupportForSurfaceFormatsAndPresentModes();
            _device::querySupportForDepthFormats();

            destroySwapchain();
            createSwapchain(swapchain_rect.extent.width, swapchain_rect.extent.height, vsync, power_saving);
            framebuffer_size_last_generation = framebuffer_size_generation;
            recreating_swapchain = false;

            return true;
        }

        bool acquireNextImageIndex(u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, unsigned int *out_image_index) {
            VkResult result = vkAcquireNextImageKHR(device, swapchain, timeout_ns, image_available_semaphore, fence, out_image_index);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapchain();
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
                recreateSwapchain();
                SLIM_LOG_DEBUG("Swapchain recreated because swapchain returned out of date or suboptimal.");
            } else if (result != VK_SUCCESS) {
                SLIM_LOG_FATAL("Failed to present swap chain image!");
            }

            current_frame = (current_frame + 1) % max_frames_in_flight;
        }

        struct PresentCommandBuffer : CommandBuffer {
            PresentCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
                CommandBuffer(command_buffer_handle, command_pool, present_queue, present_queue_family_index) {}
        };

        struct PresentCommandPool : CommandPool<PresentCommandBuffer> {
            void create(bool transient = false) {
                _create(present_queue_family_index, transient);
                SLIM_LOG_INFO("Present command pool created.")
            }
        };

        PresentCommandPool command_pool;

        void resize(u32 width, u32 height) {
            swapchain_rect.extent.width = width;
            swapchain_rect.extent.height = height;
            framebuffer_size_generation++;
//            vkDeviceWaitIdle(device);
            recreateSwapchain();
        }

    }
}