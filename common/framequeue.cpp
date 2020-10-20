#include "framequeue.h"
#include "packetqueue.h"
#include "QDebug"

namespace BugAV {
FrameQueue::FrameQueue(qint64 frameQueueSize)
{
//    this->def = frameQueueSize;
    this->frameQueueSize = frameQueueSize;
    waitForRead = false;
    rindex = 0;
    windex = 0;
    size = 0;
    max_size = 0;
    keep_last = 0;
    rindex_shown = 0;
    pktq = nullptr;
    waitForRead = false;
    speed = 1.0;
    queue = new Frame[this->frameQueueSize];
    for (auto i = 0; i < this->frameQueueSize; i++) {
        queue[i].frame = nullptr;
        queue[i].speed = &speed;
    }
}

FrameQueue::~FrameQueue()
{
    int i;
    for (i = 0; i < max_size; i++) {
        Frame *vp = &queue[i];        
        unrefItem(vp);
        av_frame_free(&vp->frame);
        vp->frame = nullptr;
    }
    delete[] queue;
}

int FrameQueue::init(PacketQueue *pktq, int maxSize, int keepLast)
{
    this->pktq = pktq;
    for (auto i = 0; i < max_size; i++) {
        Frame *vp = &queue[i];       
        unrefItem(vp);
        if (vp->frame != nullptr) {
            av_frame_free(&vp->frame);
        }
    }
    this->max_size = FFMIN(maxSize, this->frameQueueSize);
    this->keep_last = !!keepLast;
    for (auto i = 0; i < max_size; i++) {
        if (!(queue[i].frame = av_frame_alloc())) {
            // not enough mem for alloc
            return AVERROR(ENOMEM);
        }
    }
    return 0;
}

void FrameQueue::unrefItem(Frame *vp)
{
    if (vp->frame != nullptr) {
        av_frame_unref(vp->frame);
    }
//    avsubtitle_free(&vp->sub);
}

void FrameQueue::wakeSignal()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
    waitForRead = false;
    cond.wakeOne();
}

Frame *FrameQueue::peek()
{
    auto f = &queue[(rindex + rindex_shown) % max_size];
    return f;
}

Frame *FrameQueue::peekNext()
{
    auto f = &queue[(rindex + rindex_shown + 1) % max_size];
    return f;
}

Frame *FrameQueue::peekLast()
{
    return &queue[rindex];
}

Frame *FrameQueue::peekWriteable()
{
    mutex.lock();
    if  (size >= max_size &&
           !pktq->abort_request) {
//        mutex.unlock();
        cond.wait(&mutex);
//        return nullptr;
    }
    mutex.unlock();

    if (pktq->abort_request)
        return nullptr;

    return &queue[windex];
}

Frame *FrameQueue::peekReadable()
{
    /* wait until we have a readable a new frame */
    mutex.lock();
    while (size - rindex_shown <= 0 &&
          !pktq->abort_request) {
       cond.wait(&mutex);
    }
    mutex.unlock();

    if (pktq->abort_request)
       return nullptr;

    return &queue[(rindex + rindex_shown) % max_size];
}

void FrameQueue::queuePush()
{
    if (++windex == max_size){
        windex = 0;
    }
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
    size++;
    cond.wakeOne();
    waitForRead = false;
}

void FrameQueue::queueNext()
{
//    qDebug() << "queue Next";
    if (keep_last && !rindex_shown) {
        rindex_shown = 1;
        return;
    }
    unrefItem(&queue[rindex]);
    if (++rindex == max_size)
        rindex = 0;
    QMutexLocker lock(&mutex);
    size--;
    cond.wakeOne();
}

int FrameQueue::queueNbRemain()
{
    return size - rindex_shown;
}

int64_t FrameQueue::queueLastPos()
{
    Frame *fp = &queue[rindex];
    if (rindex_shown && fp->serial == pktq->serial)
        return fp->pos;
    else
        return -1;
}

qreal FrameQueue::bufferPercent()
{
    return (queueNbRemain() /this->frameQueueSize) * 100;
}

bool FrameQueue::isWriteable()
{
    auto allowWrite = !(size >= max_size);
    return allowWrite;
}

void FrameQueue::reset()
{
//    waitForRead = false;
//    rindex = 0;
//    windex = 0;
//    size = 0;
//    keep_last = 0;
//    rindex_shown = 0;
////    pktq = nullptr;
//    waitForRead = false;
//    speed = 1.0;
    auto s = FFMAX(this->max_size, this->frameQueueSize);
    init(this->pktq, s, keep_last);
}

//void FrameQueue::syncAllFrameToNewPts(const double &oldSpeed, const double &newSpeed)
//{
//    QMutexLocker lock(&mutex);
//    for (auto i = 0; i < max_size; i++) {
//       queue[i].pts = (queue[i].pts * oldSpeed) / newSpeed;
//    }
//    return;
//}

Frame::Frame()
{
    uploaded = 0;
    serial = 0;
    pts = 0;           /* presentation timestamp for the frame */
    duration = 0;      /* estimated duration of the frame */
    pos = 0;          /* byte position of the frame in the input file */
    width = 0;
    height = 0;
    format = 0;
    uploaded = 0;
    flip_v = 0;
}

Frame::~Frame()
{

}

double Frame::getPts() const
{
    if (*speed == 1.0) {
        return pts;
    }
    auto a = pts / (*speed);
    return a;
}

void Frame::setPts(double value)
{
    pts = value;
}
}
