#include "videostate.h"
extern "C" {
#include "libavutil/time.h"
}
VideoState::VideoState()
{    
    continue_read_thread = new QWaitCondition;
    show_mode = ShowMode::SHOW_MODE_VIDEO;    
    av_sync_type = ShowModeClock::AV_SYNC_VIDEO_MASTER;
    videoq = new PacketQueue;
    init();
    reset();
}

VideoState::~VideoState()
{
    continue_read_thread->wakeAll();
    delete continue_read_thread;
    if (ic != nullptr) {
        avformat_close_input(&ic);
    }
    sws_freeContext(img_convert_ctx);
    delete videoq;
}

void VideoState::init()
{
    viddec.init(videoq, nullptr, continue_read_thread);
    pictq.init(videoq, VIDEO_PICTURE_QUEUE_SIZE, 1);
    vidclk.init(videoq->serial);    
}

ShowModeClock VideoState::getMasterSyncType()
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
    if (this->video_stream >= 0 && this->videoq->nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
           this->audio_stream >= 0 && this->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
           setClockSpeed(&this->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, this->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
       } else if ((this->video_stream < 0 || this->videoq->nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
                  (this->audio_stream < 0 || this->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
           setClockSpeed(&this->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, this->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
       } else {
           double speed = this->extclk.speed;
           if (speed != 1.0)
               setClockSpeed(&this->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
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
    if (ic != nullptr && ic->iformat == nullptr) {
        return 0;
    }
    if(   !strcmp(ic->iformat->name, "rtp")
       || !strcmp(ic->iformat->name, "rtsp")
       || !strcmp(ic->iformat->name, "sdp")
        || !strcmp(ic->iformat->name, "rtmp")
    )
        return 1;

    if(ic->pb && (   !strncmp(ic->url, "rtp:", 4)
                 || !strncmp(ic->url, "udp:", 4)
                )
    )
        return 1;
    return 0;
}

void VideoState::decoderAbort(Decoder *d, FrameQueue *fq)
{
    d->queue->abort();
    fq->wakeSignal();
}

void VideoState::vidDecoderAbort()
{
    decoderAbort(&viddec, &pictq);
}

void VideoState::flush()
{
    videoq->flush();
}

void VideoState::resetStream()
{
    video_stream = -1;
    video_st = nullptr;
}

void VideoState::reset()
{
    resetStream();
    framedrop = 1; // drop frame when cpu too slow.
    vidclk.init(videoq->serial);
    abort_request = 0;
    force_refresh = 0;
    paused = 0;
    last_paused = 0;
    step = 0;
    videoq->abort_request = 0;
    eof = 0;
}

