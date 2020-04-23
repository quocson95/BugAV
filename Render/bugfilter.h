#ifndef BUGFILTER_H
#define BUGFILTER_H

extern "C" {
#include <libavutil/frame.h>
#include "libavfilter/avfilter.h"
#include <libavcodec/avcodec.h>
}

#include <QString>
class BugFilter
{
public:
    BugFilter();
    ~BugFilter();

    int init(AVCodecContext *avctx);

    QString getFilterDesc() const;
    void setFilterDesc(const QString &value);

    int pushFrame(AVFrame *frame);
    AVFrame *pullFrame();

    int getStatusInit() const;

private:
    int initPriv(AVCodecContext *avctx);
private:
    int statusInit;
    AVFrame *avFrame;
    QString filterDesc;

    AVFilterContext *out_filter_ctx;
    AVFilterContext *in_filter_ctx;
    AVFilterGraph *filter_graph;



};

#endif // BUGFILTER_H
