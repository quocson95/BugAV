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

#include "RenderOuput/IRenderer.h"
Render::Render()
    :Render(nullptr, nullptr)
{
    requestStop = false;
    hasInit = false;
}


Render::Render(IRenderer *renderer, VideoState *is)
    :QObject()
    ,renderer{renderer}
    ,is{is}
//    ,picture{nullptr}
{
    thread = new QThread{this};
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
//    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));    
}

Render::~Render()
{
    //    delete thread;
    if (filter != nullptr) {
        delete filter;
    }

    av_frame_free(&frameYUV);
    delete out_buffer;
}

void Render::start()
{
    thread->start();
}

void Render::stop()
{
    requestStop = true;
    thread->quit();
    do {
        thread->wait(100);
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
        if (qIsNaN(duration) || duration <= 0 || duration > maxFrameDuration) {
            return vp->duration;
        } else {
            return duration;
        }
    }
    return 0.0;
}

double Render::compute_target_delay(VideoState *is, double delay)
{
    double sync_threshold, diff = 0;
    /* update delay to follow master synchronisation source */
    if (is->getMasterSyncType() != ShowModeClock::AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = is->vidclk.get() - is->getMasterClock();

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
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
    return fmt;
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
    if (renderer == nullptr) {
        return;
    }
    auto frame = f->frame;
    auto fmt = (AVPixelFormat(frame->format));
    *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
                frame->width, frame->height, fmt,
                frame->width, frame->height, AV_PIX_FMT_YUV420P, sws_flags,
                nullptr, nullptr, nullptr);

    sws_scale(*img_convert_ctx, frame->data, frame->linesize, 0,
                          frame->height, frameYUV->data, frameYUV->linesize);
    auto fCrop = cropImage(frameYUV, 10, 10, 200, 200);
    if (fCrop != nullptr) {
        renderer->initShader(frame->width, frame->height);
        renderer->updateData(fCrop->data);
//        av_frame_unref(fCrop);
    }
//    av_frame_free(&fCrop);
}

AVFrame *Render::cropImage(AVFrame *in, int left, int top, int right, int bottom)
{
    return in;
    if (filter->pushFrame(in) < 0) {
        qDebug() << "Push frame filter error ";
        return in;
    }
    auto out = filter->pullFrame();    
    return out;
}

void Render::process()
{
    initPriv();
    qDebug() << "!!!Render Thread start";
    emit started();
    forever {
        if (requestStop) {
            break;
        }
        if (is->pictq.queueNbRemain() == 0) {
            thread->msleep(50);
            continue;
        } else {
            // init filter
            if (filter->getStatusInit() < 0) {
                filter->init(is->viddec.avctx);
            }
        }
        break;
    }
    if (requestStop) {
        emit stopped();
        return;
    }

    forever {
        if (requestStop) {
            break;
        }        
        if (remaining_time > 0.0) {
//            qDebug() << "Remain time " << remaining_time << " render need sleep";
            thread->usleep(static_cast<unsigned long>(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;        
        if (is->show_mode != ShowMode::SHOW_MODE_NONE && (!is->paused || is->force_refresh)){
            videoRefresh();            
        }
    }
    emit stopped();
    qDebug() << "!!!Render Thread exit";
    thread->quit();
}

void Render::videoRefresh()
{
//    qDebug() << " video refresh";
    if (is->video_st == nullptr) {
        return;
    }
//    Frame *sp, *sp2;
    if (!is->paused && is->getMasterSyncType() == ShowModeClock::AV_SYNC_EXTERNAL_CLOCK && is->realtime) {
            is->checkExternalClockSpeed();
    }
    // show video
    auto time = av_gettime_relative() / 1000000.0;
    if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
        videoDisplay();
        is->last_vis_time = time;
    }
    remaining_time = FFMIN(remaining_time, is->last_vis_time + rdftspeed - time);
    forever{
        if (is->pictq.queueNbRemain() > 0) {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;
            lastvp = is->pictq.peekLast();
            vp = is->pictq.peek();

            if (vp->serial != is->videoq->serial) {
                is->pictq.queueNext();
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

            time = av_gettime_relative()/1000000.0;
            if (time < is->frame_timer + delay) {
                remaining_time = FFMIN(is->frame_timer + delay - time, remaining_time);
                break;
            }
            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX) {
                is->frame_timer = time;
            }
            is->pictq.mutex.lock();
            if (!isnan(vp->pts)) {
                updateVideoPts(vp->pts, vp->pos, vp->serial);
            }
            is->pictq.mutex.unlock();
            if (is->pictq.queueNbRemain() > 1) {
                Frame *nextvp = is->pictq.peekNext();
                duration = vp_duration(is->max_frame_duration, vp, nextvp);
                if(!is->step && (is->framedrop>0 || (is->framedrop && is->getMasterSyncType() != ShowModeClock::AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration){
                   is->frame_drops_late++;
                   is->pictq.queueNext();
                   continue;
                }
            }
            is->pictq.queueNext();
            is->force_refresh = 1;
            if (is->step && !is->paused) {
                is->streamTogglePause();
            }
        }
        break;
    }
    if (is->force_refresh && is->show_mode == ShowMode::SHOW_MODE_VIDEO && is->pictq.rindex_shown) {
        videoDisplay();
    }
    is->force_refresh = 0;
}

void Render::videoDisplay()
{
//    qDebug() << "video display";
    Frame *vp = is->pictq.peekLast();
    if (!vp->uploaded) {
        uploadTexture(vp, &is->img_convert_ctx);
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

}

void Render::initPriv()
{
    if (!hasInit) {
        remaining_time = 0.0;
        filter = new BugFilter;
        hasInit = true;
        frameYUV = av_frame_alloc();
        out_buffer= new uint8_t[avpicture_get_size(is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height)];
//        avpicture_fill((AVPicture *)frameYUV, out_buffer, is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height);
//        int avpicture_fill(AVPicture *picture, const uint8_t *ptr,
//                           enum AVPixelFormat pix_fmt, int width, int height);
        av_image_fill_arrays(frameYUV->data, frameYUV->linesize,  out_buffer, is->viddec.avctx->pix_fmt, is->viddec.avctx->width, is->viddec.avctx->height, 1);
    }
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
    return thread->isRunning();
}

void Render::setRenderer(IRenderer *value)
{
    renderer = value;    
}
