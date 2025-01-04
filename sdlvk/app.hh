#pragma once
#include "inc.hh"
namespace {

struct GfxApp {
    VkPipeline gfx_pipeline;

    VkBuffer       vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer       index_buffer;
    VkDeviceMemory index_buffer_memory;

    void init(
        VkDevice           device,
        VkPhysicalDevice   physical_device,
        VkSurfaceFormatKHR surface_format,
        VkQueue            graphics_queue,
        VkCommandPool      command_pool,
        VkRenderPass       main_pass
    );
    void frame(
        VkCommandBuffer buffer,
        VkRenderPass    main_pass,
        VkFramebuffer   main_pass_framebuffer,
        VkExtent2D      main_pass_extent
    );
};

}  // namespace