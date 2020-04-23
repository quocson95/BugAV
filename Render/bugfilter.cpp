#include "bugfilter.h"
extern "C" {
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
}

BugFilter::BugFilter()
    :statusInit{-1}
    ,avFrame{nullptr}
    ,filterDesc{"crop=100:100:50:50"}
    ,out_filter_ctx{nullptr}
    ,in_filter_ctx{nullptr}
    ,filter_graph{nullptr}
{
}
BugFilter::~BugFilter()
{
    if (filter_graph != nullptr) {
        avfilter_graph_free(&filter_graph);
    }
    if (avFrame != nullptr) {
        av_frame_free(&avFrame);
    }
}

int BugFilter::init(AVCodecContext *avctx)
{
    if (avFrame == nullptr) {
        avFrame = av_frame_alloc();
    }
    statusInit = initPriv(avctx);
    return statusInit;
}

int BugFilter::initPriv(AVCodecContext *avctx)
{
    char args[512];
    int ret;
    auto buffersrc  = avfilter_get_by_name("buffer");
    auto buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;
    filter_graph = avfilter_graph_alloc();
    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    auto a = QStringLiteral("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
                .arg(avctx->width).arg(avctx->height).arg(avctx->pix_fmt)
                .arg(1).arg(AV_TIME_BASE) //time base 1/1?
                .arg(avctx->sample_aspect_ratio.num).arg(avctx->sample_aspect_ratio.den) //sar
                ;
    ret = avfilter_graph_create_filter(&in_filter_ctx, buffersrc, "in",
                                       a.toUtf8().constData(), nullptr, filter_graph);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot create buffer source\n");
        return ret;
    }
    /* buffer video sink: to terminate the filter chain. */
//    buffersink_params = av_buffersink_params_alloc();
//    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&out_filter_ctx, buffersink, "out",
                                       nullptr, nullptr, filter_graph);
//    av_free(buffersink_params);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot create buffer sink\n");
        return ret;
    }
    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = in_filter_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = out_filter_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;
    if ((ret = avfilter_graph_parse_ptr(filter_graph, filterDesc.toUtf8().constData(),
                                    &inputs, &outputs, nullptr)) < 0)
        return ret;
    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        return ret;
    return 0;
}

int BugFilter::getStatusInit() const
{
    return statusInit;
}

QString BugFilter::getFilterDesc() const
{
    return filterDesc;
}

void BugFilter::setFilterDesc(const QString &value)
{
    filterDesc = value;
}

int BugFilter::pushFrame(AVFrame *frame)
{
    if (statusInit < 0) {
        return statusInit;
    }
    av_frame_ref(avFrame, frame);
    auto ret = av_buffersrc_write_frame(in_filter_ctx, avFrame);
    return ret;
}

AVFrame *BugFilter::pullFrame()
{
    if (statusInit < 0) {
        return nullptr;
    }
    auto ret = av_buffersink_get_frame(out_filter_ctx, avFrame);
    if (ret >= 0 ) {
         return avFrame;
    }
    return nullptr;
}
