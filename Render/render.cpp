#include "render.h"
#include "QDebug"
#include "bugfilter.h"
extern "C" {
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"

}
#include <QImage>
#include <QThread>
#include <QThreadPool>
#include "common/videostate.h"

#include "RenderOuput/IBugAVRenderer.h"
#include <RenderOuput/ibugavdefaultrenderer.h>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QElapsedTimer>

#define RNDTO2(X) ( ( (X) & 0xFFFFFFFE ))
#define RNDTO32(X) ( ( (X) % 32 ) ? ( ( (X) + 32 ) & 0xFFFFFFE0 ) : (X) )


namespace BugAV {
Render::Render()
    :Render(nullptr, nullptr)
{
    requestStop = false;
    hasInit = false;
    isRun = false;
    privState = PrivState::Stop;
}


Render::Render(VideoState *is, IBugAVRenderer *renderer)
    :QObject(nullptr)
//    ,QRunnable()
    ,renderer{renderer}
    ,is{is}
    ,elPrevFrame{nullptr}
//    ,picture{nullptr}
{    
    defaultRenderer = new IBugAVDefaultRenderer;
    elTimer = nullptr;

    if (renderer == nullptr) {
        renderer = defaultRenderer;
    }
    thread = new QThread{this};
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
//    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    timerCheckNoFrameRender = new QTimer{};
    connect(timerCheckNoFrameRender, &QTimer::timeout, [=]()->void {
        if (this->lastUpdateFrame == 0) {
            return;
        }
        auto now = QDateTime::currentMSecsSinceEpoch();
        if (now - this->lastUpdateFrame > 10000) {
            emit noRenderNewFrameLongTime();
        }
    });
    preferPixFmt = AVPixelFormat::AV_PIX_FMT_NONE;
    img = nullptr;    
}

Render::~Render()
{

//    if (filter != nullptr) {
//        delete filter;
//    }

        qDebug() << "Destroy Render";
    stop();
//    av_frame_free(&frameYUV);
//    delete out_buffer;

    if (defaultRenderer != nullptr)  {
        delete  defaultRenderer;
    }
    if (img != nullptr) {
        delete img;
    }
    thread->deleteLater();
    timerCheckNoFrameRender->deleteLater();
//    delete thread;

}

void Render::start()
{
//    qDebug() << "!!!Render Thread start";
    if (thread->isRunning()) {
        return;
    }
    lastUpdateFrame = 0;
    requestStop = false;
    privState = PrivState::WaitingFirstFrame;
    lastVideoRefresh = 0;
    remaining_time = 0;
    freeSwsBuff();
    timerCheckNoFrameRender->start(2000);

    thread->start();
}

void Render::stop()
{
//    qDebug() << "!!!Render Thread stop";
    timerCheckNoFrameRender->stop();
    requestStop = true;   
    isRun = false;
    privState = PrivState::Stop;
    thread->quit();
    do {
        thread->wait(500);
    } while(thread->isRunning());
}

void Render::setIs(VideoState *value)
{
    is = value;
}

double Render::vp_duration(double maxFrameDuration, Frame *vp, Frame *nextvp)
{
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0.0 || duration > maxFrameDuration) {
            return vp->duration;
        } else {
            return duration;
        }
    }
    return 0.0;
}

double Render::compute_target_delay(VideoState *is, double delay)
{
    double sync_threshold, diff = 0.0;
    /* update delay to follow master synchronisation source */
    if (!is->isVideoClock()) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = is->vidclk.get() - is->getMasterClock();

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!std::isnan(diff) && std::fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }
//    qDebug() << "video delay " << delay << " diff " << -diff;
    return delay;
}

QImage::Format Render::avFormatToImageFormat(int format)
{
//    auto f = static_cast<AVPixelFormat>(format);
//    for (int i = 0; qpixfmt_map[i].fmt != AV_PIX_FMT_NONE; ++i) {
//        if (qpixfmt_map[i].fmt == f)
//            return qpixfmt_map[i].qfmt;
//    }
    return QImage::Format_Invalid;
}

