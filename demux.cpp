#include "demux.h"
#include <QDebug>
#include <QMutexLocker>
extern "C" {
#include "libavfilter/avfilter.h"
}
Demux::Demux(VideoState *is)
    :input_format{nullptr}
    ,format_ctx{nullptr}
//    ,abortRequest{false}
{
//    this->is = is;
    dict = nullptr;
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
}

Demux::~Demux()
{
    unload();
    if (dict != nullptr) {
        av_dict_free(&dict);
    }
}

bool Demux::load()
{
    unload();
    input_format = av_find_input_format(fileUrl.toUtf8().constData());
    if (format_ctx == nullptr) {
        format_ctx = avformat_alloc_context();
    }
    // set dict
    av_dict_set(&dict, "probesize", QByteArray::number(4096000), 0);
//    av_dict_set(&dict, "probesize", QByteArray::number(4096), 0);
    auto ret = avformat_open_input(&format_ctx, fileUrl.toUtf8().constData(), input_format, &dict);
    if (ret < 0) {
        qDebug() << "Open input fail";
        unload();
        return false;
    }
    format_ctx->flags |= AVFMT_FLAG_GENPTS;
    av_format_inject_global_side_data(format_ctx);
    ret = avformat_find_stream_info(format_ctx, &dict);
    if (ret < 0) {
        qDebug() << "could not find codec parameters";
        return false;
    }
    if (format_ctx->pb != nullptr) {
        format_ctx->pb->eof_reached = 0;
    }
    max_frame_duration = (format_ctx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
//    is->realtime = true;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        auto type = format_ctx->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = int(i);
            break;
        }
    }
    qDebug() << format_ctx->streams[videoStreamIndex]->info;
    return true;
}

bool Demux::unload()
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
    if (format_ctx != nullptr) {
        qDebug("closing d->format_ctx");
        avformat_close_input(&format_ctx);
        format_ctx = nullptr;
        input_format = nullptr;
    }
    return true;

}

bool Demux::readFrame()
{
//    auto ret = av_read_frame(format_ctx, pkt);
//    if (ret < 0) {
//        if ((ret == AVERROR_EOF || avio_feof(format_ctx->pb)) && !is->eof) {
//            if (videoStreamIndex>= 0)
//                packetQueuePutNullpacket(&queue, videoStreamIndex);
////            if (is->audio_stream >= 0)
////                packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
////            if (is->subtitle_stream >= 0)
////                packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
////            is->eof = 1;
//        }
//        if (format_ctx->pb && format_ctx->pb->error)
//            return false;
////        SDL_LockMutex(wait_mutex);
////        SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
////        SDL_UnlockMutex(wait_mutex);
////        continue;
//    } else {
////        is->eof = 0;
//    }
}

void Demux::setFileUrl(const QString &value)
{
    fileUrl = value;
}

int Demux::framQueueInit(FrameQueue *f, PacketQueue *pktq, int maxSize, int keepLast)
{
    int i;
    memset(f, 0, sizeof(FrameQueue));
    f->pktq = pktq;
    f->max_size = FFMIN(maxSize, FRAME_QUEUE_SIZE);
    f->keep_last = !!keepLast;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

void Demux::streamOpenComponent(int streamIndex)
{
//    AVCodecContext *avctx;
    AVCodec *codec;
    if (streamIndex < 0 || streamIndex >= int(format_ctx->nb_streams)) {
        qDebug() << "stream index is invalid";
        return;
    }
    avctx = avcodec_alloc_context3(nullptr);
    if (avctx == nullptr) {
        qDebug() << "avcodec_alloc_context3 error";
        return;
    }
    auto ret = avcodec_parameters_to_context(avctx, format_ctx->streams[streamIndex]->codecpar);
    if (ret < 0) {
        avcodec_free_context(&avctx);
    }
    avctx->pkt_timebase = format_ctx->streams[streamIndex]->time_base;
    codec = avcodec_find_decoder(avctx->codec_id);
//    switch(avctx->codec_type){
//       case AVMEDIA_TYPE_AUDIO   : is->last_audio_stream    = streamIndex; break;
//       case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = streamIndex; break;
//       case AVMEDIA_TYPE_VIDEO   : is->last_video_stream    = streamIndex; break;
//   }
    if (!codec) {
        qDebug() << "No decoder could be found for codec " <<  avcodec_get_name(avctx->codec_id);
        avcodec_free_context(&avctx);
        return;
    }
    avctx->codec_id = codec->id;
    int stream_lowres = 0;
    if (stream_lowres > codec->max_lowres) {
        qDebug() << "The maximum value for lowres supported by the decoder is " << codec->max_lowres;
        stream_lowres = codec->max_lowres;
    }
    avctx->lowres = stream_lowres;
//    is->eof = 0;
    format_ctx->streams[streamIndex]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO: {
//        is->video_stream = streamIndex;
//        is->video_st = format_ctx->streams[streamIndex];

//        decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
//        if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
//            goto out;
//        is->queue_attachments_req = 1;
//        break;
//        av_read_pause
    }
    }

}

void Demux::streamComponentClose(int streamIndex)
{
    AVFormatContext *ic = format_ctx;
    AVCodecParameters *codecpar;
    if (streamIndex < 0 || streamIndex >= ic->nb_streams)
            return;
    codecpar = ic->streams[streamIndex]->codecpar;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO: {
//        decoder_abort(&is->viddec, &is->pictq);
//        decoder_destroy(&is->viddec);
        break;
    }
    }
}

void Demux::streamClose()
{
//    streamComponentClose(videoStreamIndex);
//    avformat_close_input(&format_ctx);
//    packetQueueDestroy(&is->videoq);
//    av_freep(is);
}

void Demux::packetQueueDestroy(PacketQueue *q)
{
    packetQueueFlush(q);
}

void Demux::packetQueueFlush(PacketQueue *q)
{
    MyAVPacketList *pkt, *pkt1;
    QMutexLocker lock(&mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
}

int Demux::packetQueuePutNullpacket(PacketQueue *q, int streamIndex)
{
    AVPacket pkt1, *pkt = &pkt1;
   av_init_packet(pkt);
   pkt->data = nullptr;
   pkt->size = 0;
   pkt->stream_index = streamIndex;
   return packetQueuePut(q, pkt);
}

int Demux::packetQueuePut(PacketQueue *q, AVPacket *pkt)
{
    int ret;

//    SDL_LockMutex(q->mutex);
    ret = packetQueuePutPrivate(q, pkt);
//    SDL_UnlockMutex(q->mutex);
    if (pkt != &flush_pkt && ret < 0)
        av_packet_unref(pkt);
    return ret;
}

int Demux::packetQueuePutPrivate(PacketQueue *q, AVPacket *pkt)
{

}
