#include "videodecoder.h"
#include "QDebug"

#include <QThread>
#include "videostate.h"

VideoDecoder::VideoDecoder()
    :VideoDecoder{nullptr}
{    
}

VideoDecoder::VideoDecoder(VideoState *is)
    :QObject(nullptr)
    ,is{is}
    ,frame{nullptr}
{
    thread = new QThread;
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    connect(thread, &QThread::finished, this, &VideoDecoder::threadFinised);
}

VideoDecoder::~VideoDecoder()
{
    if (frame != nullptr) {
        av_frame_free(&frame);
    }
}

void VideoDecoder::start()
{
    thread->start();
}

void VideoDecoder::stop()
{
    is->videoq.abort();
    is->pictq.wakeSignal();
    this->thread->quit();
    this->thread->wait(2000);
}

void VideoDecoder::setIs(VideoState *value)
{
    is = value;
}

int VideoDecoder::getVideoFrame(AVFrame *frame)
{
    auto gotPicture = is->viddec.decodeFrame(frame);
    if (gotPicture < 0) {
        qDebug() << "got picture error";
        return -1;
    }
    if (gotPicture) {
        double dpts = NAN;
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        }
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
        if (is->framedrop>0 || (is->framedrop && is->getMasterSyncType() != ShowModeClock::AV_SYNC_VIDEO_MASTER)) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - is->getMasterClock();
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff - is->frame_last_filter_delay < 0 &&
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq.nb_packets) {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    gotPicture = 0;
                }
            }
        }
    }
    return gotPicture;
}

int VideoDecoder::queuePicture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
     Frame *vp;
     if (!(vp = is->pictq.peekWriteable())) {
        return -1;
     }
     vp->sar = src_frame->sample_aspect_ratio;
     vp->uploaded = 0;

     vp->width = src_frame->width;
     vp->height = src_frame->height;
     vp->format = src_frame->format;

     vp->pts = pts;
     vp->duration = duration;
     vp->pos = pos;
     vp->serial = serial;
     av_frame_move_ref(vp->frame, src_frame);
     is->pictq.queuePush();
     return 0;
}

void VideoDecoder::process()
{
    if (frame == nullptr) {
        frame = av_frame_alloc();
    }
    if (!frame) {
        // error when alloc mem,
        // todo fix here
        qDebug() << "Can't alloc frame";
        return;
    }
    while (is->videoq.size == 0) {
//        qDebug() << "wait video stream input";
        if (is->abort_request) {
            av_frame_free(&frame);
            frame = nullptr;
            break;
        }
        thread->msleep(100);
    }
    if (!is->abort_request) {
        tb = is->video_st->time_base;
        frame_rate = av_guess_frame_rate(is->ic, is->video_st, nullptr);
        forever{
//            qDebug() << "decoder video frame";
            auto ret = getVideoFrame(frame);
            if (ret < 0) {
                break;
                // break; // todo handle this, continue or exit?
            }
            if (!ret) {
                continue;
            }
            duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : (frame->pts * av_q2d(tb));
            ret = queuePicture(frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
            av_frame_unref(frame);
            if (ret < 0 ) {
                // todo ??
                break;
            }
        }
    }
    qDebug() << "!!!Video decoder Thread exit";
    av_frame_free(&frame);
    frame = nullptr;
}

void VideoDecoder::threadFinised()
{
    is->videoq.flush();
}
