#pragma once
#include "inc.hh"
namespace {

struct GfxApp {
    VkPipeline gfx_pipeline;

    VkBuffer       vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer       index_buffer;
    VkDeviceMemory index_buffer_memory;

    void init(GfxWindow* gfx);
    void frame(GfxFrameContext ctx);
};

}  // namespace