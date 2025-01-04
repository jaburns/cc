#pragma once
#include "inc.hh"
namespace {

struct KeyboardState {
    Array<bool, SDL_NUM_SCANCODES> scancodes_down;
};

struct JoystickState {
    static constexpr usize AXIS_COUNT   = 32;
    static constexpr usize BUTTON_COUNT = 32;

    Array<f32, AXIS_COUNT>    axis_values;
    Array<bool, BUTTON_COUNT> buttons_down;
};

struct GfxApp;

struct GfxWindow {
    static constexpr bool  ENABLE_VALIDATION_LAYERS = (bool)DEBUG;
    static constexpr usize MAX_FRAMES_IN_FLIGHT     = 2;
    static constexpr usize MAX_SWAP_CHAIN_IMAGES    = 4;

    SDL_Window* sdl_window;

    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkQueue          graphics_queue;
    VkQueue          present_queue;

    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR surface_format;

    VkSwapchainKHR swap_chain;
    VkExtent2D     swap_chain_extent;

    InlineVec<VkImage, MAX_SWAP_CHAIN_IMAGES>       swap_chain_images;
    InlineVec<VkImageView, MAX_SWAP_CHAIN_IMAGES>   swap_chain_image_views;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> imgui_framebuffers;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> main_pass_framebuffers;

    VkRenderPass main_pass;
    VkRenderPass imgui_render_pass;

    VkCommandPool command_pool;
    u32           cur_framebuffer_idx;

    bool framebuffer_resized;

    Array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> command_buffers;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     image_available_semaphores;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     render_finished_semaphores;
    Array<VkFence, MAX_FRAMES_IN_FLIGHT>         in_flight_fences;

    SDL_AudioDeviceID sdl_audio_device;
    AudioCallbackFn   audio_callback_fn;
    AudioPlayer       audio_player;

    SDL_JoystickID active_joystick_id;
    SDL_Joystick*  sdl_joystick;
    JoystickState  joystick;
    KeyboardState  keyboard;
    ivec2          screen_size;
    ivec2          mouse_delta;
    f32            mouse_delta_wheel;
    bool           mouse_button;

    GfxApp* app;

    void init(cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    void create_swap_chain();
    bool poll();
    void swap();
};

#define VKExpect(expr)                                                                            \
    do {                                                                                          \
        VkResult result = (expr);                                                                 \
        if (result != VK_SUCCESS) Panic("Vulkan API call '%s' returned error %d", #expr, result); \
    } while (0)

template <typename T, auto Fn, typename... Args>
Slice<T> vk_get_slice(Arena* arena, Args... args);

template <auto Fn, typename VecType, typename... Args>
void vk_get_vec(VecType* vec, Args... args);

}  // namespace