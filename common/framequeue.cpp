#include "framequeue.h"
#include "packetqueue.h"

FrameQueue::FrameQueue()
{
}

FrameQueue::~FrameQueue()
{
    int i;
    for (i = 0; i < max_size; i++) {
        Frame *vp = &queue[i];
        unrefItem(vp);
        av_frame_free(&vp->frame);
    }
}

int FrameQueue::init(PacketQueue *pktq, int maxSize, int keepLast)
{
    this->pktq = pktq;
    this->max_size = FFMIN(maxSize, FRAME_QUEUE_SIZE);
    this->keep_last = !!keepLast;
    for (auto i = 0; i < max_size; i++)
    if (!(queue[i].frame = av_frame_alloc())) {
        // not enough mem for alloc
        return AVERROR(ENOMEM);
    }
    return 0;
}

void FrameQueue::unrefItem(Frame *vp)
{
    av_frame_unref(vp->frame);
    avsubtitle_free(&vp->sub);
}

void FrameQueue::wakeSignal()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
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
    while (size >= max_size &&
           !pktq->abort_request) {
        cond.wait(&mutex);
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

void *FrameQueue::queuePush()
{
    if (++windex == max_size){
        windex = 0;
    }
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
    size++;
    cond.wakeOne();
}

void FrameQueue::queueNext()
{
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

Frame::Frame()
{
    uploaded = 0;
}

Frame::~Frame()
{

}
