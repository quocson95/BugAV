#ifndef DEFINE_H
#define DEFINE_H
#include <QtGlobal>
#include "common/global.h"

const char program_name[] = "ffplay";
const int program_birth_year = 2003;

constexpr qint64 MAX_QUEUE_SIZE = (15 * 1024 * 1024);
constexpr qint64 MAX_QUEUE_SIZE_VOD = (45 * 1024 * 1024); // 45Mb

constexpr int MIN_FRAMES = 25;
constexpr int EXTERNAL_CLOCK_MIN_FRAMES = 2;
constexpr int EXTERNAL_CLOCK_MAX_FRAMES = 10;

/* Minimum SDL audio buffer size, in samples. */
constexpr int SDL_AUDIO_MIN_BUFFER_SIZE = 512;
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
constexpr int SDL_AUDIO_MAX_CALLBACKS_PER_SEC = 30;

/* Step size for volume control in dB */
constexpr double SDL_VOLUME_STEP = (0.75);

/* no AV sync correction is done if below the minimum AV sync threshold */
constexpr double AV_SYNC_THRESHOLD_MIN = 0.04;
/* AV sync correction is done if above the maximum AV sync threshold */
constexpr double AV_SYNC_THRESHOLD_MAX = 0.1;
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
constexpr double AV_SYNC_FRAMEDUP_THRESHOLD = 0.1;
/* no AV correction is done if too big error */
constexpr double AV_NOSYNC_THRESHOLD  = 10.0;

/* maximum audio speed change to get correct sync */
constexpr int SAMPLE_CORRECTION_PERCENT_MAX = 10;

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
constexpr double EXTERNAL_CLOCK_SPEED_MIN  = 0.900;
constexpr double EXTERNAL_CLOCK_SPEED_MAX  = 1.010;
constexpr double EXTERNAL_CLOCK_SPEED_STEP = 0.001;

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
constexpr int AUDIO_DIFF_AVG_NB = 20;

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
constexpr double REFRESH_RATE  = 0.01;

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
constexpr int SAMPLE_ARRAY_SIZE = (8 * 65536);

constexpr int CURSOR_HIDE_DELAY = 1000000;

constexpr int USE_ONEPASS_SUBTITLE_RENDER = 1;

constexpr int VOD_MUL = 32;

constexpr int VIDEO_PICTURE_QUEUE_SIZE = 3;
constexpr int VIDEO_PICTURE_QUEUE_SIZE_VOD = VIDEO_PICTURE_QUEUE_SIZE * VOD_MUL;

constexpr int SUBPICTURE_QUEUE_SIZE = 16;
constexpr int SUBPICTURE_QUEUE_SIZE_VOD = 16 * VOD_MUL;

constexpr int SAMPLE_QUEUE_SIZE = 9;
constexpr int SAMPLE_QUEUE_SIZE_VOD = SAMPLE_QUEUE_SIZE * VOD_MUL;

constexpr int FRAME_QUEUE_SIZE =
        qMax(SAMPLE_QUEUE_SIZE, qMax(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE));

constexpr int FRAME_QUEUE_SIZE_VOD =
        qMax(SAMPLE_QUEUE_SIZE_VOD, qMax(VIDEO_PICTURE_QUEUE_SIZE_VOD, SUBPICTURE_QUEUE_SIZE_VOD));


//#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

#define AVCODEC_STATIC_REGISTER FFMPEG_MODULE_CHECK(LIBAVCODEC, 58, 10, 100)
#define AVFORMAT_STATIC_REGISTER FFMPEG_MODULE_CHECK(LIBAVFORMAT, 58, 9, 100)

constexpr int MIX_MAX_VOLUME = 128;

constexpr int NEXT_NB_CHANNELS[] = {0, 0, 1, 6, 2, 6, 4, 6};
constexpr int NEXT_SAMPLE_RATES[] = {0, 44100, 48000, 96000, 192000};


#ifdef Q_OS_LINUX
    using LONG = int;
    using CHAR = char;
    using SHORT = short;
    #ifndef VOID
    using VOID = void;
    #endif

    using DWORD = unsigned int;
    #if !defined(MIDL_PASS)
    typedef int INT;
    #endif
#else
#include "windows.h"
using PLAYM4_HWND = HWND;
#endif

namespace BugAV {
class Define {
public:
    Define();
    ~Define() = default;

    void setModePlayer(ModePlayer mode);
    ModePlayer getModePlayer() const;

    bool isInModeRealTime() const;
    bool isInModeVOD() const;

    qint64 MaxQueueSize() const;
    int VideoPictureQueueSize() const;
    qint64 FramePictQueueSize() const;

    qint64 FrameSampQueueSize() const;
    qint64 AudioQueueSize() const;

private:
    //    int liveMode;
    ModePlayer modePlayer;
};
}
#endif // DEFINE_H

