#include "render.h"
#include "QDebug"
extern "C" {
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"

}
#include <QImage>
#include <QThread>
#include "videostate.h"
#include "bugglwidget.h"
Render::Render()
    :Render(nullptr, nullptr)
{
    requestStop = false;
}


Render::Render(IRenderer *renderer, VideoState *is)
    :QObject(nullptr)
    ,renderer{renderer}
    ,is{is}
//    ,picture{nullptr}
{
    thread = new QThread;
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
}

Render::~Render()
{

}

void Render::start()
{
    thread->start();
}

void Render::stop()
{
    requestStop = true;
    this->thread->quit();
    this->thread->wait(2000);
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
    auto f = static_cast<AVPixelFormat>(format);
    for (int i = 0; qpixfmt_map[i].fmt != AV_PIX_FMT_NONE; ++i) {
        if (qpixfmt_map[i].fmt == f)
            return qpixfmt_map[i].qfmt;
    }
    return QImage::Format_Invalid;
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
    if (renderer == nullptr) {
        return;
    }
    auto frame = f->frame;
    *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
                frame->width, frame->height, AVPixelFormat(frame->format),
                frame->width, frame->height, AV_PIX_FMT_YUV420P, sws_flags,
                nullptr, nullptr, nullptr);

    frame->color_range = AVCOL_RANGE_JPEG;
    sws_scale(*img_convert_ctx, frame->data, frame->linesize, 0,
                          is->viddec.avctx->height, frame->data, frame->linesize);
    renderer->initShader(frame->width, frame->height);
    renderer->updateData(frame->data);
}

void Render::process()
{
    forever {
        if (requestStop) {
            break;
        }
        if (is->pictq.queueNbRemain() == 0) {
            thread->msleep(50);
            continue;
        }
        if (remaining_time > 0.0) {
            qDebug() << "Remain time " << remaining_time << " render need sleep";
            thread->usleep(static_cast<unsigned long>(remaining_time * 1000000.0));
        }
        if (is->show_mode != ShowMode::SHOW_MODE_NONE && (!is->paused || is->force_refresh)){
            videoRefresh();
        }
    }
    qDebug() << "!!!Render Thread exit";
}

void Render::videoRefresh()
{
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
//            video_display(is);
        is->last_vis_time = time;
    }
    remaining_time = FFMIN(remaining_time, is->last_vis_time + rdftspeed - time);
    forever{
        if (is->pictq.queueNbRemain() != 0) {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;
            lastvp = is->pictq.peekLast();
            vp = is->pictq.peek();
            if (vp->serial != is->videoq.serial) {
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
            if (qIsNaN(vp->pts)) {
                updateVideoPts(vp->pts, vp->pos, vp->serial);
            }
            is->pictq.mutex.unlock();
            if (is->pictq.queueNbRemain() > 1) {
                Frame *nextvp = is->pictq.peek();
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
//    qDebug() << "videoDisplay";
//    auto avFrame = is->pictq.peekLast()->frame;
//    auto mImgConvertCtx = sws_getContext( is->viddec.avctx->width, is->viddec.avctx->height, is->viddec.avctx->pix_fmt, 300, 300, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
//    sws_scale(mImgConvertCtx, avFrame->data, avFrame->linesize, 0, is->viddec.avctx->height, avFrame->data, avFrame->linesize);
//    //setting QImage from frameRGB
//    for( int y = 0; y < 300; ++y ) {
//       memcpy( image->scanLine(y), avFrame->data[0]+y * avFrame->linesize[0], avFrame->linesize[0] );
//    }
//    auto f = std::rand();
//    image->save(QString("%1.png").arg(f));
    Frame *vp = is->pictq.peekLast();
    if (!vp->uploaded) {
        uploadTexture(vp, &is->img_convert_ctx);
        vp->uploaded = 1;
        vp->flip_v = vp->frame->linesize[0] < 0;
    }

}

void Render::setRenderer(IRenderer *value)
{
    renderer = value;
//    render->moveToThread(thread);
}
