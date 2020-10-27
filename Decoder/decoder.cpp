#include "decoder.h"
#include "QDebug"
#include <QThread>
#include "common/videostate.h"
namespace BugAV {
Decoder::Decoder():
    avctx{nullptr}
{    
}

Decoder::Decoder(PacketQueue *queue, QWaitCondition *cond):
    avctx{nullptr}
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
                case AVMEDIA_TYPE_UNKNOWN:
                case AVMEDIA_TYPE_DATA:
                case AVMEDIA_TYPE_SUBTITLE:
                case AVMEDIA_TYPE_ATTACHMENT:
                case AVMEDIA_TYPE_NB:
                    break;
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
                        ret = avcodec_receive_frame(avctx, frame);
                        if (ret >= 0) {
                            AVRational tb = (AVRational){1, frame->sample_rate};
                            if (frame->pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(frame->pts, this->avctx->pkt_timebase, tb);
                            else if (this->next_pts != AV_NOPTS_VALUE)
                                frame->pts = av_rescale_q(this->next_pts, this->next_pts_tb, tb);
                            if (frame->pts != AV_NOPTS_VALUE) {
                                this->next_pts = frame->pts + frame->nb_samples;
                                this->next_pts_tb = tb;
                            }
                        }
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
                ret = 0;
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

void Decoder::freeAvctx()
{
    if (avctx != nullptr) {
        avcodec_close(avctx);
        avcodec_free_context(&avctx);
    }
    avctx = nullptr;
}

int Decoder::getPkt_serial() const
{
    return pkt_serial;
}

void Decoder::setPkt_serial(int value)
{
    pkt_serial = value;
}

};
