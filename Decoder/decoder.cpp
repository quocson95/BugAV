#include "decoder.h"
#include "QDebug"
#include <QThread>
#include "common/videostate.h"
namespace BugAV {
Decoder::Decoder()
{    
}

Decoder::Decoder(PacketQueue *queue, QWaitCondition *cond)
{
    init(queue, nullptr, cond);
}

Decoder::~Decoder()
{
//    av_packet_unref(&pkt);
}

void Decoder::init(PacketQueue *queue, AVCodecContext *avctx, QWaitCondition *cond)
{
    setQueue(queue);
    setAvctx(avctx);
    setEmptyQueueCond(cond);
    start_pts = AV_NOPTS_VALUE;
    pkt_serial = -1;
    privState = PrivState::InitState;
}

void Decoder::start()
{
    queue->start();
}


void Decoder::setEmptyQueueCond(QWaitCondition *value)
{
    emptyQueueCond = value;
}

void Decoder::setQueue(PacketQueue *value)
{
    queue = value;
}

void Decoder::setAvctx(AVCodecContext *value)
{
    avctx = value;
}

int Decoder::decodeFrame(AVFrame *frame)
{    
    auto ret = AVERROR(EAGAIN);
    forever{
//        qDebug() << "decodeFrame";
        AVPacket pkt;
        if (queue->serial == this->pkt_serial) {
            do {                
                if (queue->abort_request) {
                    return -1;
                }
//                qDebug() << "avcodec_receive_frame";
                switch (avctx->codec_type) {
                    case AVMEDIA_TYPE_VIDEO:
                        ret = avcodec_receive_frame(avctx, frame);
//                        qDebug() << "after avcodec_receive_frame";
                        if (ret >= 0) {
                            if (decoder_reorder_pts == -1) {
                                frame->pts = frame->best_effort_timestamp;
                            } else if (!decoder_reorder_pts) {
                                frame->pts = frame->pkt_dts;
                            }
                        }
                        break;
                    case AVMEDIA_TYPE_AUDIO:
//                        ret = avcodec_receive_frame(d->avctx, frame);
//                        if (ret >= 0) {
//                            AVRational tb = (AVRational){1, frame->sample_rate};
//                            if (frame->pts != AV_NOPTS_VALUE)
//                                frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
//                            else if (d->next_pts != AV_NOPTS_VALUE)
//                                frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
//                            if (frame->pts != AV_NOPTS_VALUE) {
//                                d->next_pts = frame->pts + frame->nb_samples;
//                                d->next_pts_tb = tb;
//                            }
//                        }
                        break;
                }
                if (ret == AVERROR_EOF) {
                    finished = this->pkt_serial;
                    avcodec_flush_buffers(avctx);
                    return 0;
                }
                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));
        }

        do {
//            qDebug() << "get packet for decode";
            if (queue->nb_packets == 0)
                emptyQueueCond->wakeOne();
            if (packet_pending) {
                av_packet_move_ref(&pkt, &this->pkt);
                packet_pending = 0;
            } else {
//                int  i = 0;
//                do {
//                    if (queue->abort_request) {
//                        i = -1;
//                        break;
//                    }
//                    i = queue->get(&pkt, 0, &this->pkt_serial);
//                    QThread::usleep(1);

//                } while(i == 0);
//                if (i < 0)
//                    return -1;
            if (queue->get(&pkt, 1, &this->pkt_serial) < 0) {
                return -1;
            }
            }
        } while (queue->serial != this->pkt_serial);

        if (PacketQueue::compareFlushPkt(&pkt)) {
           avcodec_flush_buffers(avctx);
           finished = 0;
           next_pts = start_pts;
           next_pts_tb = start_pts_tb;
        } else {
            if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {

            } else {
//                qDebug() << "avcodec_send_packet";
                if (avcodec_send_packet(this->avctx, &pkt) == AVERROR(EAGAIN)) {
                    qDebug() << "Receive_frame and send_packet both returned EAGAIN, which is an API violation";
                    packet_pending = 1;
                    av_packet_move_ref(&this->pkt, &pkt);
                }
//                qDebug() << "after avcodec_send_packet";
            }
            av_packet_unref(&pkt);
        }
    }
}

void Decoder::abort()
{
    queue->abort();
    queue->flush();
}

void Decoder::clear()
{
    // av_packet_unref(&pkt);
}

int Decoder::decodeFrameV2(AVFrame *frame)
{
    if (queue->abort_request) {
        return -1;
    }
    switch (privState) {
    case PrivState::InitState:{
        if (checkPktSerial()) {
            privState = PrivState::ReceiveFrame;
        } else {
            privState = PrivState::CheckQueue;
        }
    break;
    }
    case PrivState::ReceiveFrame: {
        auto ret = receiveFrame(frame);
        if (ret == AVERROR(EAGAIN)) {
            privState = PrivState::CheckQueue;
        } else {
            privState = InitState;
            return ret;
        }
        break;
    }
    case PrivState::CheckQueue:{
        if (checkQueue() > 0) {
            privState = PrivState::SendFrame;
        } else {
            privState = PrivState::InitState;
        }
        break;
    }
    case PrivState::SendFrame: {
        sendFrame();
        privState = PrivState::InitState;
    }
    }
    return 0;
}

int Decoder::checkPktSerial()
{
    if (queue->serial == this->pkt_serial) {
        return 1;
    }
    return 0;
}

int Decoder::receiveFrame(AVFrame *frame)
{
    int ret = -1;
    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            ret = avcodec_receive_frame(avctx, frame);
//                        qDebug() << "after avcodec_receive_frame";
            if (ret >= 0) {
                if (decoder_reorder_pts == -1) {
                    frame->pts = frame->best_effort_timestamp;
                } else if (!decoder_reorder_pts) {
                    frame->pts = frame->pkt_dts;
                }
            }
            break;
        case AVMEDIA_TYPE_AUDIO:
            break;
    }
    if (ret == AVERROR_EOF) {
        finished = this->pkt_serial;
        avcodec_flush_buffers(avctx);
        return 0;
    }
    if (ret >= 0) {
        return 1;
    }
    return ret;
}

int Decoder::checkQueue()
{
//    if (queue->nb_packets == 0)
//        emptyQueueCond->wakeOne();
//    if (packet_pending) {
//        av_packet_move_ref(&pktTmp, &this->pkt);
//        packet_pending = 0;
//    } else {
//        if (queue->get(&pktTmp, 0, &this->pkt_serial) < 0) {
//            return -1;
//        } else {
//            return 0;
//        }
//    }
//    return (queue->serial != this->pkt_serial);
    return 0;
}

int Decoder::sendFrame()
{
//    if (PacketQueue::compareFlushPkt(&pktTmp)) {
//      avcodec_flush_buffers(avctx);
//      finished = 0;
//      next_pts = start_pts;
//      next_pts_tb = start_pts_tb;
//    } else {
//       if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {

//       } else {
//           if (avcodec_send_packet(this->avctx, &pktTmp) == AVERROR(EAGAIN)) {
//               qDebug() << "Receive_frame and send_packet both returned EAGAIN, which is an API violation";
//               packet_pending = 1;
//               av_packet_move_ref(&this->pkt, &pktTmp);
//           }
//       }
//       av_packet_unref(&pktTmp);
//    }
    return 1;
}

};
