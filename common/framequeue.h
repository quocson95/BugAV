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

namespace BugAV {

class PacketQueue;

class Frame {
public:
    Frame();
    ~Frame();
public:
    AVFrame *frame;
    AVSubtitle sub;
    int serial;               /* presentation timestamp for the frame */
    double duration;      /* estimated duration of the frame */
    int64_t pos;          /* byte position of the frame in the input file */
    int width;
    int height;
    int format;
    AVRational sar;
    int uploaded;
    int flip_v;  

    double *speed;
    double getPts() const;
    void setPts(double value);

private:
    double pts;

};

class FrameQueue {
public:
    FrameQueue(Define *def);
    ~FrameQueue();

    int init(PacketQueue *pktq, int maxSize, int keepLast);
    void unrefItem(Frame *vp);
    void wakeSignal();

    Frame *peek();
    Frame *peekNext();
    Frame *peekLast();
    Frame *peekWriteable();
//    Frame *peekReadable();
    void queuePush();
    void queueNext();
    int queueNbRemain();
    int64_t queueLastPos();

    qreal bufferPercent();

    bool isWriteable();

//    void syncAllFrameToNewPts(const double& oldSpeed, const double &newSpeed);

public:
    Frame *queue;
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    QMutex mutex;
    QWaitCondition cond;
    PacketQueue *pktq;

    bool waitForRead;

    double speed;

private:
    Define *def;
//    bool
};
}
#endif // FRAMEQUEUE_H

