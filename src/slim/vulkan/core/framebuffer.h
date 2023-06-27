#pragma once

#include "./base.h"

namespace gpu {
    struct FrameBuffer {
        VkFramebuffer handle;

        void create(u32 width, u32 height, VkRenderPass render_pass_handle, VkImageView *image_views, unsigned int image_views_count) {
            VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer_create_info.renderPass = render_pass_handle;
            framebuffer_create_info.attachmentCount = image_views_count;
            framebuffer_create_info.pAttachments = image_views;
            framebuffer_create_info.width = width;
            framebuffer_create_info.height = height;
            framebuffer_create_info.layers = 1;

            VK_CHECK(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &handle))
        }

        void destroy() {
            if (handle == nullptr) return;
            vkDestroyFramebuffer(device, handle, nullptr);
            handle = nullptr;
        }
    };
}