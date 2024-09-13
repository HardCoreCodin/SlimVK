#pragma once

#include "./image.h"
#include "./framebuffer.h"
#include "./graphics.h"
#include "./render_pass.h"


namespace gpu {
    namespace shadow_pass {
        RenderPass render_pass;
		FrameBuffer frame_buffer;
		GPUImage image;

        bool create(u32 width, u32 height, const char* image_name = "shadow_map") {
			VkSubpassDependency dependencies[2]{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			
			//const VkFormat offscreenDepthFormat{ VK_FORMAT_D16_UNORM };
			render_pass.create(
					{
						"shadow_render_pass", {}, 1.0f, 0, 1, {
						{ 
							Attachment::Type::Depth,  
							(u8)Attachment::Flag::Clear | 
							(u8)Attachment::Flag::Store
						}
					}
				},
				dependencies, 2
			);
			
			image.create(
				width, 
				height,
				present::depth_format,
				(
					(u8)GPUImageFlag::FILTER | 
				    (u8)GPUImageFlag::SAMPLE | 
					(u8)GPUImageFlag::VIEW
				),
				image_name,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT
			);

			frame_buffer.create(width, height, render_pass.handle, &image.view, 1);

			return true;
        }
    }
}