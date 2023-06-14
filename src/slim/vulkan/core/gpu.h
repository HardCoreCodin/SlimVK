#pragma once

#include "./debug.h"
#include "./instance.h"
#include "./device.h"
#include "./swapchain.h"

namespace gpu {
    bool init(const char* app_name = "SlimVK") {
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

        _swapchain::create(framebuffer_width, framebuffer_height, _swapchain::SwapchainConfig_VSync);

        // Create buffers

//        // Geometry vertex buffer
//        const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_VERTEX, vertex_buffer_size, true, &context->object_vertex_buffer)) {
//            KERROR("Error creating vertex buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_vertex_buffer, 0);
//
//        // Geometry index buffer
//        const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_INDEX, index_buffer_size, true, &context->object_index_buffer)) {
//            KERROR("Error creating index buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_index_buffer, 0);
//
//        // Mark all geometries as invalid
//        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
//            context->geometries[i].id = INVALID_ID;
//        }
//
        SLIM_LOG_INFO("Vulkan renderer initialized successfully.");
        return true;
    }

    void shutdown() {
        vkDeviceWaitIdle(device);

        // Destroy in the opposite order of creation.
        // Destroy buffers
//        renderer_renderbuffer_destroy(&context->object_vertex_buffer);
//        renderer_renderbuffer_destroy(&context->object_index_buffer);

/*
        // Sync objects
        for (u8 i = 0; i < _swapchain.max_frames_in_flight; ++i) {
            if (image_available_semaphores[i]) {
                vkDestroySemaphore(device, image_available_semaphores[i], 0);
                image_available_semaphores[i] = 0;
            }
            if (queue_complete_semaphores[i]) {
                vkDestroySemaphore(device, queue_complete_semaphores[i], 0);
                queue_complete_semaphores[i] = 0;
            }
            vkDestroyFence(device, in_flight_fences[i], 0);
        }
//        darray_destroy(context->image_available_semaphores);
        image_available_semaphores = 0;

//        darray_destroy(context->queue_complete_semaphores);
        queue_complete_semaphores = 0;

        // Command buffers
        for (u32 i = 0; i < _swapchain.image_count; ++i) {
            if (graphics_command_buffers[i].handle) {
//                vulkan_command_buffer_free(
//                    context,
//                    context->device.graphics_command_pool,
//                    &context->graphics_command_buffers[i]);
                graphics_command_buffers[i].handle = 0;
            }
        }
//        darray_destroy(context->graphics_command_buffers);
        graphics_command_buffers = 0;

        // Swapchain

//        vulkan_swapchain_destroy(context, &context->swapchain);

*/
        _swapchain::destroy();

        SLIM_LOG_DEBUG("Destroying Vulkan device...");
        _device::destroy();

        SLIM_LOG_DEBUG("Destroying Vulkan surface...");
        if (_device::surface) {
            vkDestroySurfaceKHR(instance, _device::surface, nullptr);
            _device::surface = 0;
        }

        _debug::shutdown();

        SLIM_LOG_DEBUG("Destroying Vulkan instance...");
        vkDestroyInstance(instance, nullptr);
    }

}