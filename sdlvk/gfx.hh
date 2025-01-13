#pragma once
#include "inc.hh"
// -----------------------------------------------------------------------------
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

class VKDropPool {
    typedef void (*DropFn)(VkDevice, void*, void*);

    struct Element {
        DropFn drop;
        void*  target;
    };
    Vec<Element> elems;

  public:
    static VKDropPool alloc(Arena* arena, usize capacity);
    forall(T) void push(void (*drop)(VkDevice, T*, const VkAllocationCallbacks*), T* target);
    void drop_all(VkDevice device);
};

class Gfx {
    static constexpr bool  ENABLE_VALIDATION_LAYERS = (bool)DEBUG;
    static constexpr usize MAX_SWAP_CHAIN_IMAGES    = 4;

  public:
    static constexpr usize MAX_FRAMES_IN_FLIGHT = 2;

  private:
    SDL_Window*       sdl_window;
    SDL_AudioDeviceID sdl_audio_device;
    SDL_JoystickID    active_joystick_id;
    SDL_Joystick*     sdl_joystick;

    VkSurfaceKHR       surface;
    VkSurfaceFormatKHR surface_format;
    VkQueue            graphics_queue;
    VkQueue            present_queue;
    VkFormat           depth_format;
    VkCommandPool      command_pool;

    VkSwapchainKHR swap_chain;
    VKDropPool     swap_chain_drop_pool;

    InlineVec<VkImage, MAX_SWAP_CHAIN_IMAGES>       swap_chain_images;
    InlineVec<VkImageView, MAX_SWAP_CHAIN_IMAGES>   swap_chain_image_views;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> main_pass_framebuffers;
#if EDITOR
    VkRenderPass                                    imgui_render_pass;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> imgui_framebuffers;
#endif
    VkImage        color_image;
    VkImageView    color_image_view;
    VkDeviceMemory color_image_memory;

    VkImage        depth_image;
    VkImageView    depth_image_view;
    VkDeviceMemory depth_image_memory;

    Array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> command_buffers;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     image_available_semaphores;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     render_finished_semaphores;
    Array<VkFence, MAX_FRAMES_IN_FLIGHT>         in_flight_fences;

    u32  cur_image_idx;
    bool framebuffer_resized;

  public:
    u32 cur_framebuffer_idx;

    VkSampleCountFlagBits msaa_samples;

    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkDevice         device;
    VkRenderPass     main_pass;
    ivec2            screen_size;

    AudioPlayer     audio_player;
    AudioCallbackFn audio_callback_fn;

    JoystickState joystick;
    KeyboardState keyboard;
    ivec2         mouse_delta;
    f32           mouse_delta_wheel;
    bool          mouse_button;

    void init(Arena* arena, cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    bool poll();

    VkCommandBuffer main_render_pass_begin(vec4 color_clear, f32 depth_clear, u32 stencil_clear);
    void            main_render_pass_end();

    void wait_queue_idle();
    void wait_device_idle();

    void vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VKDropPool* drop_pool, VkBuffer* out_buffer, VkDeviceMemory* out_buffer_memory);
    void vk_create_image(u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VKDropPool* drop_pool, VkImage* out_image, VkDeviceMemory* out_image_memory);
    void vk_copy_buffer(VkBuffer dest, VkBuffer src, VkDeviceSize size);
    void vk_copy_buffer_to_image(VkImage dest, VkBuffer src, u32 width, u32 height);
    void vk_transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, u32 mip_levels);

    VkCommandBuffer vk_one_shot_command_buffer_begin();
    void            vk_one_shot_command_buffer_submit(VkCommandBuffer cmd_buffer);

  private:
    void create_swap_chain();
};

// -----------------------------------------------------------------------------

#define VKExpect(expr)                                                                            \
    do {                                                                                          \
        VkResult result = (expr);                                                                 \
        if (result != VK_SUCCESS) Panic("Vulkan API call '%s' returned error %d", #expr, result); \
    } while (0)

template <typename T, auto Fn, typename... Args>
Slice<T> vk_get_slice(Arena* arena, Args... args);

template <auto Fn, typename VecType, typename... Args>
void vk_get_vec(VecType* vec, Args... args);

// -----------------------------------------------------------------------------
}  // namespace