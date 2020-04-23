#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H
extern "C" {
#include <libavcodec/avcodec.h>
}

#include <QMutex>
#include <QWaitCondition>


class MyAVPacketList {
public:
    MyAVPacketList();
    ~MyAVPacketList();
public:
    AVPacket pkt;
    MyAVPacketList *next;
    int serial;
};

class PacketQueue {

public:
    static AVPacket flushPkt;
    PacketQueue();
    ~PacketQueue();

    static void mustInitOnce();
    static bool compareFlushPkt(AVPacket *pkt);
    void flush();
    void abort();
    void start();

    int put(AVPacket *pkt);
    int putNullPkt(int streamIndex);
    int putFlushPkt();
    int get(AVPacket *pkt, int block, int *serial);

private:
    int putPrivate(AVPacket *pkt);
public:
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    QMutex mutex;
    QWaitCondition cond;

};


#endif // PACKETQUEUE_H
