#include "demuxer.h"
extern "C" {
#include "libavutil/time.h"
}
#include "define.h"
#include "QDebug"

#include "packetqueue.h"
#include "clock.h"
#include "streaminfo.h"
#include "common.h"
#include "framequeue.h"

#include <QThread>



Demuxer::Demuxer()
    :QObject(nullptr)
    ,videoq{nullptr}
    ,formatOpts{nullptr}
    ,ic{nullptr}
    ,realtime{false}
    ,infinityBuff{0}
    ,abortRequest{false}
    ,paused{false}
    ,lastPaused{false}
    ,readPausedReturn{0}
    ,seekReq{false}
    ,queueAttachmentsReq{0}
    ,step{0}
    ,read_pause_return{0}
    ,loop{0}
{
    startTime = AV_NOPTS_VALUE;
    vStreamInfo = new StreamInfo;
    pkt = &pkt1;
    thread = new QThread;
    moveToThread(thread);
    connect(thread, SIGNAL (started()), this, SLOT (process()));
    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
    fakeInit();
}

Demuxer::~Demuxer()
{
    unload();
}

void Demuxer::fakeInit()
{
    videoq = new PacketQueue;
    extclk = new Clock;
    vidclk = new Clock;
    vStreamInfo = new StreamInfo;
    vStreamInfo->decoder = new Decoder;
    vStreamInfo->fqueue = new FrameQueue;
}

void Demuxer::setPktq(PacketQueue *value)
{
    videoq = value;
}

void Demuxer::setAvformat(QVariantMap avformat)
{
    this->avformat = avformat;
}

bool Demuxer::load()
{
    unload();
    input_format = av_find_input_format(fileUrl.toUtf8().constData());
    ic = avformat_alloc_context();
    if (ic == nullptr) {
        return AVERROR(ENOMEM);
    }
    // todo: set interrupt
    // set avformat
    av_dict_set(&formatOpts, "probesize", QByteArray::number(4096000), 0);
    auto ret = avformat_open_input(&ic, fileUrl.toUtf8().constData(), input_format, &formatOpts);
    if (ret < 0) {
        qDebug() << "Open input fail";
        unload();
        return false;
    }
    ic->flags |= AVFMT_FLAG_GENPTS;
    av_format_inject_global_side_data(ic);

    //find stream info
    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        unload();
        qDebug() << "could not find codec parameters";
    }
    if (ic->pb) {
        ic->pb->eof_reached = 0;
    }
    maxFrameDuration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    /* if seeking requested, we execute it */
    if (startTime != AV_NOPTS_VALUE) {
        auto timestamp = startTime;
        if (ic->start_time != AV_NOPTS_VALUE) {
            timestamp += ic->start_time;
        }
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            qDebug() << fileUrl << " could not seek to position " << timestamp / AV_TIME_BASE;
        }
    }
    realtime = isRealtime(fileUrl);
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        auto type = ic->streams[i]->codecpar->codec_type;
        if (type != AVMEDIA_TYPE_UNKNOWN) {
            stIndex[type] = int(i);
//            break;
        }
    }
    stIndex[AVMEDIA_TYPE_VIDEO] = findBestStream(
                ic, AVMEDIA_TYPE_VIDEO, stIndex[AVMEDIA_TYPE_VIDEO]);

    // open stream
    if (streamOpenCompnent(stIndex[AVMEDIA_TYPE_VIDEO]) < 0) {
        qDebug() << "Failed to open file %s " << fileUrl << " or configure filtergraph";
        if (ic != nullptr) {
            avformat_close_input(&ic);
            ic = nullptr;
        }
        return -1;
    }
    if (infinityBuff < 0 && realtime)
        infinityBuff = 1;
    thread->start();
    return true;
}

void Demuxer::unload()
{

}

void Demuxer::readFrame()
{    
    qDebug() << "read frame";
    if (paused != lastPaused) {
        lastPaused = paused;
        if (paused) {
            readPausedReturn = av_read_pause(ic);
        } else {
            av_read_play(ic);
        }
    }
    //  todo :need wait 10 ms to avoid trying to get another packet
    if (seekReq) {
        seek();
    }
    if (queueAttachmentsReq) {
        if (doQueueAttachReq() < 0) {
            // panic because alloc mem error, show error
            return;
        }
    }
    auto enoughtPkt = streamHasEnoughPackets(
                vStreamInfo->stream, vStreamInfo->index, videoq);
    if ((infinityBuff < 1 && videoq->size > MAX_QUEUE_SIZE)
            || enoughtPkt) {
        qDebug() << "full queue";
        mutex.lock();
        cond.wait(&mutex, 10); // wait max 10ms
        mutex.unlock();
        return;
    }
    if (!paused &&
            (vStreamInfo->stream == nullptr
             || (vStreamInfo->decoder->finished == videoq->serial
                 && vStreamInfo->fqueue->queueNbRemain()))) {
        if (loop != 1 && (!loop || -- loop)) {
            streamSeek(startTime != AV_NOPTS_VALUE ? startTime : 0, 0, 0);
        } else {
            return; // todo
        }
    }
    auto ret = av_read_frame(ic, pkt);
    if (ret < 0) {
        if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !eof) {
            if (vStreamInfo->index >= 0) {
                videoq->putNullPkt(vStreamInfo->index);
            }
            eof = 1;
        }
        if (ic->pb && ic->pb->error) {
            return;
        }
    } else {
        eof = 0;
    }
    if (pkt->stream_index == vStreamInfo->index
            && !(vStreamInfo->stream->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
        videoq->put(pkt);
    } else {
        av_packet_unref(pkt);
    }
    qDebug() << "queue size " << videoq->size;
    return;
}

