#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include <libavcodec/avcodec.h>
}
#include "QObject"

class QWaitCondition;

//class StreamInfo;
namespace BugAV {

class VideoState;
class PacketQueue;
class Decoder {
public:
    Decoder();
    Decoder(PacketQueue *queue, QWaitCondition *cond);
    ~Decoder();

    void init(PacketQueue *queue, AVCodecContext *avctx, QWaitCondition *cond);
    void start();
    void setEmptyQueueCond(QWaitCondition *value);
    void setQueue(PacketQueue *value);
    void setAvctx(AVCodecContext *value);
    int decodeFrame(AVFrame *frame);

    void abort();

    void clear();

    int decodeFrameV2(AVFrame *frame);

public:   
    AVPacket pkt;
    PacketQueue *queue;
    AVCodecContext *avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    QWaitCondition *emptyQueueCond;
    int decoder_reorder_pts = -1; //let decoder reorder pts 0=off 1=on -1=auto

//    AVPacket pktTmp;

private:
    enum PrivState {
        InitState,
        ReceiveFrame,
        SendFrame,
        CheckQueue,
    };
    PrivState privState;

    int checkPktSerial();

    int receiveFrame(AVFrame *frame);

    int checkQueue();

    int sendFrame();
};

}


#endif // DECODER_H
