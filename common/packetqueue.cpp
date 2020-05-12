#include "packetqueue.h"
namespace BugAV {
AVPacket PacketQueue::flushPkt;

MyAVPacketList::MyAVPacketList()
    :next{nullptr}
    ,serial{-1}
{

}

MyAVPacketList::~MyAVPacketList()
{

}

PacketQueue::PacketQueue()
    :first_pkt{nullptr}
    ,last_pkt{nullptr}
    ,nb_packets{0}
    ,size{0}
    ,duration{0}
    ,abort_request{0}
    ,serial{-1}
{
     cond = new QWaitCondition;
     mutex = new QMutex;
}

PacketQueue::~PacketQueue()
{
    flush();
    delete cond;
    delete mutex;
}

void PacketQueue::mustInitOnce()
{
    av_init_packet(&flushPkt);
    flushPkt.data = (uint8_t *)&flushPkt;
}

bool PacketQueue::compareFlushPkt(AVPacket *pkt)
{
    auto v = (pkt->data == flushPkt.data);
    return v;
}

void PacketQueue::init()
{   
    abort_request = 1;
}

void PacketQueue::flush()
{
    MyAVPacketList *pkt, *pkt1;
    pkt = nullptr;
    pkt1 = nullptr;
    QMutexLocker lock(mutex);
    Q_UNUSED(lock)
    for (pkt = first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
//        av_freep(&pkt); // todo delete or av_freep ??
        delete pkt;
    }
    last_pkt = nullptr;
    first_pkt = nullptr;
    nb_packets = 0;
    size = 0;
    duration = 0;
}

void PacketQueue::abort()
{
    QMutexLocker lock(mutex);
    Q_UNUSED(lock)
    abort_request = 1;
    cond->wakeOne();
}

void PacketQueue::start()
{
    QMutexLocker lock(mutex);
    Q_UNUSED(lock)
    abort_request = 0;
    putPrivate(&flushPkt);
}

int PacketQueue::put(AVPacket *pkt)
{
    int ret;

    mutex->lock();
    ret = putPrivate(pkt);
    mutex->unlock();

    if (pkt != &flushPkt && ret < 0)
        av_packet_unref(pkt);
    return ret;
}

int PacketQueue::putNullPkt(int streamIndex)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->stream_index = streamIndex;
    return put(pkt);
}

int PacketQueue::putFlushPkt()
{
    return put(&flushPkt);
}

int PacketQueue::get(AVPacket *pkt, int block, int *serial)
{
    MyAVPacketList *pkt1 = nullptr;
    int ret;

    QMutexLocker lock(mutex);
    Q_UNUSED(lock)

    forever {
        if (abort_request) {
            ret = -1;
            break;
        }

        pkt1 = first_pkt;
        if (pkt1 != nullptr) {
            first_pkt = pkt1->next;
            if (first_pkt == nullptr)
                last_pkt = nullptr;
            nb_packets--;
            size -= pkt1->pkt.size + int(sizeof(*pkt1));
            duration -= pkt1->pkt.duration;
            *pkt = pkt1->pkt;
            if (serial != nullptr) {
                *serial = pkt1->serial;
            }
//          av_free(pkt1);
            delete pkt1;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            cond->wait(mutex);
        }
    }
    return ret;
}

int PacketQueue::putPrivate(AVPacket *pkt)
{
    MyAVPacketList *pkt1;

    if (abort_request)
       return -1;

    pkt1 = new MyAVPacketList;
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = nullptr;
    if (pkt == &flushPkt) {
        serial++;
    }
    pkt1->serial = serial;

    if (last_pkt == nullptr)
        first_pkt = pkt1;
    else {
        last_pkt->next = pkt1;
    }
    last_pkt = pkt1;
    nb_packets++;
    size += pkt1->pkt.size + int(sizeof(*pkt1));
    duration += pkt1->pkt.duration;
    /* XXX: should duplicate packet data in DV case */
    cond->wakeOne();
    return 0;
}
}
