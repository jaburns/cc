#pragma once
#include "inc.hh"
namespace {

#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_MASTER_VOLUME .2

struct AudioClip {
    Slice<f32> samples;

    void load(Arena* arena, cchar* path);
};

struct AudioPlayerMsg {
    enum class Tag : u8 {
        Play,
        Stop,
    };

    struct Play {
        AudioClip* clip;
    };

    struct Stop {
    };

    Tag tag;
    union {
        Play as_play;
        Stop as_stop;
    };
};

class AudioPlayer {
    Channel<AudioPlayerMsg> msg_chan = {};
    AudioClip*              clip     = {};
    usize                   idx      = {};

  public:
    static AudioPlayer alloc(Arena* arena);

    void play_clip(AudioClip* clip);
    void stream_callback(u8* out_stream, i32 out_stream_byte_len);
};

typedef void (AudioPlayer::*AudioCallbackFn)(u8* out_stream, i32 out_stream_byte_len);

}  // namespace