#pragma once

#include "./debug.h"

bool gpu::RenderPass::create(const Config &new_config) {
    config = new_config;

    // Main sub pass
    VkSubpassDescription sub_pass = {};
    sub_pass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Attachments.
    VkAttachmentDescription attachment_descriptions[4];
    VkAttachmentDescription color_attachment_descriptions[2];
    VkAttachmentDescription depth_attachment_descriptions[2];

    u32 attachment_count = 0;
    u32 color_attachment_count = 0;
    u32 depth_attachment_count = 0;

    // Can always just look at the first target since they are all the same (one per frame).
    // render_target* target = &out_renderpass->targets[0];
    for (u32 i = 0; i < config.target.attachment_count; ++i) {
        RenderTarget::Attachment::Config &attachment_config = config.target.attachments[i];

        VkAttachmentDescription attachment_desc = {};
        if (attachment_config.type == RenderTarget::Attachment::Type::Color) {

            // Colour attachment.
            bool do_clear_colour = (config.clear_flags & (const u8)ClearFlags::Color) != 0;

            if (attachment_config.source == RenderTarget::Attachment::Source::Default) {
                attachment_desc.format = _swapchain::image_format.format;
            } else {
                // TODO: configurable format?
                attachment_desc.format = VK_FORMAT_R8G8B8A8_UNORM;
            }

            attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
            // attachment_desc.loadOp = do_clear_colour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;

            // Determine which load operation to use.
            if (attachment_config.load_operation == RenderTarget::Attachment::LoadOp::DontCare) {
                // If we don't care, the only other thing that needs checking is if the attachment is being cleared.
                attachment_desc.loadOp = do_clear_colour ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            } else {
                // If we are loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
                if (attachment_config.load_operation == RenderTarget::Attachment::LoadOp::Load) {
                    if (do_clear_colour) {
                        SLIM_LOG_WARNING("Colour attachment load operation set to load, but is also set to clear. This combination is invalid, and will err toward clearing. Verify attachment configuration.");
                        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    } else {
                        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                    }
                } else {
                    SLIM_LOG_FATAL("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for colour attachment.", attachment_desc.loadOp, config.clear_flags);
                    return false;
                }
            }

            // Determine which store operation to use.
            if (attachment_config.store_operation == RenderTarget::Attachment::StoreOp::DontCare) {
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            } else if (attachment_config.store_operation == RenderTarget::Attachment::StoreOp::Store) {
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            } else {
                SLIM_LOG_FATAL("Invalid store operation (0x%x) set for depth attachment. Check configuration.", attachment_config.store_operation);
                return false;
            }

            // NOTE: these will never be used on a colour attachment.
            attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // If loading, that means coming from another pass, meaning the format should be
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. Otherwise it is undefined.
            attachment_desc.initialLayout = (
                attachment_config.load_operation == RenderTarget::Attachment::LoadOp::Load ?
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
                VK_IMAGE_LAYOUT_UNDEFINED
            );

            // If this is the last pass writing to this attachment, present after should be set to true.
            attachment_desc.finalLayout = (
                attachment_config.present_after ?
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );  // Transitioned to after the render pass
            attachment_desc.flags = 0;

            // Push to colour attachments array.
            color_attachment_descriptions[color_attachment_count++] = attachment_desc;
        } else if (attachment_config.type == RenderTarget::Attachment::Type::Depth) {
            // Depth attachment.
            bool do_clear_depth = (config.clear_flags & (const u8)ClearFlags::Depth) != 0;

            if (attachment_config.source == RenderTarget::Attachment::Source::Default) {
                attachment_desc.format = _device::depth_format;
            } else {
                // TODO: There may be a more optimal format to use when not the default depth target.
                attachment_desc.format = _device::depth_format;
            }

            attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
            // Determine which load operation to use.
            if (attachment_config.load_operation == RenderTarget::Attachment::LoadOp::DontCare) {
                // If we don't care, the only other thing that needs checking is if the attachment is being cleared.
                attachment_desc.loadOp = (
                    do_clear_depth ?
                    VK_ATTACHMENT_LOAD_OP_CLEAR :
                    VK_ATTACHMENT_LOAD_OP_DONT_CARE
                );
            } else {
                // If we are loading, check if we are also clearing. This combination doesn't make sense, and should be warned about.
                if (attachment_config.load_operation == RenderTarget::Attachment::LoadOp::Load) {
                    if (do_clear_depth) {
                        SLIM_LOG_WARNING("Depth attachment load operation set to load, but is also set to clear. This combination is invalid, and will err toward clearing. Verify attachment configuration.");
                        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    } else {
                        attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                    }
                } else {
                    SLIM_LOG_FATAL("Invalid and unsupported combination of load operation (0x%x) and clear flags (0x%x) for depth attachment.", attachment_desc.loadOp, config.clear_flags);
                    return false;
                }
            }

            // Determine which store operation to use.
            if (attachment_config.store_operation == RenderTarget::Attachment::StoreOp::DontCare) {
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            } else if (attachment_config.store_operation == RenderTarget::Attachment::StoreOp::Store) {
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            } else {
                SLIM_LOG_FATAL("Invalid store operation (0x%x) set for depth attachment. Check configuration.", attachment_config.store_operation);
                return false;
            }

            // TODO: Configurability for stencil attachments.
            attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            // If coming from a previous pass, should already be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL. Otherwise undefined.
            attachment_desc.initialLayout = (
                attachment_config.load_operation == RenderTarget::Attachment::LoadOp::Load ?
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL :
                VK_IMAGE_LAYOUT_UNDEFINED
            );
            // Final layout for depth stencil attachments is always this.
            attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Push to colour attachments array.
            depth_attachment_descriptions[depth_attachment_count++] = attachment_desc;
        }
        // Push to general array.
        attachment_descriptions[attachment_count++] = attachment_desc;
    }

    // Setup the attachment references.
    u32 attachments_added = 0;

    // Colour attachment reference.

    VkAttachmentReference colour_attachment_references[16];
    if (color_attachment_count > 0) {
        for (u32 i = 0; i < color_attachment_count; ++i) {
            colour_attachment_references[i].attachment = attachments_added;  // Attachment description array index
            colour_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments_added++;
        }

        sub_pass.colorAttachmentCount = color_attachment_count;
        sub_pass.pColorAttachments = colour_attachment_references;
    } else {
        sub_pass.colorAttachmentCount = 0;
        sub_pass.pColorAttachments = 0;
    }

    // Depth attachment reference.
    VkAttachmentReference depth_attachment_references[1];
    if (depth_attachment_count > 0) {
        SLIM_ASSERT_MSG(depth_attachment_count == 1, "Multiple depth attachments not supported.")
        depth_attachment_references[0].attachment = attachments_added;  // Attachment description array index
        depth_attachment_references[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments_added++;

        // Depth stencil data.
        sub_pass.pDepthStencilAttachment = depth_attachment_references;
    } else {
        sub_pass.pDepthStencilAttachment = 0;
    }

    // Input from a shader
    sub_pass.inputAttachmentCount = 0;
    sub_pass.pInputAttachments = 0;

    // Attachments used for multisampling colour attachments
    sub_pass.pResolveAttachments = 0;

    // Attachments not used in this sub pass, but must be preserved for the next.
    sub_pass.preserveAttachmentCount = 0;
    sub_pass.pPreserveAttachments = 0;

    // Render pass dependencies. TODO: make this configurable.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Render pass create.
    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = attachment_count;
    render_pass_create_info.pAttachments = attachment_descriptions;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &sub_pass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    render_pass_create_info.pNext = 0;
    render_pass_create_info.flags = 0;

    VK_CHECK(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &handle))

    return true;
}

