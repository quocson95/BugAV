#include "videodecoder.h"
#include "QDebug"

extern "C" {
#include <libavutil/time.h>
}
#include <QThread>
#include <QThreadPool>
#include "common/videostate.h"
#include "../Render/bugfilter.h"

namespace BugAV {
VideoDecoder::VideoDecoder()
    :VideoDecoder{nullptr}
{    
}

VideoDecoder::VideoDecoder(VideoState *is)
    :QObject(nullptr)
//    ,QRunnable()
    ,is{is}
    ,frame{nullptr}
    ,filter{nullptr}
{
    isRun = false;
    privState = PrivateState::StopState;
    requetsStop = false;
//    speedRate = 1.0;
    frame = nullptr;
    thread = new QThread{this};
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
//    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
//    connect(thread, &QThread::finished, this, &VideoDecoder::threadFinised);
}

VideoDecoder::~VideoDecoder()
{
    qDebug() << "Destroy VideoDecoder";
    if (frame != nullptr) {
        av_frame_free(&frame);
    }    
    if (filter != nullptr) {
        delete filter;
    }
    thread->deleteLater();
}

void VideoDecoder::start()
{
    if (thread->isRunning()) {
        return;
    }
//    if (isRun) {
//        return;
//    }
//    isRun = true;
//    qDebug() << "!!!VideoDecoder Thread start";
    requetsStop = false;
    privState = PrivateState::InitState;
    thread->start();
//    is->viddec.start();
//    class VideoDecoderRun: public QRunnable{
//    public:
//        VideoDecoderRun(VideoDecoder *seft) {
//            vDecoder = seft;
//        }
//        void run() {
//            vDecoder->process( );
//        }
//    private:
//        VideoDecoder *vDecoder;
//    };
//    QThreadPool::globalInstance()->start(new VideoDecoderRun(this));
}

void VideoDecoder::stop()
{
//    qDebug() << "!!!vDecoder Thread stop";
    requetsStop = true;
//    is->flush();
    isRun = false;
//    is->videoq->flush();
    thread->quit();
    do {
        thread->wait(500);
    } while(thread->isRunning());

    privState = PrivateState::StopState;

}

void VideoDecoder::setIs(VideoState *value)
{
    is = value;
}

bool VideoDecoder::isRunning() const
{
//    return true;
//    return isRun;
    return thread->isRunning();
}

//void VideoDecoder::setSpeedRate(const double & speedRate)
//{
//    this->speedRate = speedRate;
//}

int VideoDecoder::initPriv()
{
    if (frame == nullptr) {
        frame = av_frame_alloc();
    }
    if (!frame) {
        // error when alloc mem,
        // todo fix here
        emit stopped();
        isRun = false;
        qDebug() << "Can't alloc frame";
        return -1;
    }
    return 1;
}

int VideoDecoder::isStreamInputAvail()
{
    auto avail = (is->videoq->size > 0 && is->ic != nullptr && is->video_st != nullptr);
    return avail;
}

int VideoDecoder::calcInfo()
{
    tb = is->video_st->time_base;
    frame_rate = av_guess_frame_rate(is->ic, is->video_st, nullptr);
    return 1;
}

int VideoDecoder::getFrame()
{  
    auto ret = getVideoFrame(frame);
    if (ret < 0) {
        return ret;
    }
    if (ret == 0) {
        return 0;
    }
    duration = 0;
    if (frame_rate.num && frame_rate.den) {
        auto av = AVRational{frame_rate.den, frame_rate.num};
        duration = av_q2d(av);
    }
//    duration = ( ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
    pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : (frame->pts * av_q2d(tb));
//    if (is->realtime && is->pictq->queueNbRemain() > 1) {
//        pts /= speed_rate;
//    }
    return 1;

}

int VideoDecoder::addQueuFrame()
{
    // check writeable
//    if (!is->pictq->isWriteable()) {
////        qDebug() << "picture queue not allow write";
//        return -1;
//    }
//    qDebug() << "queuePicture";
    ret = queuePicture(filterFrame(frame), pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
    //            qDebug() << "after queuePicture";
    if (ret > 0) {
        av_frame_unref(frame);
    }
    return ret;
}

int VideoDecoder::getVideoFrame(AVFrame *frame)
{
    if (is->abort_request) {
        return  -1;
    }
    auto gotPicture = is->viddec.decodeFrame(frame);
    if (gotPicture < 0) {
        qDebug() << "got picture error";
        return -1;
    }
//    if (!gotPicture) {
//        return 0;
//    }
    if (is->video_st == nullptr) {
        av_frame_unref(frame);
        return -1;
    }
    if (gotPicture) {
        double dpts = double(NAN);
        if (frame->pts != AV_NOPTS_VALUE) {
            dpts = av_q2d(is->video_st->time_base) * frame->pts;
        }
        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
        if (is->framedrop > 0 || (is->framedrop && !is->isVideoClock())) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double diff = dpts - is->getMasterClock();
                if (!std::isnan(diff) && std::fabs(diff) < AV_NOSYNC_THRESHOLD &&
                    diff - is->frame_last_filter_delay < 0 &&
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq->nb_packets)
                {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
                    gotPicture = 0;
                }
            }
        }
    }
    return gotPicture;
}

