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
    for (u32 i = 0; i < config.attachment_count; ++i) {
        Attachment::Config &attachment_config = config.attachment_configs[i];
        bool color = attachment_config.type == Attachment::Type::Color;
        u8 flags = attachment_config.flags;
        bool present = flags & (u8)Attachment::Flag::Present;
        bool store = flags & (u8)Attachment::Flag::Store;
        bool load = flags & (u8)Attachment::Flag::Load;
        bool clear = flags & (u8)Attachment::Flag::Clear;
        bool view = flags & (u8)Attachment::Flag::SourceIsView;

        VkAttachmentDescription attachment_desc = {};
        attachment_desc.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : (load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE);
        attachment_desc.storeOp = store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;

        // TODO: Configurability for stencil attachments.
        attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if (color) {
            attachment_desc.format = view ? VK_FORMAT_R8G8B8A8_UNORM : present::image_format.format;

            // If loading, that means coming from another pass, meaning the format should be
            // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. Otherwise it is undefined.
            attachment_desc.initialLayout = load ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

            // If this is the last pass writing to this attachment, present after should be set to true.
            attachment_desc.finalLayout = present ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Push to colour attachments array.
            color_attachment_descriptions[color_attachment_count++] = attachment_desc;
        } else if (attachment_config.type == Attachment::Type::Depth) {
            attachment_desc.format = present::depth_format;

            // If coming from a previous pass, should already be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL. Otherwise undefined.
            attachment_desc.initialLayout = load ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

            // Final layout for depth stencil attachments is always this.
            attachment_desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Push to depth attachments array.
            depth_attachment_descriptions[depth_attachment_count++] = attachment_desc;
        }
        // Push to general array.
        attachment_descriptions[attachment_count++] = attachment_desc;
    }

    // Setup the attachment references.
    u32 attachments_added = 0;

    // Colour attachment reference.
    VkAttachmentReference color_attachment_references[16];
    if (color_attachment_count > 0) {
        for (u32 i = 0; i < color_attachment_count; ++i) {
            color_attachment_references[i].attachment = attachments_added;  // Attachment description array index
            color_attachment_references[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachments_added++;
        }

        sub_pass.colorAttachmentCount = color_attachment_count;
        sub_pass.pColorAttachments = color_attachment_references;
    } else {
        sub_pass.colorAttachmentCount = 0;
        sub_pass.pColorAttachments = nullptr;
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
        sub_pass.pDepthStencilAttachment = nullptr;
    }

    bool has_depth_input_attachment = false;
    unsigned int depth_input_attachment_index = -1;

    // Input from a shader
    sub_pass.inputAttachmentCount = 0;
    sub_pass.pInputAttachments = nullptr;

    // Attachments used for multisampling colour attachments
    sub_pass.pResolveAttachments = nullptr;

    // Attachments not used in this sub pass, but must be preserved for the next.
    sub_pass.preserveAttachmentCount = 0;
    sub_pass.pPreserveAttachments = nullptr;

    // Render pass dependencies. TODO: make this configurable.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // 1.1 Aspect Reference (disambiguate between depth vs. stencil input attachment to read):
    VkInputAttachmentAspectReference aspect_reference {0,depth_input_attachment_index, VK_IMAGE_ASPECT_DEPTH_BIT};
    VkRenderPassInputAttachmentAspectCreateInfo aspect {VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO };
    aspect.aspectReferenceCount = 1;
    aspect.pAspectReferences = &aspect_reference;

    // Render pass create.
    VkRenderPassCreateInfo render_pass_create_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    render_pass_create_info.attachmentCount = attachment_count;
    render_pass_create_info.pAttachments = attachment_descriptions;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &sub_pass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;
    render_pass_create_info.pNext = has_depth_input_attachment ? &aspect : nullptr;
    render_pass_create_info.flags = 0;

    VK_CHECK(vkCreateRenderPass(device, &render_pass_create_info, nullptr, &handle))

    return true;
}

void gpu::RenderPass::destroy() {
    vkDestroyRenderPass(device, handle, nullptr);
    handle = nullptr;
}