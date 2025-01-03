#include "inc.hh"
namespace {

void AudioClip::load(Arena* arena, cchar* path) {
    ZeroStruct(this);

    ScratchArena scratch = ScratchArena(Slice(arena));

    FILE* fp = fopen(path, "rb");
    if (!fp) Panic("Failed to open audio file %s", path);

    OggVorbis_File vorbis;
    if (ov_open_callbacks(fp, &vorbis, NULL, 0, OV_CALLBACKS_DEFAULT) != 0) {
        Panic("Invalid ogg file");
    }
    i64 total_sample_count = 2 * ov_pcm_total(&vorbis, -1);

    vorbis_info* vi       = ov_info(&vorbis, -1);
    i32          channels = vi->channels;
    if (channels != 1 && channels != 2) {
        Panic("Unsupported number of channels. Only mono and stereo are supported.");
    }

    bool must_resample = vi->rate != AUDIO_SAMPLE_RATE;

    Arena*     buffer_arena = must_resample ? scratch.arena : arena;
    Slice<f32> buffer       = buffer_arena->alloc_many<f32>(total_sample_count);

    i32 j = 0;
    while (1) {
        f32** pcm;
        i32   section = 0;
        i32   read    = ov_read_float(&vorbis, &pcm, 1024, &section);
        if (channels == 1) {
            for (i32 i = 0; i < read; ++i) {
                buffer.elems[j++] = pcm[0][i];
                buffer.elems[j++] = pcm[0][i];
            }
        } else {
            for (i32 i = 0; i < read; ++i) {
                buffer.elems[j++] = pcm[0][i];
                buffer.elems[j++] = pcm[1][i];
            }
        }
        if (read <= 0)
            break;
    }

    if (must_resample) {
        f32 ratio                   = (f32)AUDIO_SAMPLE_RATE / (f32)vi->rate;
        i32 og_sample_count         = total_sample_count;
        total_sample_count          = (i32)(og_sample_count * ratio);
        Slice<f32> resampled_buffer = arena->alloc_many<f32>(total_sample_count);

        // https://yehar.com/blog/wp-content/uploads/2009/08/deip.pdf
        // TODO(jaburns) replace linear interpolation resampling with B-spline:
        //
        // // 4-point, 3rd-order B-spline (x-form)
        // float ym1py1 = y[-1]+y[1];
        // float c0 = 1/6.0*ym1py1 + 2/3.0*y[0];
        // float c1 = 1/2.0*(y[1]-y[-1]);
        // float c2 = 1/2.0*ym1py1 - y[0];
        // float c3 = 1/2.0*(y[0]-y[1]) + 1/6.0*(y[2]-y[-1]);
        // return ((c3*x+c2)*x+c1)*x+c0;

        for (i32 i = 0; i < total_sample_count; ++i) {
            f32 src_index       = i / ratio;
            i32 idx_floor       = (i32)src_index;
            f32 frac            = src_index - idx_floor;
            i32 idx_ceil        = (idx_floor + 1 < og_sample_count) ? idx_floor + 1 : idx_floor;
            resampled_buffer[i] = buffer[idx_floor] * (1.0f - frac) + buffer[idx_ceil] * frac;
        }

        buffer = resampled_buffer;
    }

    samples = buffer;

    ov_clear(&vorbis);
}

void AudioPlayer::play_clip(AudioClip* clip) {
    AudioPlayerMsg msg = {
        .tag     = AudioPlayerMsg::Tag::Play,
        .as_play = {.clip = clip},
    };
    msg_chan.push(&msg);
}

AudioPlayer AudioPlayer::alloc(Arena* arena) {
    AudioPlayer ret = {};
    ret.msg_chan    = Channel<AudioPlayerMsg>::alloc(arena, 256);
    return ret;
}

void AudioPlayer::stream_callback(u8* out_stream, i32 out_stream_byte_len) {
    for (auto& msg : msg_chan) {
        switch (msg.tag) {
            case AudioPlayerMsg::Tag::Play: {
                clip = msg.as_play.clip;
                idx  = 0;
                break;
            }
            case AudioPlayerMsg::Tag::Stop: {
                break;
            }
        }
    }

    f32* out    = (f32*)out_stream;
    i32  length = out_stream_byte_len / sizeof(f32);

    for (u32 i = 0; i < length; i += 2) {
        if (clip == NULL || idx >= clip->samples.count) {
            // out[i]     = ((f32)rand() / (f32)RAND_MAX) * AUDIO_MASTER_VOLUME;
            // out[i + 1] = ((f32)rand() / (f32)RAND_MAX) * AUDIO_MASTER_VOLUME;
            out[i]     = 0.f;
            out[i + 1] = 0.f;
            continue;
        }
        out[i]     = AUDIO_MASTER_VOLUME * clip->samples.elems[idx++];
        out[i + 1] = AUDIO_MASTER_VOLUME * clip->samples.elems[idx++];
    }
}

}  // namespace