void gpu::RenderPass::destroy() {
    vkDestroyRenderPass(device, handle, nullptr);
    handle = 0;
}

bool gpu::RenderPass::begin(RenderTarget &render_target) {
    // Cold-cast the context

    CommandBuffer &command_buffer = graphics_command_buffers[_swapchain::image_index];

    // Begin the render pass.
    VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    begin_info.renderPass = handle;
    begin_info.framebuffer = render_target.framebuffer->handle;
    begin_info.renderArea.offset.x = config.render_area.left;
    begin_info.renderArea.offset.y = config.render_area.top;
    begin_info.renderArea.extent.width = config.render_area.right - config.render_area.left;
    begin_info.renderArea.extent.height = config.render_area.bottom - config.render_area.top;

    begin_info.clearValueCount = 0;
    begin_info.pClearValues = 0;

    VkClearValue clear_values[2] = {{}, {}};
    bool do_clear_colour = (config.clear_flags & (u8)RenderPass::ClearFlags::Color) != 0;
    if (do_clear_colour) {
        clear_values[begin_info.clearValueCount].color.float32[0] = config.clear_color.r;
        clear_values[begin_info.clearValueCount].color.float32[1] = config.clear_color.g;
        clear_values[begin_info.clearValueCount].color.float32[2] = config.clear_color.b;
        clear_values[begin_info.clearValueCount].color.float32[3] = 1.0f;
        begin_info.clearValueCount++;
    } else {
        // Still add it anyway, but don't bother copying data since it will be ignored.
        begin_info.clearValueCount++;
    }

    bool do_clear_depth = (config.clear_flags & (u8)RenderPass::ClearFlags::Depth) != 0;
    if (do_clear_depth) {
        clear_values[begin_info.clearValueCount].color.float32[0] = config.clear_color.r;
        clear_values[begin_info.clearValueCount].color.float32[1] = config.clear_color.g;
        clear_values[begin_info.clearValueCount].color.float32[2] = config.clear_color.b;
        clear_values[begin_info.clearValueCount].color.float32[3] = 1.0f;
        clear_values[begin_info.clearValueCount].depthStencil.depth = config.depth;

        bool do_clear_stencil = (config.clear_flags & (u8)RenderPass::ClearFlags::Stencil) != 0;
        clear_values[begin_info.clearValueCount].depthStencil.stencil = do_clear_stencil ? config.stencil : 0;
        begin_info.clearValueCount++;
    } else {
        for (u32 i = 0; i < render_target.attachment_count; ++i) {
            if (render_target.attachments[i].config.type == RenderTarget::Attachment::Type::Depth) {
                // If there is a depth attachment, make sure to add the clear count, but don't bother copying the data.
                begin_info.clearValueCount++;
            }
        }
    }

    begin_info.pClearValues = begin_info.clearValueCount > 0 ? clear_values : 0;

    vkCmdBeginRenderPass(command_buffer.handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer.state = State::InRenderPass;

    Color color{0.0f, 0.5f, 0.3f};
    VK_BEGIN_DEBUG_LABEL(command_buffer.handle, config.name, color);

    return true;
}

bool gpu::RenderPass::end() {
    CommandBuffer &command_buffer = graphics_command_buffers[_swapchain::image_index];
    // End the render pass.
    vkCmdEndRenderPass(command_buffer.handle);
    VK_END_DEBUG_LABEL(command_buffer.handle);

    command_buffer.state = State::Recording;

    return true;
}