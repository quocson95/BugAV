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
    av_packet_unref(&pkt);
}

void Decoder::init(PacketQueue *queue, AVCodecContext *avctx, QWaitCondition *cond)
{
    setQueue(queue);
    setAvctx(avctx);
    setEmptyQueueCond(cond);
    start_pts = AV_NOPTS_VALUE;
    pkt_serial = -1;
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
                if (queue->get(&pkt, 1, &this->pkt_serial) < 0)
                    return -1;
            }
        } while (queue->serial != this->pkt_serial);

        if (PacketQueue::compareFlushPkt(&pkt)) {
           avcodec_flush_buffers(avctx);
           finished = 0;
           next_pts = start_pts;
           next_pts_tb = start_pts_tb;
        } else {
            if (avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
//                int got_frame = 0;
//                ret = avcodec_decode_subtitle2(avctx, sub, &got_frame, &pkt);
//                if (ret < 0) {
//                    ret = AVERROR(EAGAIN);
//                } else {
//                    if (got_frame && !pkt.data) {
//                       d->packet_pending = 1;
//                       av_packet_move_ref(&d->pkt, &pkt);
//                    }
//                    ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
//                }
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
}
