#pragma once

#include "./debug.h"
#include "./instance.h"
#include "./device.h"
#include "./present.h"
#include "./command.h"
#include "./transfer.h"
#include "./compute.h"
#include "./render_target.h"
#include "./render_pass.h"
#include "./shadow.h"
#include "./shader.h"
#include "./pipeline.h"
#include "./buffer.h"
#include "./graphics.h"


namespace gpu {
    bool initGPU(u32 width = DEFAULT_WIDTH, u32 height = DEFAULT_HEIGHT, const char* app_name = "SlimVK") {
        if (!_instance::init(app_name)) {
            SLIM_LOG_FATAL("Failed to initialize Vulkan instance!");
            return false;
        }

        if (!_debug::init()) {
            SLIM_LOG_FATAL("Failed to initialize Vulkan debugging!");
            return false;
        }

        SLIM_LOG_DEBUG("Creating Vulkan surface...");
        if (CreateVulkanSurface(instance, _device::surface) != VK_SUCCESS) {
            SLIM_LOG_FATAL("Failed to create Vulkan platform surface!");
            return false;
        }
        SLIM_LOG_DEBUG("Vulkan surface created.");

        if (!_device::init()) {
            SLIM_LOG_FATAL("Failed to initialize Vulkan device!");
            return false;
        }

        present::command_pool.create();
        if (_device::present_shares_graphics_queue)
            graphics_command_pool.handle = present::command_pool.handle;
        else
            graphics_command_pool.create();

        transient_graphics_command_pool.create(true);
        transient_graphics_command_pool.allocate(transient_graphics_command_buffer);

        present::swapchain_rect = {{}, {width, height}};

        _device::querySupportForSurfaceFormatsAndPresentModes();
        if (!_device::querySupportForDepthFormats()) {
            present::depth_format = VK_FORMAT_UNDEFINED;
            SLIM_LOG_FATAL("Failed to find a supported format!")
        }

        shadow_pass::create(2048, 2048);

        present::render_pass.create({
            "main_render_pass",
            Color{ 0.0f, 0.0f, 0.2f }, 1.0f, 0,
            2, {
                {Attachment::Type::Color,  (
                    (u8)Attachment::Flag::Clear |
                    (u8)Attachment::Flag::Store |
                    (u8)Attachment::Flag::Present
                )},
                { Attachment::Type::Depth ,  (u8)Attachment::Flag::Clear}
            }
        });

        present::createSwapchain(width, height, true, false);

        // Create sync objects.
        for (u8 i = 0; i < present::max_frames_in_flight; ++i) {
            VkSemaphoreCreateInfo semaphore_create_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present::image_available_semaphores[i]);
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present::render_complete_semaphores[i]);

            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
            // This will prevent the application from waiting indefinitely for the first frame to render since it
            // cannot be rendered until a frame is "rendered" before it.
            VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &present::in_flight_fences[i]))
        }

        // Create command buffers.
        // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
        // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
        // by this list.
        for (u32 i = 0; i < present::image_count; i++) present::images_in_flight[i] = nullptr;

        for (auto &command_buffer : present::_graphics_command_buffers)
            graphics_command_pool.allocate(command_buffer,true);

        SLIM_LOG_DEBUG("Vulkan command buffers created.");

        SLIM_LOG_INFO("Vulkan renderer initialized successfully.");
        return true;
    }

    void waitForGPU() {
        vkDeviceWaitIdle(device);
    }

    void shutdownGPU() {
        vkDeviceWaitIdle(device);

        // Command buffers
        for (u32 i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
            present::_graphics_command_buffers[i].free();

        // Sync objects
        for (u8 i = 0; i < present::max_frames_in_flight; ++i) {
            if (present::image_available_semaphores[i]) {
                vkDestroySemaphore(device, present::image_available_semaphores[i], nullptr);
                present::image_available_semaphores[i] = nullptr;
            }
            if (present::render_complete_semaphores[i]) {
                vkDestroySemaphore(device, present::render_complete_semaphores[i], nullptr);
                present::render_complete_semaphores[i] = nullptr;
            }
            vkDestroyFence(device, present::in_flight_fences[i], nullptr);
            present::in_flight_fences[i] = nullptr;
        }

        present::destroySwapchain();
        present::render_pass.destroy();

        shadow_pass::destroy();

        SLIM_LOG_INFO("Destroying command pools...");

        present::command_pool.destroy();
        if (!_device::present_shares_graphics_queue)
            graphics_command_pool.destroy();

        transient_graphics_command_pool.destroy();

        SLIM_LOG_DEBUG("Destroying Vulkan device...");
        _device::destroy();

        SLIM_LOG_DEBUG("Destroying Vulkan surface...");
        if (_device::surface) {
            vkDestroySurfaceKHR(instance, _device::surface, nullptr);
            _device::surface = nullptr;
        }

        _debug::shutdown();

        SLIM_LOG_DEBUG("Destroying Vulkan instance...");
        vkDestroyInstance(instance, nullptr);
    }

    void beginRenderPass() {
        if (graphics_command_buffer)
            graphics_command_buffer->beginRenderPass(present::render_pass, present::swapchain_frames[present::current_image_index].framebuffer, present::swapchain_rect);
        else SLIM_LOG_WARNING("beginRenderPass: No command buffer")
    }

    void endRenderPass() {
        if (graphics_command_buffer) graphics_command_buffer->endRenderPass();
        else SLIM_LOG_WARNING("endRenderPass: No command buffer")
    }

    bool beginFrame() {
        graphics_command_buffer = nullptr;

        // Check if recreating swap chain and boot out.
        if (present::recreating_swapchain) {
            VkResult result = vkDeviceWaitIdle(device);
            SLIM_LOG_DEBUG("recreating_swapchain: vkDeviceWaitIdle(device)")
            if (!isVulkanResultSuccess(result)) {
                SLIM_LOG_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (1) failed: '%s'", getVulkanResultString(result, true));
                return false;
            }
            SLIM_LOG_INFO("Recreating swapchain, booting.");
            return false;
        }

        // Check if the framebuffer has been resized. If so, a new swapchain must be created.
        // Also include a vsync changed check.
        if (present::framebuffer_size_generation != present::framebuffer_size_last_generation || present::render_flag_changed) {
            VkResult result = vkDeviceWaitIdle(device);
            SLIM_LOG_DEBUG("framebuffer_size_generation != framebuffer_size_last_generation || render_flag_changed: vkDeviceWaitIdle(device)")
            if (!isVulkanResultSuccess(result)) {
                SLIM_LOG_ERROR("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed: '%s'", getVulkanResultString(result, true))
                return false;
            }

            present::render_flag_changed = false;
            SLIM_LOG_DEBUG("if (!recreateSwapchain())")
            // If the swapchain recreation failed (because, for example, the window was minimized),
            // boot out before unsetting the flag.
            if (!present::recreateSwapchain()) {
                SLIM_LOG_ERROR("Failed to recreate swapchain!.");
                return false;
            }

            SLIM_LOG_INFO("Resized, booting.");
            return false;
        }

        // Wait for the execution of the current frame to complete. The fence being free will allow this one to move on.
        VkResult result = vkWaitForFences(device, 1, &present::in_flight_fences[present::current_frame], true, UINT64_MAX);
        if (!isVulkanResultSuccess(result)) {
            SLIM_LOG_FATAL("In-flight fence wait failure! error: %s", getVulkanResultString(result, true));
            return false;
        }

        // Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
        // This same semaphore will later be waited on by the queue submission to ensure this image is available.
        if (!present::acquireNextImageIndex(UINT64_MAX, present::image_available_semaphores[present::current_frame], VK_NULL_HANDLE , &present::current_image_index)) {
            SLIM_LOG_ERROR("Failed to acquire next image index, booting.");
            return false;
        }

        // Begin recording commands.
        graphics_command_buffer = &present::_graphics_command_buffers[present::current_frame];
        graphics_command_buffer->reset();
        graphics_command_buffer->begin(false, false, false);
        graphics_command_buffer->setViewportAndScissor(present::swapchain_rect);

        return true;
    }

    bool endFrame() {
        if (!graphics_command_buffer) {
            SLIM_LOG_ERROR("endFrame: No command buffer")
            return false;
        }

        graphics_command_buffer->end();

        // Make sure the previous frame is not using this image (i.e. its fence is being waited on)
        if (present::images_in_flight[present::current_image_index] != VK_NULL_HANDLE) {
//            SLIM_LOG_DEBUG("images_in_flight[current_image_index = %d] != VK_NULL_HANDLE : vkWaitForFences(images_in_flight[%d])", present::current_image_index, present::current_image_index)
            VkResult result = vkWaitForFences(device, 1, &present::images_in_flight[present::current_image_index], true, UINT64_MAX);
            if (!isVulkanResultSuccess(result)) {
                SLIM_LOG_FATAL("vkWaitForFences error: %s", getVulkanResultString(result, true));
            }
        }

        // Mark the image fence as in-use by this frame.
        present::images_in_flight[present::current_image_index] = present::in_flight_fences[present::current_frame];

        // Reset the fence for use on the next frame
        VK_CHECK(vkResetFences(device, 1, &present::in_flight_fences[present::current_frame]))

        // Submit the queue and wait for the operation to complete.
        // Begin queue submission
        VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

        // Command buffer(s) to be executed.
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &graphics_command_buffer->handle;

        // The semaphore(s) to be signaled when the queue is complete.
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &present::render_complete_semaphores[present::current_frame];

        // Wait semaphore ensures that the operation cannot begin until the image is available.
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &present::image_available_semaphores[present::current_frame];

        // Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
        // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment
        // writes from executing until the semaphore signals (i.e. one frame is presented at a time)
        VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.pWaitDstStageMask = flags;

        VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, present::in_flight_fences[present::current_frame]);
        if (result != VK_SUCCESS) {
            SLIM_LOG_ERROR("vkQueueSubmit failed with result: %s", getVulkanResultString(result, true));
            return false;
        }

        graphics_command_buffer->state = State::Submitted;
        // End queue submission

        // Give the image back to the swapchain.
        present::present(present::render_complete_semaphores[present::current_frame], present::current_image_index);

        return true;
    }
}