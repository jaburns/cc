#if EDITOR
#define app_dll_export extern "C"
#else
#define app_dll_export static
#endif

#include "../sdlvk/inc.hh"
#include "../../src/main.hh"

#include "../sdlvk/inc.cc"
#include "../../src/main.cc"

app_dll_export AudioCallbackFn app_get_audio_callback_fn() {
    return &AudioPlayer::stream_callback;
}