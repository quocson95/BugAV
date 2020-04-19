#ifndef COMMON_H
#define COMMON_H
extern "C" {
#include <libavcodec/avcodec.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavcodec/avfft.h>
}
#include "define.h"
#include "framequeue.h"
#include "packetqueue.h"
#include "clock.h"

static unsigned sws_flags = SWS_BICUBIC;

class AudioParams {
public:
    int freq;
    int channels;
    int64_t channel_layout;
    AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
};

enum class ShowModeClock{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

class Decoder {
public:
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    SDL_cond *empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    SDL_Thread *decoder_tid;
};

enum class ShowMode {
    SHOW_MODE_NONE = -1,
    SHOW_MODE_VIDEO = 0,
    SHOW_MODE_WAVES,
    SHOW_MODE_RDFT,
    SHOW_MODE_NB
};

class SDL_Texture;

class VideoState {
public:
    SDL_Thread *read_tid = nullptr;
    AVInputFormat *iformat  = nullptr;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext *ic = nullptr;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    FrameQueue subpq;
    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;
    Decoder subdec;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st = nullptr;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf = nullptr;
    uint8_t *audio_buf1 = nullptr;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    AudioParams audio_src;
#if CONFIG_AVFILTER
    AudioParams audio_filter_src;
#endif
    AudioParams audio_tgt;
    SwrContext *swr_ctx = nullptr;
    int frame_drops_early;
    int frame_drops_late;

    ShowMode show_mode;
    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int sample_array_index;
    int last_i_start;
    RDFTContext *rdft = nullptr;
    int rdft_bits;
    FFTSample *rdft_data = nullptr;
    int xpos;
    double last_vis_time;
    SDL_Texture *vis_texture;
    SDL_Texture *sub_texture;
    SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st = nullptr;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st = nullptr;
    PacketQueue videoq;
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx;
    struct SwsContext *sub_convert_ctx;
    int eof;

    char *filename;
    int width, height, xleft, ytop;
    int step;

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph
#endif

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    SDL_cond *continue_read_thread;
};

class TextureFormatEntry {
public:
    AVPixelFormat format;
    int texture_fmt;
};


#endif // COMMON_H
