#include "bugfilter.h"
extern "C" {
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/avstring.h>
    #include <libavutil/opt.h>
    #include <libavutil/eval.h>
    #include <libavutil/display.h>
}

#include <QDebug>

#include <common/videostate.h>

namespace BugAV {
BugFilter::BugFilter()
    :statusInit{-1}
    ,avFrame{nullptr}
    ,filterDesc{"crop=100:100:50:50"}
    ,filt_out{nullptr}
    ,filt_in{nullptr}
    ,graph{nullptr}
{
    lastW = lastH = 0;
    lastFormat = AVPixelFormat::AV_PIX_FMT_NONE;
    lastSerial = -1;
    lastVFilterIdx = 0;
    avfilter_register_all();
}
BugFilter::~BugFilter()
{
    freeMem();
}

int BugFilter:: init(AVFrame *frame, VideoState *is)
{    
    if (statusInit > 0) {
        if (lastW != frame->width
                || lastH != frame->height
                || lastFormat != frame->format
                || lastSerial != is->viddec.pkt_serial) {
            // do nothing
            qDebug("Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d",
                   lastW, lastH,
                  (const char *)av_x_if_null(av_get_pix_fmt_name(lastFormat), "none"), lastSerial,
                  frame->width, frame->height,
                  (const char *)av_x_if_null(av_get_pix_fmt_name(AVPixelFormat(frame->format)), "none"), is->viddec.pkt_serial);
        } else {
                return statusInit;
        }
    } else {
        if (frame->width == 0 || frame->height == 0) {
            return statusInit;
        }
    }
    freeMem();
    avFrame = av_frame_alloc();
    if (!avFrame) {
        av_frame_free(&avFrame);
        return -1;
    }
    graph = avfilter_graph_alloc();
    if (!graph) {
        return -1;
    }
    statusInit = initPriv(frame, is);
    if (statusInit <= 0) {
        av_frame_free(&avFrame);
        avfilter_graph_free(&graph);        
    } else {
        lastW = frame->width;
        lastH = frame->height;
        lastFormat = AVPixelFormat(frame->format);
        lastSerial = is->viddec.pkt_serial;
    }
    return statusInit;
}

void BugFilter::freeMem()
{
    if (graph != nullptr) {
        avfilter_graph_free(&graph);
    }
    if (avFrame != nullptr) {
        av_frame_free(&avFrame);
    }
}

int BugFilter::configure_filtergraph(AVFilterGraph *graph, const char *filtergraph, AVFilterContext *source_ctx, AVFilterContext *sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;

    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs  = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = nullptr;

        inputs->name        = av_strdup("out");
        inputs->filter_ctx  = sink_ctx;
        inputs->pad_idx     = 0;
        inputs->next        = nullptr;

        ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, nullptr);
        if (ret < 0)
            goto fail;
    } else {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

//    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
//    for (i = 0; i < graph->nb_filters - nb_filters; i++)
//        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}

int BugFilter::getRotation(AVStream *st)
{
    AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
      uint8_t* displaymatrix = av_stream_get_side_data(st,
                                                       AV_PKT_DATA_DISPLAYMATRIX, NULL);
      double theta = 0;

      if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
          char *tail;
          theta = av_strtod(rotate_tag->value, &tail);
          if (*tail)
              theta = 0;
      }
      if (displaymatrix && !theta)
          theta = -av_display_rotation_get((int32_t*) displaymatrix);

      theta -= 360*floor(theta/360 + 0.9/360);

      if (fabs(theta - 90*round(theta/90)) > 2)
          av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
                 "If you want to help, upload a sample "
                 "of this file to ftp://upload.ffmpeg.org/incoming/ "
                 "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
      return theta;
}

int BugFilter::initPriv(AVFrame *frame, VideoState *is)
{
//    qDebug() << avfilter_configuration();
    graph->nb_threads = 0;
    AVPixelFormat pix_fmts[6];
    char sws_flags_str[512] = "";
    char buffersrc_args[256];
    int ret;

    AVFilterContext   /* *filt_src = NULL, *filt_out = NULL, */ *last_filter = NULL;
    AVCodecParameters *codecpar = is->video_st->codecpar;
    AVRational fr = av_guess_frame_rate(is->ic, is->video_st, nullptr);
    AVDictionaryEntry *e = nullptr;
    int nb_pix_fmts = 0;
    int i, j;


//    pix_fmts[] = AV_PIX_FMT_YUV420P;
    pix_fmts[0] = AVPixelFormat(28);
    pix_fmts[1] = AVPixelFormat(26);
    pix_fmts[2] = AVPixelFormat(123);
    pix_fmts[3] = AVPixelFormat(121);
    pix_fmts[4] = AVPixelFormat(0);
    pix_fmts[5] = AV_PIX_FMT_NONE;

    av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", "bicubic");
    if (strlen(sws_flags_str))
        sws_flags_str[strlen(sws_flags_str)-1] = '\0';
    graph->scale_sws_opts = av_strdup(sws_flags_str);
    snprintf(buffersrc_args, sizeof(buffersrc_args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 frame->width, frame->height, frame->format,
                 is->video_st->time_base.num, is->video_st->time_base.den,
                 codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
//    if (fr.num && fr.den)
//            av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);
    qDebug() << buffersrc_args;

    auto buffersrc  = avfilter_get_by_name("buffer");
    auto buffersink = avfilter_get_by_name("buffersink");
    if ((ret = avfilter_graph_create_filter(&filt_in,
                                               buffersrc,
                                               "in", buffersrc_args, NULL,
                                               graph)) < 0)
    if (ret < 0) {
        return ret;
    }
    AVBufferSinkParams *buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&filt_out,
                                           buffersink,
                                           "out", NULL, buffersink_params,  graph);
    av_free(buffersink_params);

    if (ret < 0) {
        return ret;
    }


    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts,  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0) {
        return ret;
    }
    last_filter= filt_out;

//#define INSERT_FILT(name, arg) do {                                          \
//AVFilterContext *filt_ctx;                                               \
//                                                                         \
//ret = avfilter_graph_create_filter(&filt_ctx,                            \
//                                   avfilter_get_by_name(name),           \
//                                   "ffplay_" name, arg, NULL, graph);    \
//if (ret < 0)                                                             \
//    return ret;                                                           \
//                                                                         \
//ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
//if (ret < 0)                                                             \
//    return ret;                                                           \
//                                                                         \
//last_filter = filt_ctx;                                                  \
//} while (0)

//    double theta  = getRotation(is->video_st);
//    if (fabs(theta - 90) < 1.0) {
//        INSERT_FILT("transpose", "clock");
//    } else if (fabs(theta - 180) < 1.0) {
//        INSERT_FILT("hflip", NULL);
//        INSERT_FILT("vflip", NULL);
//    } else if (fabs(theta - 270) < 1.0) {
//        INSERT_FILT("transpose", "cclock");
//    } else if (fabs(theta) > 1.0) {
//        char rotate_buf[64];
//        snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
//        INSERT_FILT("rotate", rotate_buf);
//    }

    ret = configure_filtergraph(graph, "crop=150:150:0:0", filt_in, last_filter);
    if (ret < 0) {
        return ret;
    }
    return 1;

}