AVPixelFormat Render::fixDeprecatedPixelFormat(AVPixelFormat fmt)
{
    AVPixelFormat pixFormat;
    switch (fmt) {
    case AV_PIX_FMT_YUVJ420P :
        pixFormat = AV_PIX_FMT_YUV420P;
        break;
    case AV_PIX_FMT_YUVJ422P  :
        pixFormat = AV_PIX_FMT_YUV422P;
        break;
    case AV_PIX_FMT_YUVJ444P   :
        pixFormat = AV_PIX_FMT_YUV444P;
        break;
    case AV_PIX_FMT_YUVJ440P :
        pixFormat = AV_PIX_FMT_YUV440P;
        break;
    default:
        pixFormat = fmt;
        break;
    }
    return pixFormat;
}

//int Render::getSDLPixFmt(int format)
//{
//    int i;
//    auto sdl_pix_fmt = (int)SDL_PIXELFORMAT_UNKNOWN;
//    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
//        if (format == sdl_texture_format_map[i].format) {
//            sdl_pix_fmt = (int)sdl_texture_format_map[i].texture_fmt;
//            break;
//        }
//    }
//    return  sdl_pix_fmt;
//}

void Render::updateVideoPts(double pts, int64_t pos, int serial)
{
    is->vidclk.set(pts, serial);
    is->extclk.syncToSlave(&is->vidclk);
}

void Render::uploadTexture(Frame *f, SwsContext **img_convert_ctx)
{    
//    qDebug() << " upload Texture " << QThread::currentThreadId();
    if (f->frame == nullptr) {
        return;
    }

    auto frame = f->frame;
    if (frame->width == 0
            ||frame->height == 0
            ) {
        return;
    }

    if (lastUpdateFrame == 0) {
        emit firstFrameComming();
    }
    lastUpdateFrame = QDateTime::currentMSecsSinceEpoch();

    if (renderer == nullptr || renderer == defaultRenderer) {
        return;
    }
    // reduce using too much gpu, limit maximum at 100fps
    if (!elPrevFrame->hasExpired(10)) {
//        qDebug() << "update img too fast. ignore";
        return;
    }
    elPrevFrame->restart();

    if (!initSwsBuff) {
        initSwsBuff = true;
        auto ret = av_image_alloc(dst_data, dst_linesize, frame->width, frame->height, AV_PIX_FMT_RGB32, 1);
        if (ret < 0) {
            qDebug() << "Init sws data error ";
            initSwsBuff =  false;
        }
    }

    auto fmt = fixDeprecatedPixelFormat(AVPixelFormat(is->viddec.avctx->pix_fmt));
    frame->format = fmt;
    auto ret = 0;
    AVFrame *dstFrame = frame;
    if (this->preferPixFmt == AVPixelFormat::AV_PIX_FMT_NONE && fmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
//        // do nothing.
//        // native render yuv420p
    } else if (fmt != AVPixelFormat::AV_PIX_FMT_RGB32) {
//         todo native renderer non yuv 420p
        *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
                    is->viddec.avctx->width, is->viddec.avctx->height, fmt,
                    is->viddec.avctx->width, is->viddec.avctx->height, AV_PIX_FMT_RGB32,
                    sws_flags,nullptr, nullptr, nullptr);

        if (*img_convert_ctx) {
            ret = sws_scale(*img_convert_ctx  ,frame->data, frame->linesize, 0,
                            is->viddec.avctx->height, dst_data, dst_linesize);

        }
        for (auto i = 0; i < 3; i++) {
            dstFrame->data[i] = dst_data[i];
            dstFrame->linesize[i] = dst_linesize[i];
        }
        dstFrame->width = frame->width;
        dstFrame->height = frame->height;
        dstFrame->format = AV_PIX_FMT_RGB32;
    }

    if (ret >=0) {
        if (renderer != nullptr ) {
            renderer->updateData(dstFrame);
        }
    }
    av_frame_unref(frame);
}

//AVFrame *Render::cropImage(AVFrame *in, int left, int top, int right, int bottom)
//{
////    return in;
//    if (filter->init(in, is) < 0) {
//        return in;
//    }
//    if (filter->pushFrame(in) < 0) {
//        qDebug() << "Push frame filter error ";
//        return in;
//    }
//    auto out = filter->pullFrame();
//    return out;
//}

