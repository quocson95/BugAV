#ifndef DEMUX_H
#define DEMUX_H

#include "common.h"

#include <libavformat/avformat.h>

#include <QMutex>
#include <QVariantHash>

class Demux
{
public:
    Demux(VideoState *is);
    ~Demux();
    bool load();
    bool unload();

    bool readFrame();
    void setFileUrl(const QString &value);

    int framQueueInit(FrameQueue *f, PacketQueue *pktq, int maxSize, int keepLast);
private:
    void streamOpenComponent(int streamIndex);
    void streamComponentClose(int streamIndex);
    void streamClose();
    void packetQueueDestroy(PacketQueue *q);
    void  packetQueueFlush(PacketQueue *q);
    int packetQueuePutNullpacket(PacketQueue *q, int streamIndex);
    int packetQueuePut(PacketQueue *q, AVPacket *pkt);
    int packetQueuePutPrivate(PacketQueue *q, AVPacket *pkt);

private:
//    VideoState *is;
    QMutex mutex;
    QString fileUrl;
    AVInputFormat *input_format;
    AVFormatContext *format_ctx;
    int videoStreamIndex;

    int max_frame_duration;

    AVDictionary *dict;
    QVariantHash options;
//    QString forced_codec_name;
    AVCodecContext *avctx;
    AVPacket pkt1, *pkt = &pkt1;
    AVPacket flush_pkt;
    PacketQueue queue;        

};

#endif // DEMUX_H
