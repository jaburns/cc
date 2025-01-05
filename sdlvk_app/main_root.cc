#if EDITOR
#include <dlfcn.h>
#include "../../src/main.hh"
#include "../vendor/watchfs.h"
#include "../sdlvk/inc.cc"
#else
#include "main_dll.cc"
#endif

global AtomicVal<bool> can_do_audio = {false};
global AtomicVal<bool> inside_audio = {false};

void audio_callback_trampoline(void* data, u8* out_stream, i32 out_stream_byte_len) {
    Gfx* gfx    = {};
    f32* out    = {};
    i32  length = {};

    if (!*can_do_audio) goto write_empty;
    *inside_audio = true;
    if (!*can_do_audio) goto write_empty;

    gfx = (Gfx*)data;
    if (gfx->audio_callback_fn != NULL) {
        (gfx->audio_player.*gfx->audio_callback_fn)(out_stream, out_stream_byte_len);
        goto end;
    }

write_empty: {}
    out    = (f32*)out_stream;
    length = out_stream_byte_len / sizeof(f32);
    for (i32 i = 0; i < length; ++i) {
        out[i] = 0.f;
    }

end:
    *inside_audio = false;
}

i32 main() {
#if EDITOR
    void* handle = dlopen("bin/libreload.dylib", RTLD_LAZY);
    if (!handle) Panic("Error loading dylib: %s\n", dlerror());
    void            (*app_init)(App*, Gfx*, Arena)     = (void (*)(App*, Gfx*, Arena))dlsym(handle, "app_init");
    void            (*app_tick)(App*)                  = (void (*)(App*))dlsym(handle, "app_tick");
    void            (*app_frame)(App*, f32)            = (void (*)(App*, f32))dlsym(handle, "app_frame");
    AudioCallbackFn (*app_get_audio_callback_fn)(void) = (AudioCallbackFn(*)(void))dlsym(handle, "app_get_audio_callback_fn");
    void            (*app_freeze)(App*)                = (void (*)(App*))dlsym(handle, "app_freeze");
    void            (*app_thaw)(App*)                  = NULL;
#endif

#if EDITOR
    cchar* watch_paths[] = APP_RELOAD_WATCH_DIRS;
    watchfs_create(watch_paths, RawArrayLen(watch_paths));
#endif

    timing_global_init();

    *can_do_audio = true;

    Gfx gfx;
    gfx.init("vkaizo", audio_callback_trampoline);

    Arena arena           = Arena::create(memory_get_global_allocator(), 0);
    gfx.audio_player      = AudioPlayer::alloc(&arena);
    gfx.audio_callback_fn = app_get_audio_callback_fn();

    App* app = arena.alloc_one<App>();
    app_init(app, &gfx, arena);

    u64 last_ticks     = timing_get_ticks();
    u64 tick_acc_nanos = 0;

    while (gfx.poll()) {
        u64 cur_ticks   = timing_get_ticks();
        u64 delta_ticks = cur_ticks - last_ticks;
        last_ticks      = cur_ticks;

        tick_acc_nanos += timing_ticks_to_nanos(delta_ticks);
        while (tick_acc_nanos > APP_NANOS_PER_TICK) {
            tick_acc_nanos -= APP_NANOS_PER_TICK;
            app_tick(app);
        }

        f32 tick_lerp = (f32)((f64)tick_acc_nanos / (f64)APP_NANOS_PER_TICK);

#if EDITOR
        ImGui::ShowDemoWindow();
#endif

        gfx.begin_frame();
        app_frame(app, tick_lerp);
        gfx.end_frame();

#if EDITOR
        Str changed_path = Str::from_nullable_cstr(watchfs_check_file_changed());
        if (changed_path.count > 0) {
            Str    suffix  = changed_path.trim().after_last_index('.');
            cchar* command = nullptr;
            if (suffix.eq_cstr("slang")) {
                command = "jaburns_cc/sdlvk_app/build.sh --shaders-only";
            } else if (suffix.eq_cstr("cc") || suffix.eq_cstr("hh") || suffix.eq_cstr("c") || suffix.eq_cstr("h")) {
                command = "jaburns_cc/sdlvk_app/build.sh --dll-only";
            }
            if (command && 0 == system(command)) {
                *can_do_audio = false;
                while (*inside_audio);

                gfx.audio_callback_fn = nullptr;
                app_freeze(app);

                dlclose(handle);
                handle                    = dlopen("bin/libreload.dylib", RTLD_LAZY);
                app_init                  = NULL;
                app_tick                  = (void (*)(App*))dlsym(handle, "app_tick");
                app_frame                 = (void (*)(App*, f32))dlsym(handle, "app_frame");
                app_get_audio_callback_fn = (AudioCallbackFn(*)(void))dlsym(handle, "app_get_audio_callback_fn");
                app_freeze                = (void (*)(App*))dlsym(handle, "app_freeze");
                app_thaw                  = (void (*)(App*))dlsym(handle, "app_thaw");

                gfx.audio_callback_fn = app_get_audio_callback_fn();
                app_thaw(app);

                *can_do_audio = true;

                watchfs_check_file_changed();
            }
        }
#endif
    }

    gfx.wait_safe_quit();

    *can_do_audio = false;
    while (*inside_audio);

#if EDITOR
    watchfs_destroy();
#endif

    return 0;
}