void Render::process()
{    
    isRun = true;
    qDebug() << "!!!Render Thread start";
//    emit started();

    elPrevFrame = new QElapsedTimer;
    elPrevFrame->start();
    forever {
        if (requestStop) {
//            emit stopped();
            break;
        }
        if (is->pictq->queueNbRemain() == 0) {
            thread->msleep(50);
            continue;
        } else {
            if (is->viddec.avctx != nullptr) {
                initPriv();
                break;
            } else {
                continue;
            }
        }
    }
    remaining_time = 0.0;

    if (elTimer == nullptr) {
        elTimer = new QElapsedTimer;
        elTimer->start();
    }
    if (!requestStop) {
        forever {
            if (requestStop) {
                break;
            }
            if (remaining_time > 0.0) {
//                qDebug() << "Remain time " << remaining_time << " render need sleep";
                thread->usleep(static_cast<unsigned long>(remaining_time * 1000000.0));
            }
            if (requestStop) {
                break;
            }
            remaining_time = REFRESH_RATE;
            if (is->show_mode != ShowMode::SHOW_MODE_NONE && (!is->paused || is->force_refresh)){
                videoRefresh();
            }
        }
    }
    delete elTimer;
    elTimer = nullptr;
//    emit stopped();
    elPrevFrame->invalidate();
    delete  elPrevFrame;
    elPrevFrame = nullptr;
    isRun = false;
    qDebug() << "!!!Render Thread exit";
    thread->terminate();
}

void Render::videoRefresh()
{
//    qDebug() << " video refresh";

//    Frame *sp, *sp2;
    if (!is->paused && is->isExternalClock() && is->realtime) {
            is->checkExternalClockSpeed();
    }

    // show video
//    auto time = av_gettime_relative() / 1000000.0;
//    if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
//        videoDisplay();
//        is->last_vis_time = time;
//    }
//    remaining_time = FFMIN(remaining_time, is->last_vis_time + rdftspeed - time);
    if (is->video_st == nullptr) {
        is->force_refresh = 0;
        return;
    }

    forever{
        if (is->pictq->queueNbRemain() == 0) {
            // nothing to do, no picture to display in the queue
        } else {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;
            lastvp = is->pictq->peekLast();
            vp = is->pictq->peek();

            if (vp->serial != is->videoq->serial) {
                is->pictq-> queueNext();
                continue;
            }

            if (lastvp->serial != vp->serial) {
                is->frame_timer = av_gettime_relative() / 1000000.0;
            }

            if (is->paused) {
                break;
            }
            /* compute nominal last_duration */
            last_duration = vp_duration(is->max_frame_duration, lastvp, vp);
            delay = compute_target_delay(is, last_duration);

            auto time = av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                remaining_time = FFMIN(is->frame_timer + delay - time, remaining_time);
                break;
            }
            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX) {
                is->frame_timer = time;
            }
            is->pictq->mutex.lock();
            if (!std::isnan(vp->pts)) {   
                updateVideoPts(vp->pts, vp->pos, vp->serial);
                updatePositionChanged();
            }
            is->pictq->mutex.unlock();

            if (is->pictq->queueNbRemain() > 1) {
                Frame *nextvp = is->pictq->peekNext();
                duration = vp_duration(is->max_frame_duration, vp, nextvp);
                if(!is->step && (is->framedrop > 0 || (is->framedrop && !is->isVideoClock())) && time > is->frame_timer + duration){
//                    qDebug() << "Drop frame";
                   is->frame_drops_late++;
                   is->pictq->queueNext();
                   continue;
                }
            }
            is->pictq->queueNext();
            is->force_refresh = 1;
            if (is->step && !is->paused) {
                is->streamTogglePause();
            }
        }
        break;
    }
    if (is->force_refresh && is->show_mode == ShowMode::SHOW_MODE_VIDEO && is->pictq->rindex_shown) {
        videoDisplay();
    }
    is->force_refresh = 0;
    if (0) {
            static int64_t last_time;
            int64_t cur_time;
            int aqsize, vqsize, sqsize;
            double av_diff;

            cur_time = av_gettime_relative();
            if (!last_time || (cur_time - last_time) >= 30000) {
                aqsize = 0;
                vqsize = 0;
                sqsize = 0;
                if (is->audio_st)
                    aqsize = is->audioq.size;
                if (is->video_st)
                    vqsize = is->videoq->size;
                if (is->subtitle_st)
                    sqsize = is->subtitleq.size;
                av_diff = 0;
                if (is->audio_st && is->video_st)
                    av_diff = is->audclk.get() - is->vidclk.get();
                else if (is->video_st)
                    av_diff = is->getMasterClock() - is->vidclk.get();
                else if (is->audio_st)
                    av_diff = is->getMasterClock() - is->audclk.get();
                av_log(NULL, AV_LOG_INFO,
                       "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64 "/%" PRId64 " %4d  \r",
                       is->getMasterClock(),
                       (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
                       av_diff,
                       is->frame_drops_early + is->frame_drops_late,
                       aqsize / 1024,
                       vqsize / 1024,
                       sqsize,
                       is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
                       is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0,
                       is->videoq->nb_packets);

                fflush(stdout);
                last_time = cur_time;
            }
        }
}

