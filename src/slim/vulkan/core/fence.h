#pragma once

#include "./base.h"

namespace gpu {

    struct Fence {
        VkFence handle = nullptr;
        bool is_signaled = true;

        void create() {
            destroy();

            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
            // This will prevent the application from waiting indefinitely for the first frame to render since it
            // cannot be rendered until a frame is "rendered" before it.
            VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &handle))
        }

        void destroy() {
            if (handle) vkDestroyFence(device, handle, nullptr);
            handle = nullptr;
        }
    };

}