#if EDITOR
#include <dlfcn.h>
#include "../../src/main.hh"
#include "../vendor/watchfs.h"
#include "../sdlvk/inc.cc"
#else
#include "main_dll.cc"
#endif

#define NANOS_PER_TICK 16666667LU

global AtomicVal<bool> can_do_audio = {false};
global AtomicVal<bool> inside_audio = {false};

void audio_callback_trampoline(void* data, u8* out_stream, i32 out_stream_byte_len) {
    Gfx* win    = {};
    f32* out    = {};
    i32  length = {};

    if (!*can_do_audio) goto write_empty;
    *inside_audio = true;
    if (!*can_do_audio) goto write_empty;

    win = (Gfx*)data;
    if (win->audio_callback_fn != NULL) {
        (win->audio_player.*win->audio_callback_fn)(out_stream, out_stream_byte_len);
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
    App*            (*app_create)(Gfx*, MemoryAllocator) = (App * (*)(Gfx*, MemoryAllocator)) dlsym(handle, "app_create");
    void            (*app_tick)(App*)                    = (void (*)(App*))dlsym(handle, "app_tick");
    void            (*app_frame)(App*, f32)              = (void (*)(App*, f32))dlsym(handle, "app_frame");
    void            (*app_freeze)(App*)                  = (void (*)(App*))dlsym(handle, "app_freeze");
    AudioCallbackFn (*app_thaw)(App*)                    = NULL;
#endif

#if EDITOR
    cchar* watch_paths[] = {"src", "shaders", "assets", "jaburns_cc"};
    watchfs_create(watch_paths, RawArrayLen(watch_paths));
#endif

    timing_global_init();

    *can_do_audio = true;

    Gfx win;
    win.init("vkaizo", audio_callback_trampoline);

    App* app;
    app = app_create(&win, memory_get_global_allocator());

    u64 last_ticks     = timing_get_ticks();
    u64 tick_acc_nanos = 0;

    while (win.poll()) {
        u64 cur_ticks   = timing_get_ticks();
        u64 delta_ticks = cur_ticks - last_ticks;
        last_ticks      = cur_ticks;

        tick_acc_nanos += timing_ticks_to_nanos(delta_ticks);
        while (tick_acc_nanos > NANOS_PER_TICK) {
            tick_acc_nanos -= NANOS_PER_TICK;
            app_tick(app);
        }

        f32 tick_lerp = (f32)((f64)tick_acc_nanos / (f64)NANOS_PER_TICK);

#if EDITOR
        ImGui::ShowDemoWindow();
#endif

        win.begin_frame();
        app_frame(app, tick_lerp);
        win.end_frame();

#if EDITOR
        Str changed_path = Str::from_nullable_cstr(watchfs_check_file_changed());
        if (changed_path.count > 0) {
            Str suffix = changed_path.trim().after_last_index('.');
            log(suffix);
            cchar* command = suffix.eq_cstr("slang") ? "./build.sh --shaders-only" : "./build.sh --dll-only";
            log(command);
            if (0 == system(command)) {
                *can_do_audio = false;
                while (*inside_audio);

                win.audio_callback_fn = nullptr;
                app_freeze(app);

                dlclose(handle);
                handle     = dlopen("bin/libreload.dylib", RTLD_LAZY);
                app_create = NULL;
                app_tick   = (void (*)(App*))dlsym(handle, "app_tick");
                app_frame  = (void (*)(App*, f32))dlsym(handle, "app_frame");
                app_freeze = (void (*)(App*))dlsym(handle, "app_freeze");
                app_thaw   = (AudioCallbackFn(*)(App*))dlsym(handle, "app_thaw");

                win.audio_callback_fn = app_thaw(app);

                *can_do_audio = true;

                watchfs_check_file_changed();
            }
        }
#endif
    }

    win.wait_safe_quit();

    *can_do_audio = false;
    while (*inside_audio);

#if EDITOR
    watchfs_destroy();
#endif

    return 0;
}