QString Demuxer::getFileUrl() const
{
    return fileUrl;
}

void Demuxer::setFileUrl(const QString &value)
{
    fileUrl = value;
}

bool Demuxer::isRealtime(const QString &fileUrl)
{
    return true;
}

int Demuxer::findBestStream(AVFormatContext *ic, AVMediaType type, int index)
{
    return av_find_best_stream(ic, type, index, -1, nullptr, 0);
}

int Demuxer::streamHasEnoughPackets(AVStream *st, int streamID, PacketQueue *queue)
{
    return streamID < 0 ||
               queue->abort_request ||
               (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
               queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

int Demuxer::streamOpenCompnent(int stream_index)
{
    if (stream_index < 0 || stream_index >= int(ic->nb_streams)){
        return -1;
    }
    auto avctx = avcodec_alloc_context3(nullptr);
    if (!avctx)
            return AVERROR(ENOMEM);
    auto ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) {
        avcodec_free_context(&avctx);
    }
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;
    auto codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        qDebug() << "No decoder could be found for codec";
        ret = AVERROR(EINVAL);
        avcodec_free_context(&avctx);
        return ret;
    }
    avctx->codec_id = codec->id;
    auto stream_lowres = 0;
    if (stream_lowres > codec->max_lowres) {
        qDebug() << "The maximum value for lowres supported by the decoder is " << codec->max_lowres;
        stream_lowres = codec->max_lowres;
    }
    avctx->lowres = stream_lowres;
    AVDictionary *opts;
    opts = nullptr;
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        avcodec_free_context(&avctx);
        av_dict_free(&opts);
        return ret;
    }
    av_dict_free(&opts);
    eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    // todo imple audio, sub
        case AVMEDIA_TYPE_VIDEO:{
            vStreamInfo->index = stream_index;
            vStreamInfo->stream = ic->streams[stream_index];
            vStreamInfo->avctx = avctx;
            queueAttachmentsReq = 1;
            break;
        }
    }
    return 0;
}

void Demuxer::seek()
{
    int64_t seek_target = seekPos;
    int64_t seek_min    = seekRel > 0 ? seek_target - seekRel + 2: INT64_MIN;
    int64_t seek_max    = seekRel < 0 ? seek_target - seekRel - 2: INT64_MAX;
    auto ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max, seek_flags);
    if (ret < 0 ) {
        qDebug() << ic->url << " error while seeking";
    } else {
        // todo add flush for audio queue, subs queue
        videoq->flush();
        videoq->putFlushPkt();
        if (seek_flags == AVSEEK_FLAG_BYTE) {
            extclk->set(NAN, 0);
        } else {
            extclk->set(seekTarget / double(AV_TIME_BASE), 0);
        }
    }
    seekReq = 0;
    queueAttachmentsReq = 1;
    eof = 0;
    if (paused) {
        stepToNextFrame();
    }    
}

int Demuxer::doQueueAttachReq()
{
    if (vStreamInfo->stream && vStreamInfo->stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        AVPacket copy = { nullptr };
        auto ret = -1;
        if ((ret = av_packet_ref(&copy, &vStreamInfo->stream->attached_pic)) < 0) {
            if (ic) {
//              avformat_close_input(&ic);
              abortRequest = 1;
              return -1;
            }
        }
        videoq->put(&copy);
        videoq->putNullPkt(vStreamInfo->index);
    }
    queueAttachmentsReq = 0;
    return 0;
}

void Demuxer::stepToNextFrame()
{
    if (paused) {
        streamTogglePause();
    }
    step = 1;
}

void Demuxer::streamTogglePause()
{
    if (paused) {
        frame_timer += av_gettime_relative() / 1000000.0 - vidclk->last_updated;
        if (read_pause_return != AVERROR(ENOSYS)) {
            vidclk->paused = 0;
        }
        vidclk->set(vidclk->get(), vidclk->serial);
    }
    extclk->set(extclk->get(), extclk->serial);
    paused = !paused;
}

void Demuxer::streamSeek(int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (seekReq) {
        seekPos = int(pos);
        seekRel = int(rel);
        seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            seek_flags |= AVSEEK_FLAG_BYTE;
        seekReq = 1;
        continueReadThread.wakeOne();
    }
}

void Demuxer::process()
{
    qDebug() << "do process";
    forever{
        readFrame();
    }
}
