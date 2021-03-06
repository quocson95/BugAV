#include "videostate.h"
extern "C" {
#include "libavutil/time.h"
}

#include <QElapsedTimer>
#include <QUrl>
#include <QtGlobal>

namespace BugAV {
VideoState::VideoState(Define *def):
    img_convert_ctx{nullptr}
  ,audio_disable{false}
  ,debug{0}
{    
    #ifdef QT_DEBUG
      debug = 1;
    #else
      debug = 0;
    #endif
    this->def = def;
    pictq = new FrameQueue{def->FramePictQueueSize()};
    sampq = new FrameQueue{def->FrameSampQueueSize()};

    continue_read_thread = new QWaitCondition;
    show_mode = ShowMode::SHOW_MODE_VIDEO;    
    av_sync_type = ShowModeClock::AV_SYNC_EXTERNAL_CLOCK;
    videoq = new PacketQueue;
    audioq = new PacketQueue;
    useAVFilter = false;
    ic = nullptr;    
    seek_req  = 0;
    abort_request = 0;
    force_refresh = 0;
    paused = 0;
    last_paused = 0;
    queue_attachments_req = 0;
    seek_req = 0;
    seek_flags = 0 ;
    seek_pos = 0;
    seek_rel = 0;
    read_pause_return = 0;

    realtime = 0;

    audio_stream = -1;

    audio_clock = qQNaN();
    audio_clock_serial = 0;
    audio_diff_cum = 0.0; /* used for AV difference average computation */
    audio_diff_avg_coef = 0.0;
    audio_diff_threshold = 0.0;
    audio_diff_avg_count = 0;

    audio_hw_buf_size = 0;
    audio_buf_size = 0; /* in bytes */
    audio_buf1_size = 0;
    audio_buf_index = 0; /* in bytes */
    audio_write_buf_size = 0;
    audio_volume = 100;
    muted = true;
//    disableAudio = false;
    frame_drops_early = 0;
    frame_drops_late = 0;
    last_i_start = 0;
    rdft_bits = 0;
    xpos = 0;
    last_vis_time = 0;

    subtitle_stream = 0;

    frame_timer = 0.0;
    frame_last_returned_time = 0;
    frame_last_filter_delay = 0;
    video_stream = -1;
    max_frame_duration = 0.0;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity

    eof = 0;

    fileUrl = QLatin1String("");
    width = height = xleft = ytop = 0;
    step = 0;

    useAVFilter = false; // need avfilter avframe

    last_video_stream = last_audio_stream = last_subtitle_stream = -1;

    framedrop = 0;

    duration = 0;
    speed = 1.0;
    videoq->abort_request = 0;
    audioq->abort_request = 0;

    setSpeed(1.0);
//    PacketQueue::mustInitOnce();
    init();
    reset();
    elLastEmptyRead = new QElapsedTimer;
    elLastEmptyRead->invalidate();
}

VideoState::~VideoState()
{
    continue_read_thread->wakeAll();
    delete continue_read_thread;
    if (ic != nullptr) {
        avformat_close_input(&ic);
    }
    if (img_convert_ctx != nullptr) {
        sws_freeContext(img_convert_ctx);
    }
    delete elLastEmptyRead;

    viddec.clear();
    delete ic;
    // video
    delete pictq;  
    delete videoq;
    // audio    

    delete sampq;
    delete audioq;
}

void VideoState::init()
{
    // video
    viddec.init(videoq, nullptr, continue_read_thread);
    pictq->init(videoq, def->VideoPictureQueueSize(), 1);    
    vidclk.init(&videoq->serial);
    // audio
    auddec.init(audioq, nullptr, continue_read_thread);
    sampq->init(audioq, def->AudioQueueSize(), 1);
    audclk.init(&audioq->serial);
    audio_clock_serial = -1;
    extclk.init(&extclk.serial);
}

ShowModeClock VideoState::getMasterSyncType() const
{
    if (av_sync_type == ShowModeClock::AV_SYNC_VIDEO_MASTER) {
        if (video_st)
            return ShowModeClock::AV_SYNC_VIDEO_MASTER;
        else
            return ShowModeClock::AV_SYNC_AUDIO_MASTER;
    } else if (av_sync_type == ShowModeClock::AV_SYNC_AUDIO_MASTER) {
        if (audio_st)
            return ShowModeClock::AV_SYNC_AUDIO_MASTER;
        else
            return ShowModeClock::AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return ShowModeClock::AV_SYNC_EXTERNAL_CLOCK;
    }
}

double VideoState::getMasterClock()
{
    double val;
    auto syncClock = getMasterSyncType();
    switch (syncClock) {
        case ShowModeClock::AV_SYNC_VIDEO_MASTER:
            val = vidclk.get();
            break;
        case ShowModeClock::AV_SYNC_AUDIO_MASTER:
            val = audclk.get();
            break;
        default:
            val = extclk.get();
            break;
    }
    return val;
}

void VideoState::checkExternalClockSpeed()
{
    if ((this->video_stream >= 0 && this->videoq->nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) ||
           (this->audio_stream >= 0 && this->audioq->nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES))
    {
//        setClockSpeed(&this->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, this->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
        this->extclk.setSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, this->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
    } else if ((this->video_stream < 0 || this->videoq->nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
                  (this->audio_stream < 0 || this->audioq->nb_packets > EXTERNAL_CLOCK_MAX_FRAMES))
    {
//        setClockSpeed(&this->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, this->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
        this->extclk.setSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, this->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
    } else {
       double speed = this->extclk.speed;
       if (speed != 1.0)
//           setClockSpeed(&this->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
           this->extclk.setSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
    }
}

void VideoState::setClockSpeed(Clock *c, double speed)
{
    c->set(c->get(), c->serial);
    c->speed = speed;
}

void VideoState::streamTogglePause()
{
    if (this->paused) {
        this->frame_timer += av_gettime_relative() / 1000000.0 - this->vidclk.last_updated;
        if (this->read_pause_return != AVERROR(ENOSYS)) {
            this->vidclk.paused = 0;
        }
        this->vidclk.set(this->vidclk.get(), this->vidclk.serial);
    }
    this->extclk.set(this->extclk.get(), this->extclk.serial);
    this->paused = this->vidclk.paused = this->audclk.paused = this->extclk.paused = !this->paused;
}

int VideoState::isRealtime()
{
//    if (ic == nullptr || ic->iformat == nullptr) {
//        return 0;
//    }
//    if(   !strcmp(ic->iformat->name, "rtp")
//       || !strcmp(ic->iformat->name, "rtsp")
//       || !strcmp(ic->iformat->name, "sdp")
//        || !strcmp(ic->iformat->name, "rtmp")
//    )
//        return 1;

//    if(ic->pb && (   !strncmp(ic->url, "rtp:", 4)
//                 || !strncmp(ic->url, "udp:", 4)
//                )
//    )
    auto u = QUrl{fileUrl};
    localFile = u.isLocalFile();
    if (fileUrl.startsWith("rtp")
            || fileUrl.startsWith("rtsp")
            || fileUrl.startsWith("sdp")
            || fileUrl.startsWith("rtmp")) {
        return 1;
    }
    return 0;
}

bool VideoState::isLocalFile()
{
    return localFile;
}

void VideoState::decoderAbort(Decoder *d, FrameQueue *fq)
{
    d->queue->abort();
    fq->wakeSignal();
}

void VideoState::vidDecoderAbort()
{
    decoderAbort(&viddec, pictq);
    decoderAbort(&auddec, sampq);
}

//void VideoState::flush()
//{
//    videoq->flush();
//}

void VideoState::reset()
{
    resetAudioStream();
    resetVideoStream();
    eof = 0;
//    framedrop = 0; // drop frame when cpu too slow.

//    //init picture frame queue
//    pictq->init(videoq, def->VideoPictureQueueSize(), 1);

//    //init package queue
//    videoq->init();

//    // init audio frame queue
//    sampq->init(audioq, def->AudioQueueSize(), 1);

//    // init audio package queue
//    audioq->init();

//    //init video clock
//    vidclk.init(&videoq->serial);

//    // init audio clock
//    audclk.init(&audioq->serial);
    init();

    if (img_convert_ctx != nullptr) {
        sws_freeContext(img_convert_ctx);
        img_convert_ctx = nullptr;
    }
}

bool VideoState::isExternalClock() const
{
    return getMasterSyncType() == ShowModeClock::AV_SYNC_EXTERNAL_CLOCK;
}

bool VideoState::isVideoClock() const
{
    return getMasterSyncType() == ShowModeClock::AV_SYNC_VIDEO_MASTER;
}

bool VideoState::isAudioClock() const
{
    return getMasterSyncType() == ShowModeClock::AV_SYNC_AUDIO_MASTER;
}

void VideoState::setIformat(AVInputFormat *value)
{
    iformat = value;
    realtime = isRealtime();
}

void VideoState::setSpeed(double value)
{
    pictq->speed = value;
    sampq->speed = value;
    speed = value;
}

double VideoState::getSpeed() const
{
    return speed;
}

void VideoState::resetAudioStream()
{
    last_audio_stream = audio_stream = -1;
    audio_st = nullptr;
    auddec.freeAvctx();
}

void VideoState::resetVideoStream()
{
    last_video_stream = video_stream = -1;
    video_st = nullptr;
    viddec.freeAvctx();
}
}
