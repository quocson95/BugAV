#ifndef STREAMINFO_H
#define STREAMINFO_H

#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>
class Decoder;
class FrameQueue;
class StreamInfo
{
public:
    StreamInfo();
    ~StreamInfo();

    AVCodecContext *avctx;
    AVCodec *codec;
    int streamLowres;
    int index;
    AVStream *stream;

    Decoder *decoder;
    FrameQueue *fqueue;
};

#endif // STREAMINFO_H
