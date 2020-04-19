#include "ffplay.h"

ffplay::ffplay()
{

}

void ffplay::init()
{
    sdl_texture_format_map = {
        { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
        { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
        { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
        { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
        { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
        { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
        { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
        { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
        { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
        { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
        { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
        { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
        { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
        { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
        { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
        { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
        { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
        { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
        { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
        { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
    };

    av_sync_type = ShowModeClock::AV_SYNC_AUDIO_MASTER;
    screen_width = default_width;
    screen_height = default_height;
}

int ffplay::cmp_audio_fmts(AVSampleFormat fmt1, int64_t channel_count1, AVSampleFormat fmt2, int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
       if (channel_count1 == 1 && channel_count2 == 1)
           return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
       else
           return channel_count1 != channel_count2 || fmt1 != fmt2;
}

