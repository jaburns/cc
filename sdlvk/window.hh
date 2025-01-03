#pragma once
#include "inc.hh"
namespace {

struct GfxWindow {
    static constexpr bool  ENABLE_VALIDATION_LAYERS = true;
    static constexpr usize MAX_FRAMES_IN_FLIGHT     = 2;

    SDL_Window*      sdl_window;
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    VkQueue          present_queue;

    VkSurfaceFormatKHR surface_format;

    VkSurfaceKHR         surface;
    VkSwapchainKHR       swap_chain;
    VkExtent2D           swap_chain_extent;
    Slice<VkImage>       swap_chain_images;
    Slice<VkImageView>   swap_chain_image_views;
    Slice<VkFramebuffer> swap_chain_framebuffers;
    VkRenderPass         render_pass;
    VkPipelineLayout     pipeline_layout;
    VkPipeline           graphics_pipeline;
    VkCommandPool        command_pool;
    u32                  cur_framebuffer_idx;

    Array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> command_buffers;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     image_available_semaphores;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     render_finished_semaphores;
    Array<VkFence, MAX_FRAMES_IN_FLIGHT>         in_flight_fences;

    AudioCallbackFn audio_callback_fn;
    AudioPlayer     audio_player;

    void init(Arena* arena, cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    bool poll();
    void swap();
};

#define VKExpect(expr, msg)                                             \
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