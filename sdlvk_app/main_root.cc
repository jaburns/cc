#if EDITOR
#include <dlfcn.h>
#include "../../src/main.hh"
#include "watch_fs.h"
#include "../sdlvk/inc.cc"
#else
#include "main_dll.cc"
#endif
namespace {
// -----------------------------------------------------------------------------

#if EDITOR
global AtomicVal<bool> rebuild_running   = {false};
global AtomicVal<bool> rebuild_succeeded = {false};
global pthread_t       rebuild_thread    = nullptr;

void* rebuild_thread_fn(void* data) {
    *rebuild_succeeded = system((cchar*)data) == 0;
    *rebuild_running   = false;
    return nullptr;
}
#endif

// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------

void root_main() {
#if EDITOR
    void* handle = dlopen("bin/libreload.dylib", RTLD_LAZY);
    if (!handle) Panic("Error loading dylib: %s\n", dlerror());
    void            (*app_init)(App*, Gfx*, Arena)     = (void (*)(App*, Gfx*, Arena))dlsym(handle, "app_init");
    void            (*app_tick)(App*)                  = (void (*)(App*))dlsym(handle, "app_tick");
    bool            (*app_frame)(App*, f64, f32, f32)  = (bool (*)(App*, f64, f32, f32))dlsym(handle, "app_frame");
    AudioCallbackFn (*app_get_audio_callback_fn)(void) = (AudioCallbackFn(*)(void))dlsym(handle, "app_get_audio_callback_fn");
    void            (*app_freeze)(App*)                = (void (*)(App*))dlsym(handle, "app_freeze");
    void            (*app_thaw)(App*)                  = NULL;

    cchar* watch_paths[] = APP_RELOAD_WATCH_DIRS;
    watch_fs_create(watch_paths, RawArrayLen(watch_paths));
#endif
    timing_global_init();

    *can_do_audio = true;

    Arena arena = Arena::create(memory_get_global_allocator());

    Gfx gfx;
    gfx.init(&arena, APP_WINDOW_TITLE, audio_callback_trampoline);

    gfx.audio_player      = AudioPlayer::alloc(&arena);
    gfx.audio_callback_fn = app_get_audio_callback_fn();

    App* app = arena.alloc_one<App>();
    app_init(app, &gfx, arena);

    u64 first_ticks    = timing_get_ticks();
    u64 last_ticks     = first_ticks;
    u64 tick_acc_nanos = 0;

    while (gfx.poll()) {
        u64 cur_ticks   = timing_get_ticks();
        u64 delta_ticks = cur_ticks - last_ticks;
        last_ticks      = cur_ticks;

        f32 delta_time = (f32)((f64)timing_ticks_to_nanos(delta_ticks) / 1'000'000'000.);
        f64 time       = (f64)(timing_ticks_to_nanos(timing_get_ticks() - first_ticks) / 1000) / 1'000'000.;

        tick_acc_nanos += timing_ticks_to_nanos(delta_ticks);
        while (tick_acc_nanos > APP_NANOS_PER_TICK) {
            tick_acc_nanos -= APP_NANOS_PER_TICK;
            app_tick(app);
        }

        f32 tick_lerp = (f32)((f64)tick_acc_nanos / (f64)APP_NANOS_PER_TICK);

        if (!app_frame(app, time, delta_time, tick_lerp)) break;

#if EDITOR
        if (!*rebuild_running) {
            Str changed_path = Str::from_nullable_cstr(watch_fs_consume_file_changed());
            if (changed_path.count > 0) {
                Str    suffix = changed_path.trim().after_last_index('.');
                cchar* cmd    = nullptr;
                if (suffix.eq_cstr("slang")) {
                    cmd = "jaburns_cc/sdlvk_app/build.sh --shaders-only";
                } else if (suffix.eq_cstr("cc") || suffix.eq_cstr("hh") || suffix.eq_cstr("c") || suffix.eq_cstr("h")) {
                    cmd = "jaburns_cc/sdlvk_app/build.sh --dll-only";
                }
                if (cmd) {
                    *rebuild_running = true;
                    pthread_create(&rebuild_thread, nullptr, rebuild_thread_fn, (void*)cmd);
                }
            }
        }
        if (*rebuild_succeeded) {
            *rebuild_succeeded = false;

            *can_do_audio = false;
            while (*inside_audio);

            gfx.audio_callback_fn = nullptr;
            app_freeze(app);

            dlclose(handle);
            handle                    = dlopen("bin/libreload.dylib", RTLD_LAZY);
            app_init                  = NULL;
            app_tick                  = (void (*)(App*))dlsym(handle, "app_tick");
            app_frame                 = (bool (*)(App*, f64, f32, f32))dlsym(handle, "app_frame");
            app_get_audio_callback_fn = (AudioCallbackFn(*)(void))dlsym(handle, "app_get_audio_callback_fn");
            app_freeze                = (void (*)(App*))dlsym(handle, "app_freeze");
            app_thaw                  = (void (*)(App*))dlsym(handle, "app_thaw");

            gfx.audio_callback_fn = app_get_audio_callback_fn();
            app_thaw(app);

            *can_do_audio = true;

            watch_fs_consume_file_changed();
        }
#endif
    }

    gfx.wait_device_idle();

    *can_do_audio = false;
    while (*inside_audio);

#if EDITOR
    watch_fs_destroy();
    if (rebuild_thread) pthread_join(rebuild_thread, nullptr);
#endif
}

}  // namespace
// -----------------------------------------------------------------------------

i32 main() { root_main(); }

// -----------------------------------------------------------------------------