AVFrame *VideoDecoder::filterFrame(AVFrame *frame)
{
//    return frame;
    auto ret = 1;
    if (filter->getStatusInit() <= 0) {
        qDebug() << "init filter";
        ret = filter->init(frame, is);
    }
    if ( ret < 0) {
        return frame;
    } else {
        frame_rate = filter->getFrameRate();
    }

    ret = filter->pushFrame(frame);
    if (ret < 0) {
        is->viddec.finished = is->viddec.pkt_serial;
        return frame;
    }
    is->frame_last_returned_time = av_gettime_relative() / 1000000.0;
    auto f = filter->pullFrame();
    if (f != nullptr) {
        is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
        if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
            is->frame_last_filter_delay = 0;
        tb = filter->getTimeBase();
       return f;
    } else {
        return frame;
    }
}

int VideoDecoder::queuePicture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;
    vp = is->pictq->peekWriteable();
    if (vp == nullptr) {
        return 0;
    }

    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->setPts(pts);
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;
    av_frame_move_ref(vp->frame, src_frame);
    is->pictq->queuePush();
    return 1;
}

void VideoDecoder::process()
{
    emit started();
//    speed_rate = 64.0;
    qDebug() << "!!!Video decoder Thread start";
    isRun = true;
    if (frame == nullptr) {
        frame = av_frame_alloc();
    }
    if (!frame) {
        // error when alloc mem,
        // todo fix here
        emit stopped();
        isRun = false;
        thread->quit();
        qDebug() << "Can't alloc frame";
        return;
    }    
//    if (filter == nullptr) {
//        filter = new BugFilter;
//    }

    while (is->videoq->size == 0 || is->video_st == nullptr) {
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
            if (is->abort_request) {
                break;
            }
            auto ret = getVideoFrame(frame);
            if (ret < 0) {
                qDebug() << "get video frame error";
                break;
                // break; // todo handle this, continue or exit?
            }
            if (!ret) {
                continue;
            }

            if (frame_rate.num && frame_rate.den) {
                auto av = AVRational{frame_rate.den, frame_rate.num};
                duration = av_q2d(av);
            } else {
                duration = 0;
            }

            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : (frame->pts * av_q2d(tb));
//            qDebug() <<  "duration " << duration << " pts " << pts;
//            if (is->speed != 1.0) {
//                pts /= is->speed;
//            }

//            qDebug() << "queuePicture";
            ret = queuePicture((frame), pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
//            qDebug() << "after queuePicture";
            av_frame_unref(frame);
            if (ret < 0 ) {
                // todo ??
                break;
            }

        }
    }
//    is->flush();
    isRun = false;
    is->videoq->flush();
    if (frame != nullptr) {
        av_frame_free(&frame);
        frame = nullptr;
    }

    emit stopped();
    qDebug() << "!!!Video decoder Thread exit";    
    thread->terminate();
}

void VideoDecoder::threadFinised()
{
    is->videoq->flush();
}

void VideoDecoder::run()
{
//    qDebug() << "run video decoder";
    if (requetsStop) {
        return;
    }
    switch (privState) {
        case PrivateState::StopState: {
            privState = PrivateState::InitState;
            break;
        }
        case PrivateState::InitState: {
            if (initPriv() >= 0) {
                privState = PrivateState::CheckStreamAvailState;
            }
            break;
        }
        case PrivateState::CheckStreamAvailState: {
            if (isStreamInputAvail() > 0) {
                privState = PrivateState::CalcInfoState;
            }
            break;
        }
        case PrivateState::CalcInfoState: {
            if (calcInfo() > 0) {
                privState = PrivateState::GetFrameState;
            }
            break;
        }
        case PrivateState::GetFrameState: {
            if (getFrame() > 0) {
                privState = PrivateState::AddQueueFrameState;
            }
            break;
        }
    case PrivateState::AddQueueFrameState: {
        if (addQueuFrame() > 0) {
            privState = PrivateState::GetFrameState;
        }
        break;
    }
    }
}

}
