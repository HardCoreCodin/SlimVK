#pragma once

#include "./framebuffer.h"

namespace gpu {
    struct RenderTarget {
        Attachment::Config attachment_configs[4];
        Attachment *attachments;
        FrameBuffer *framebuffer;
        u8 attachment_count;
    };

}