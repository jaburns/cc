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
  public:
    struct Frame {
        VkCommandBuffer cmd_buffer;
        VkFramebuffer   framebuffer;
        VkExtent2D      extent;
        u32             image_index;
    };

  private:
    static constexpr bool  ENABLE_VALIDATION_LAYERS = (bool)DEBUG;
    static constexpr usize MAX_FRAMES_IN_FLIGHT     = 2;
    static constexpr usize MAX_SWAP_CHAIN_IMAGES    = 4;

    SDL_Window*       sdl_window;
    SDL_AudioDeviceID sdl_audio_device;
    SDL_JoystickID    active_joystick_id;
    SDL_Joystick*     sdl_joystick;

    VkQueue        present_queue;
    VkSurfaceKHR   surface;
    VkSwapchainKHR swap_chain;
    VkExtent2D     swap_chain_extent;

    InlineVec<VkImage, MAX_SWAP_CHAIN_IMAGES>       swap_chain_images;
    InlineVec<VkImageView, MAX_SWAP_CHAIN_IMAGES>   swap_chain_image_views;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> imgui_framebuffers;
    InlineVec<VkFramebuffer, MAX_SWAP_CHAIN_IMAGES> main_pass_framebuffers;

    VkRenderPass imgui_render_pass;
    u32          cur_framebuffer_idx;
    bool         framebuffer_resized;

    Array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> command_buffers;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     image_available_semaphores;
    Array<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     render_finished_semaphores;
    Array<VkFence, MAX_FRAMES_IN_FLIGHT>         in_flight_fences;

  public:
    VkInstance         instance;
    VkPhysicalDevice   physical_device;
    VkDevice           device;
    VkSurfaceFormatKHR surface_format;
    VkQueue            graphics_queue;
    VkCommandPool      command_pool;
    VkRenderPass       main_pass;
    Frame              frame;

    AudioPlayer     audio_player;
    AudioCallbackFn audio_callback_fn;
    JoystickState   joystick;
    KeyboardState   keyboard;
    ivec2           screen_size;
    ivec2           mouse_delta;
    f32             mouse_delta_wheel;
    bool            mouse_button;

    void init(cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    bool poll();
    void begin_frame();
    void end_frame();
    void wait_safe_quit();

    void vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VKDropPool* drop_pool, VkBuffer* out_buffer, VkDeviceMemory* out_buffer_memory);
    void vk_copy_buffer(VkQueue queue, VkBuffer dest, VkBuffer src, VkDeviceSize size);

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