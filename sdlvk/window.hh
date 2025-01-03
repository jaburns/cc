#pragma once
#include "inc.hh"
namespace {

struct GfxWindow {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;

    SDL_Window*          sdl_window;
    VkInstance           instance;
    VkPhysicalDevice     physical_device;
    VkDevice             device;
    VkQueue              graphics_queue;
    VkQueue              present_queue;
    VkSurfaceKHR         surface;
    VkSwapchainKHR       swap_chain;
    VkExtent2D           swap_chain_extent;
    VkFormat             swap_chain_image_format;
    Slice<VkImage>       swap_chain_images;
    Slice<VkImageView>   swap_chain_image_views;
    Slice<VkFramebuffer> swap_chain_framebuffers;
    VkRenderPass         render_pass;
    VkPipelineLayout     pipeline_layout;
    VkPipeline           graphics_pipeline;
    VkCommandPool        command_pool;
    VkCommandBuffer      command_buffer;
    VkSemaphore          image_available_semaphore;
    VkSemaphore          render_finished_semaphore;
    VkFence              in_flight_fence;
    u32                  wrote;

    AudioCallbackFn audio_callback_fn;
    AudioPlayer     audio_player;

    void init(Arena* arena, cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    void record_command_buffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    bool poll();
    void swap();
};

#define VKCall(expr, msg)                                               \
    do {                                                                \
        VkResult result = (expr);                                       \
        if (result != VK_SUCCESS) Panic("Error %d :: %s", result, msg); \
    } while (0)

#define VKGetSlice0(fn_name, type, arena_ptr) [&] {               \
    u32 count;                                                    \
    fn_name(&count, nullptr);                                     \
    if (count == 0) return Slice<type>{};                         \
    Slice<type> ret_slice = (arena_ptr)->alloc_many<type>(count); \
    fn_name(&count, ret_slice.elems);                             \
    return ret_slice;                                             \
}()
#define VKGetSlice(fn_name, type, arena_ptr, ...) [&] {           \
    u32 count;                                                    \
    fn_name(__VA_ARGS__, &count, nullptr);                        \
    if (count == 0) return Slice<type>{};                         \
    Slice<type> ret_slice = (arena_ptr)->alloc_many<type>(count); \
    fn_name(__VA_ARGS__, &count, ret_slice.elems);                \
    return ret_slice;                                             \
}()

}  // namespace