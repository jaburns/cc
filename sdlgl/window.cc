#include "inc.hh"
namespace {

void SdlGlWindow::init(cchar* window_title, SDL_AudioCallback sdl_audio_callback) {
    ZeroStruct(this);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0) {
        Panic("Failed to initialize SDL: %s\n", SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    screen_size.x = 1280;
    screen_size.y = 720;

    sdl_window = SDL_CreateWindow(
        window_title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        screen_size.x, screen_size.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!sdl_window) {
        Panic("Failed to create SDL window: %s\n", SDL_GetError());
    }

    sdl_context = SDL_GL_CreateContext(sdl_window);
    if (!sdl_context) {
        Panic("Failed to create SDL GL context: %s\n", SDL_GetError());
    }

    glewExperimental = GL_TRUE;
    GLenum err       = glewInit();
    if (err != GLEW_OK) {
        Panic("Error initializing GLEW: %s\n", glewGetErrorString(err));
    }

    if (SDL_GL_SetSwapInterval(1) < 0) {
        Panic("Unable to set VSync: %s", SDL_GetError());
    }

#if DEBUG
    imgui_context    = ImGui::CreateContext(NULL);
    ImGuiIO* io      = &ImGui::GetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplSdlGL3_Init(sdl_window, NULL);
#endif

    SDL_AudioSpec obtained_spec;
    SDL_AudioSpec desired_spec = (SDL_AudioSpec){
        .freq     = AUDIO_SAMPLE_RATE,
        .format   = AUDIO_F32,
        .channels = 2,
        .samples  = 512,
        .callback = sdl_audio_callback,
        .userdata = this,
    };

    sdl_audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
    if (sdl_audio_device == 0) {
        Panic("Failed to open audio device: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(sdl_audio_device, 0);

    for (u32 i = 0; i < SDL_NumJoysticks(); ++i) {
        printf("Using joystick: %s\n", SDL_JoystickNameForIndex(i));
        sdl_joystick       = SDL_JoystickOpen(i);
        active_joystick_id = i;
        break;
    }
}

i32 SdlGlWindow::poll() {
    mouse_delta       = IVEC2_ZERO;
    mouse_delta_wheel = 0.f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
#if DEBUG
        ImGui_ImplSdlGL3_ProcessEvent(&event);
        bool imgui_wants_mouse    = ImGui::GetIO().WantCaptureMouse;
        bool imgui_wants_keyboard = ImGui::GetIO().WantCaptureKeyboard;
#else
        bool imgui_wants_mouse    = false;
        bool imgui_wants_keyboard = false;
#endif
        switch (event.type) {
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        i32 w         = event.window.data1;
                        i32 h         = event.window.data2;
                        screen_size.x = w;
                        screen_size.y = h;
                        break;
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (!imgui_wants_mouse) {
                    mouse_button = true;
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (!imgui_wants_mouse) {
                    mouse_button = false;
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                if (!imgui_wants_mouse) {
                    mouse_delta.x = event.motion.xrel;
                    mouse_delta.y = -event.motion.yrel;
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                if (!imgui_wants_mouse) {
                    mouse_delta_wheel = event.wheel.preciseY;
                }
                break;
            }
            case SDL_QUIT: {
                return 1;
            }
            case SDL_KEYDOWN: {
                if (!imgui_wants_keyboard) {
                    keyboard.scancodes_down[event.key.keysym.scancode] = true;
                }
                break;
            }
            case SDL_KEYUP: {
                if (!imgui_wants_keyboard) {
                    keyboard.scancodes_down[event.key.keysym.scancode] = false;
                }
                break;
            }
            case SDL_JOYAXISMOTION: {
                // printf("Joystick %d axis %d moved to %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                if (event.jaxis.which == active_joystick_id && event.jaxis.axis < JoystickState::AXIS_COUNT) {
                    joystick.axis_values[event.jaxis.axis] =
                        event.jaxis.value < 0 ? (f32)event.jaxis.value / 32768.0 : (f32)event.jaxis.value / 32767.0;
                }
                break;
            }
            case SDL_JOYBUTTONDOWN: {
                // printf("Joystick %d button %d pressed\n", event.jbutton.which, event.jbutton.button);
                if (event.jbutton.which == active_joystick_id && event.jbutton.button < JoystickState::BUTTON_COUNT) {
                    joystick.buttons_down[event.jbutton.button] = true;
                }
                break;
            }
            case SDL_JOYBUTTONUP: {
                // printf("Joystick %d button %d released\n", event.jbutton.which, event.jbutton.button);
                if (event.jbutton.which == active_joystick_id && event.jbutton.button < JoystickState::BUTTON_COUNT) {
                    joystick.buttons_down[event.jbutton.button] = false;
                }
                break;
            }
        }
    }
#if DEBUG
    ImGui_ImplSdlGL3_NewFrame(sdl_window);
#endif
    return 0;
}

void SdlGlWindow::swap() {
#if DEBUG
    ImGui::Render();
    ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
#endif
    SDL_GL_SwapWindow(sdl_window);
}

}  // namespace