#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/rational.h>
}

#include "define.h"
#include <QMutex>
#include <QWaitCondition>

class PacketQueue;

class Frame {
public:
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;
};

class FrameQueue {
public:
    FrameQueue();
    ~FrameQueue();

    int init(PacketQueue *pktq, int maxSize, int keepLast);
    void unrefItem(Frame *vp);
    void wakeSignal();

    Frame *peek();
    Frame *peekNext();
    Frame *peekLast();
    Frame *peekWriteable();
    Frame *peekReadable();
    void *queuePush();
    void queueNext();
    int queueNbRemain();
    int64_t queueLastPos();

public:
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    QMutex mutex;
    QWaitCondition cond;
    PacketQueue *pktq;
};

#endif // FRAMEQUEUE_H
