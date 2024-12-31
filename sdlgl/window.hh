#pragma once
#include "inc.hh"
namespace {

#define JOYSTICK_AXIS_COUNT   32
#define JOYSTICK_BUTTON_COUNT 32

struct KeyboardState {
    Array<bool, SDL_NUM_SCANCODES> scancodes_down;
};

struct JoystickState {
    Array<f32, JOYSTICK_AXIS_COUNT>    axis_values;
    Array<bool, JOYSTICK_BUTTON_COUNT> buttons_down;
};

struct SdlGlWindow {
    SDL_Window*       sdl_window;
    SDL_GLContext     sdl_context;
    SDL_AudioDeviceID sdl_audio_device;
//     AudioCallbackFn   audio_callback_fn;
//     AudioPlayer       audio_player;
#if DEBUG
    ImGuiContext* imgui_context;
#endif
    SDL_JoystickID active_joystick_id;
    SDL_Joystick*  sdl_joystick;
    JoystickState  joystick;
    KeyboardState  keyboard;
    ivec2          screen_size;
    ivec2          mouse_delta;
    f32            mouse_delta_wheel;
    bool           mouse_button;

    void init(cchar* window_title, SDL_AudioCallback sdl_audio_callback);
    i32  poll();
    void swap();
};

}  // namespace