void Render::videoDisplay()
{
//    qDebug() << "video display";
    Frame *vp = is->pictq->peekLast();
    if (!vp->uploaded) {
        uploadTexture(vp, &is->img_convert_ctx);
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

}

void Render::updatePositionChanged()
{
    if (elTimer->hasExpired(1000)) {
        if (is->seek_req) {
            return;
        }
        auto ts = is->getMasterClock() * AV_TIME_BASE * is->speed - is->ic->start_time;
        emit positionChanged(ts);        
        elTimer->restart();
    }
}

void Render::setPreferPixFmt(const AVPixelFormat &value)
{
    preferPixFmt = value;
}

void Render::setSaveRawImage(bool value)
{
    saveRawImage = value;
}

void Render::run()
{
//    qDebug() << "Run " << int(privState);
    if (requestStop) {
        return;
    }
    switch (privState) {
    case PrivState::Stop: {
        privState = PrivState::WaitingFirstFrame;
        break;
    }
    case PrivState::WaitingFirstFrame: {
        if (this->isAvailFirstFrame()) {
            privState = PrivState::Init;
        }
        break;
    }
    case PrivState::Init: {
        if (initPriv() > 0) {
            privState = PrivState::HandleFrameState1;
        }
        break;
    }
    case PrivState::HandleFrameState1: {
        if (handlerFrameState1()) {
            privState = PrivState::HandleFrameState2;
        }
        break;
    }
    case PrivState::HandleFrameState2: {
        if (handlerFrameState2()) {
            privState = PrivState::HandleFrameState3;
        } else {
            privState = PrivState::HandleFrameState1;
        }
        break;
    }
    case PrivState::HandleFrameState3: {
       auto ret = handlerFrameState3();
        if (ret > 0 ) {
            if (is->force_refresh && is->show_mode == ShowMode::SHOW_MODE_VIDEO && is->pictq->rindex_shown) {
                videoDisplay();
            }
            privState = PrivState::HandleFrameState1;
        } else if (ret == 0) {
            privState = PrivState::HandleFrameState3;
        } else {
            privState = PrivState::HandleFrameState1;
        }
        is->force_refresh = 0;
        break;
    }
    }
}

bool Render::initPriv()
{
    if (!hasInit) {
        if (is->viddec.avctx == nullptr) {
            return  hasInit;
        }
        remaining_time = 0.0;        
//        frameYUV = nullptr;
//        frameYUV = av_frame_alloc();
//         out_buffer= new uint8_t[avpicture_get_size(is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height)];
//         avpicture_fill((AVPicture *)frameYUV, out_buffer, is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height);
 //        int avpicture_fill(AVPicture *picture, const uint8_t *ptr,
 //                           enum AVPixelFormat pix_fmt, int width, int height);
//         av_image_fill_arrays(frameYUV->data, frameYUV->linesize,  out_buffer, is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height, 0);
//        if (defaultRenderer == nullptr) {
//            defaultRenderer = new IBugAVDefaultRenderer;
//        }
        if (renderer == nullptr) {
            renderer = defaultRenderer;
        }
        hasInit = true;
    }
    return hasInit;
}

bool Render::isAvailFirstFrame()
{
     auto avail = (is->pictq->queueNbRemain() > 0);
     return  avail;
}

void Render::freeSwsBuff()
{
    if (initSwsBuff == true) {
        av_freep(dst_data);
        initSwsBuff = false;
    }
}

bool Render::handlerFrameState1()
{
//    if (remaining_time > 0.0) {
//            qDebug() << "Remain time " << remaining_time << " render need sleep";
//        thread->usleep(static_cast<unsigned long>(remaining_time * 1000000.0));
//        return true;
//    }
    auto now = QDateTime::currentMSecsSinceEpoch();
    if (remaining_time > 0.0) {
        auto delay = now - lastVideoRefresh;
        if (delay  < remaining_time) {
            return false;
        }
    }
    lastVideoRefresh = now;
    remaining_time = REFRESH_RATE;    
    if (is->show_mode != ShowMode::SHOW_MODE_NONE && (!is->paused || is->force_refresh)){
//        videoRefresh();
        lastVideoRefresh = now;
        return true;
    }
    return false;
}

bool Render::handlerFrameState2()
{

//    Frame *sp, *sp2;
    if (!is->paused && is->getMasterSyncType() == ShowModeClock::AV_SYNC_EXTERNAL_CLOCK && is->realtime) {
        is->checkExternalClockSpeed();
    }
    // show video
//    auto time = av_gettime_relative() / 1000000.0;
//    if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
//        videoDisplay();
//        is->last_vis_time = time;
//    }
//    remaining_time = FFMIN(remaining_time, is->last_vis_time + rdftspeed - time);

    if (is->video_st == nullptr) {
        is->force_refresh = 0;
        return false;
    }
    return true;
}

int Render::handlerFrameState3()
{
    if (is->pictq->queueNbRemain() == 0 ) {
        return 1;
    }    
    if (is->pictq->queueNbRemain() > 0) {
        double last_duration, duration, delay;
        Frame *vp, *lastvp;
        lastvp = is->pictq->peekLast();
        vp = is->pictq->peek();

        if (vp->serial != is->videoq->serial) {
            is->pictq->queueNext();
            return  0;
        }

        if (lastvp->serial != vp->serial) {
            is->frame_timer = av_gettime_relative() / 1000000.0;
        }

        if (is->paused) {
            return 1;
        }
        /* compute nominal last_duration */
        last_duration = vp_duration(is->max_frame_duration, lastvp, vp);
        delay = compute_target_delay(is, last_duration);

        auto time = av_gettime_relative()/1000000.0;
        if (time < is->frame_timer + delay) {
            remaining_time = FFMIN(is->frame_timer + delay - time, remaining_time);
            return 1;
        }
        is->frame_timer += delay;
        if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX) {
            is->frame_timer = time;
        }
        is->pictq->mutex.lock();
        if (!isnan(vp->pts)) {
            updateVideoPts(vp->pts, vp->pos, vp->serial);
        }
        is->pictq->mutex.unlock();
        if (is->pictq->queueNbRemain() > 1) {
            Frame *nextvp = is->pictq->peekNext();
            duration = vp_duration(is->max_frame_duration, vp, nextvp);
            if(!is->step && (is->framedrop>0 || (is->framedrop && is->getMasterSyncType() != ShowModeClock::AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration){
                qDebug() << "drop frame";
               is->frame_drops_late++;
               is->pictq->queueNext();
               return 0;
            }
        }
        is->pictq->queueNext();
        is->force_refresh = 1;
        if (is->step && !is->paused) {
            is->streamTogglePause();
        }
    }
//    is->force_refresh = 0;
    return 1;
}

bool Render::getRequestStop() const
{
    return requestStop;
}

void Render::setRequestStop(bool value)
{
    requestStop = value;
}

bool Render::isRunning() const
{
//    return isRun;
    return thread->isRunning();
}

QString Render::statistic()
{
    auto s = QString("Last render %1").arg(QDateTime::fromMSecsSinceEpoch(lastUpdateFrame).toString());
    return s;
}

void Render::setRenderer(IBugAVRenderer *value)
{
//    mutex.lock();
    if (value == nullptr) {
        renderer = defaultRenderer;
    } else {
        renderer = value;
    }
//    mutex.unlock();
}

IBugAVRenderer *Render::getRenderer()
{
    if (renderer != defaultRenderer) {
        return renderer;
    }
    return nullptr;
}

}
