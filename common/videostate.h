#ifndef VIDEOSTATE_H
#define VIDEOSTATE_H

extern "C" {
    #include <libavformat/avformat.h>
}

#include <QMap>

#include "clock.h"
#include "common.h"
#include "Decoder/decoder.h"
#include "framequeue.h"
struct SwsContext;
class QElapsedTimer;

namespace BugAV {

class VideoState {
public:
    VideoState(Define *def);
    ~VideoState();

    void init();
    ShowModeClock getMasterSyncType() const;
    double getMasterClock();

    void checkExternalClockSpeed();
    void setClockSpeed(Clock *c, double speed);

    void streamTogglePause();

    int isRealtime();

    void decoderAbort(Decoder *d, FrameQueue *fq);
    void vidDecoderAbort();
//    void flush();

//    void resetStream();

    void reset();
    void resetAudioStream();
    void resetVideoStream();

    bool isExternalClock() const;
    bool isVideoClock() const;
    bool isAudioClock() const;
public:
//    SDL_Thread *read_tid = nullptr;
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

    FrameQueue *pictq;
//    FrameQueue subpq;
    FrameQueue *sampq;

    Decoder auddec;
    Decoder viddec;
//    Decoder subdec;

    int audio_stream;

    ShowModeClock av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream *audio_st = nullptr;
    PacketQueue *audioq;
    int audio_hw_buf_size;
    uint8_t *audio_buf = nullptr;
    uint8_t *audio_buf1 = nullptr;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;    
    // disable audio feature
//    bool disableAudio;
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
//    SDL_Texture *vis_texture;
//    SDL_Texture *sub_texture;
//    SDL_Texture *vid_texture;

    int subtitle_stream;
    AVStream *subtitle_st = nullptr;
    PacketQueue subtitleq;

    double frame_timer;
    double frame_last_returned_time;
    double frame_last_filter_delay;
    int video_stream;
    AVStream *video_st = nullptr;
    PacketQueue *videoq;    
    double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    SwsContext *img_convert_ctx;
    struct SwsContext *sub_convert_ctx;
    int eof;

    QString fileUrl;
    int width, height, xleft, ytop;
    int step;

    bool useAVFilter; // need avfilter avframe

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph
#endif

    int last_video_stream, last_audio_stream, last_subtitle_stream;

    QWaitCondition *continue_read_thread = nullptr;
    int framedrop; // drop frame when cpu too slow.

    qint64 duration;

    bool audio_disable;

    bool noAudioStFound;

    int debug;

    void setIformat(AVInputFormat *value);

    void setSpeed(double value);

    double getSpeed() const;

    double lastPtsVideo;
    double firtsPtsVideo;

    QElapsedTimer *elLastEmptyRead;
    QMap<QString, QString> metadata;

private:
    Define *def;
    double speed;


};
}
#endif // VIDEOSTATE_H

