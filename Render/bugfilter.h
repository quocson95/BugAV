#ifndef BUGFILTER_H
#define BUGFILTER_H

extern "C" {
#include <libavutil/frame.h>
#include "libavfilter/avfilter.h"
#include <libavcodec/avcodec.h>
}
class AVStream;

#include <QString>
namespace BugAV {

class VideoState;


class BugFilter
{
public:
    BugFilter();
    ~BugFilter();

    int init(AVFrame *frame, VideoState *is);

    QString getFilterDesc() const;
    void setFilterDesc(const QString &value);

    int pushFrame(AVFrame *frame);
    AVFrame *pullFrame();

    int getStatusInit() const;
    AVRational getTimeBase();

    AVRational getFrameRate();

private:
    int initPriv(AVFrame *frame, VideoState *is);

    void freeMem();

    int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
                                     AVFilterContext *source_ctx, AVFilterContext *sink_ctx);

    int getRotation(AVStream *st);
private:
    int statusInit;
    AVFrame *avFrame;
    QString filterDesc;

    AVFilterContext *filt_out;
    AVFilterContext *filt_in;
    AVFilterGraph *graph;

    int lastW, lastH;
    AVPixelFormat lastFormat;
    int lastSerial;
    int lastVFilterIdx;
//    AVRational frameRate;



};
}
#endif // BUGFILTER_H