int BugFilter::initPriv2(AVFrame *frame, VideoState *is)
{
    int ret;
    auto *buffersrc  = avfilter_get_by_name("buffer");
    auto *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

//    snprintf(args, sizeof(args),
//                 "buffer=video_size=%dx%d:pix_fmt=%d:time_base=1/1:pixel_aspect=0/1[in];"
//                 "[in]crop=100:100:100:100[out];"
//                 "[out]buffersink",
//                 frame->width, frame->height, frame->format                 );
    QByteArray arguments("buffer=");
    arguments += "video_size=" + QByteArray::number(frame->width) + "x" + QByteArray::number(frame->height) + ":";
    arguments += "pix_fmt=" + QByteArray::number(frame->format) + ":";
    arguments += "time_base=1/1:pixel_aspect=0/1[in];";
    arguments += "[in]split [main][tmp]; [tmp] crop=100:100:10:10, vflip [flip]; [main][flip] overlay=0:H/2[out];";
    arguments += "[out]buffersink";



     ret = avfilter_graph_parse2(graph, arguments, &inputs, &outputs);

     if (ret < 0) return NULL;
         assert(inputs == NULL && outputs == NULL);
     ret = avfilter_graph_config(graph, NULL);
     if (ret < 0) return NULL;

     AVFilterContext *buffersink_ctx;
     AVFilterContext *buffersrc_ctx;
     buffersrc_ctx = avfilter_graph_get_filter(graph, "Parsed_buffer_0");
     buffersink_ctx = avfilter_graph_get_filter(graph, "Parsed_buffersink_2");
     assert(buffersrc != NULL);
     assert(buffersink != NULL);

//    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_NONE };
//    AVBufferSinkParams *buffersink_params;
//    auto filter_graph = avfilter_graph_alloc();
//    /* buffer video source: the decoded frames from the decoder will be inserted here. */
//    snprintf(args, sizeof(args),
//            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
//            frame->width, frame->height, frame->format,
//            is->viddec.avctx->time_base.num,  is->viddec.avctx->time_base.den,
//            frame->sample_aspect_ratio.num, frame->sample_aspect_ratio.den);


//    ret = avfilter_graph_create_filter(&filt_in, buffersrc, "in",
//                                       args, NULL, filter_graph);
//    if (ret < 0) {
//        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
//        return ret;
//    }
//    /* buffer video sink: to terminate the filter chain. */
//    buffersink_params = av_buffersink_params_alloc();
//    buffersink_params->pixel_fmts = pix_fmts;
//    ret = avfilter_graph_create_filter(&filt_out, buffersink, "out",
//                                       NULL, buffersink_params, filter_graph);
//    av_free(buffersink_params);
//    if (ret < 0) {
//        av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
//        return ret;
//    }
//    /* Endpoints for the filter graph. */
//    outputs->name       = av_strdup("in");
//    outputs->filter_ctx = filt_in;
//    outputs->pad_idx    = 0;
//    outputs->next       = NULL;

//    inputs->name       = av_strdup("out");
//    inputs->filter_ctx = filt_out;
//    inputs->pad_idx    = 0;
//    inputs->next       = NULL;

//    auto filter_descr = "crop=iw:ih/2:0:ih/2";

//    ret = avfilter_graph_parse_ptr(filter_graph, av_strdup(filter_descr),
//                                    &inputs, &outputs, NULL);
//   if (ret < 0)
//        return ret;
//    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
//        return ret;
//    return 0;
}

int BugFilter::getStatusInit() const
{
    return statusInit;
}

AVRational BugFilter::getTimeBase()
{
    return av_buffersink_get_time_base(filt_out);
}

AVRational BugFilter::getFrameRate()
{

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
    auto ret = av_buffersrc_add_frame(filt_in, avFrame);    
    return ret;
}

AVFrame *BugFilter::pullFrame()
{
    if (statusInit < 0) {
        return nullptr;
    }
    auto ret = av_buffersink_get_frame(filt_out, avFrame);
    if (ret >= 0 ) {
         return avFrame;
    }
    return nullptr;
}

int BugFilter::pullFrame(AVFrame *frame)
{
    auto ret = av_buffersink_get_frame(filt_out, frame);
    return ret;
}
}
