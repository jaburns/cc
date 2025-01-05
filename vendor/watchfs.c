#ifndef __APPLE__
#error Only macos supported for debug builds / file watch API
#endif

#include <CoreServices/CoreServices.h>
#include <pthread.h>
#include <dispatch/dispatch.h>
#include <stdatomic.h>

#define THREAD_SLEEP_MICROS 50000

static atomic_bool g_file_changed;
static atomic_bool g_fsevents_thread_running = true;
static pthread_t   g_fsevents_thread;

static const char** g_paths;
static unsigned int g_path_count;
static char         g_last_changed_file_in[PATH_MAX];
static char         g_last_changed_file_out[PATH_MAX];

static void fsevents_callback(
    ConstFSEventStreamRef         stream_ref,
    void*                         client_callback_info,
    size_t                        num_events,
    void*                         event_paths,
    const FSEventStreamEventFlags event_flags[],
    const FSEventStreamEventId    event_ids[]
) {
    if (num_events > 0) {
        char** paths = (char**)event_paths;
        strncpy(g_last_changed_file_in, paths[0], PATH_MAX - 1);
        g_file_changed = true;
    }
}

static void* fsevents_thread_function(void* data) {
    CFStringRef* path_refs = calloc(g_path_count, sizeof(CFStringRef));

    for (int i = 0; i < g_path_count; ++i) {
        path_refs[i] = CFStringCreateWithCString(NULL, g_paths[i], kCFStringEncodingUTF8);
    }
    CFArrayRef           paths_to_watch = CFArrayCreate(NULL, path_refs, g_path_count, &kCFTypeArrayCallBacks);
    FSEventStreamContext context        = {0, NULL, NULL, NULL, NULL};

    FSEventStreamRef stream = FSEventStreamCreate(
        NULL,
        &fsevents_callback,
        &context,
        paths_to_watch,
        kFSEventStreamEventIdSinceNow,
        0.0,
        kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagWatchRoot | kFSEventStreamCreateFlagIgnoreSelf
    );

    dispatch_queue_t dispatch_queue = dispatch_queue_create("FSEventsQueue", NULL);
    FSEventStreamSetDispatchQueue(stream, dispatch_queue);
    FSEventStreamStart(stream);

    while (g_fsevents_thread_running) {
        usleep(THREAD_SLEEP_MICROS);
    }

    FSEventStreamStop(stream);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
    dispatch_release(dispatch_queue);

    CFRelease(paths_to_watch);
    for (int i = 0; i < g_path_count; ++i) {
        CFRelease(path_refs[i]);
    }

    free(path_refs);

    return NULL;
}

void watchfs_create(const char** paths, unsigned int path_count) {
    g_paths      = paths;
    g_path_count = path_count;
    pthread_create(&g_fsevents_thread, NULL, fsevents_thread_function, NULL);
}

const char* watchfs_check_file_changed(void) {
    bool changed   = g_file_changed;
    g_file_changed = false;
    strncpy(g_last_changed_file_out, g_last_changed_file_in, PATH_MAX - 1);
    return changed ? g_last_changed_file_out : NULL;
}

void watchfs_destroy(void) {
    g_file_changed            = false;
    g_fsevents_thread_running = false;
    pthread_join(g_fsevents_thread, NULL);
}