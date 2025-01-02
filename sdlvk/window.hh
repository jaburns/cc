#pragma once
#include "inc.hh"
namespace {

struct GfxWindow {
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;

    SDL_Window*      sdl_window;
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    VkQueue          present_queue;
    VkSurfaceKHR     surface;
    VkSwapchainKHR   swap_chain;
    Slice<VkImage>   swap_chain_images;
    VkFormat         swap_chain_image_format;
    VkExtent2D       swap_chain_extent;

    AudioCallbackFn audio_callback_fn;
    AudioPlayer     audio_player;

    void init(cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    bool poll();
    void swap();
};

#define VKCall(expr, msg)                                             \
    do {                                                              \
        VkResult result = (expr);                                     \
        if (expr != VK_SUCCESS) Panic("Error %d :: %s", result, msg); \
    } while (0)

#define VKGetSlice(fn_name, type, arena_ptr, ...) [&] {           \
    u32 count;                                                    \
    fn_name(__VA_ARGS__, &count, nullptr);                        \
    if (count == 0) return Slice<type>{};                         \
    Slice<type> ret_slice = (arena_ptr)->alloc_many<type>(count); \
    fn_name(__VA_ARGS__, &count, ret_slice.elems);                \
    return ret_slice;                                             \
}()

}  // namespace