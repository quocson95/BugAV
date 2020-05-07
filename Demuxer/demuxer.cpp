#include "demuxer.h"
extern "C" {
#include "libavutil/time.h"
}
#include "common/define.h"
#include "QDebug"
#include <QThread>
#include <QThreadPool>

#include "common/packetqueue.h"
#include "common/clock.h"
#include "common/common.h"
#include "common/framequeue.h"
#include "common/videostate.h"
#include "Decoder/decoder.h"
#include "handlerinterupt.h"

namespace BugAV {
Demuxer::Demuxer()
    : Demuxer(nullptr)

{

}

Demuxer::Demuxer(VideoState *is)
    :QObject(nullptr)
    ,is{is}
    ,formatOpts{nullptr}
    ,handlerInterupt{nullptr}
    ,avctx{nullptr}
{
    startTime = AV_NOPTS_VALUE;
    pkt = &pkt1;
    isRun = false;

    if (handlerInterupt == nullptr) {
        handlerInterupt = new HandlerInterupt(this);
    }
    curThread = new QThread(this);
    moveToThread(curThread);
    connect(curThread, SIGNAL (started()), this, SLOT (process()));
//    connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));    
}

Demuxer::~Demuxer()
{
    qDebug() << "Destroy Demuxer";
    unload();
    handlerInterupt->setReqStop(true);
    av_dict_free(&formatOpts);
    curThread->deleteLater();
    delete handlerInterupt;
}

void Demuxer::setAvformat(QVariantHash avformat)
{
    this->avformat = avformat;
//    av_dict_set(&formatOpts, "probesize", QByteArray::number(40960), 0);

}

bool Demuxer::load()
{
    qDebug() << "start load stream input";
    unload();
    is->iformat = av_find_input_format(is->fileUrl.toUtf8().constData());
    is->ic = avformat_alloc_context();
    if (is->ic == nullptr) {
        return AVERROR(ENOMEM);
    }
    // todo: set interrupt
    is->ic->interrupt_callback = *handlerInterupt;
    // set avformat    

    parseAvFormatOpts();

    handlerInterupt->begin(HandlerInterupt::Action::Open);
    auto x = is->fileUrl.toUtf8().constData();
    auto ret = avformat_open_input(&is->ic,
                                   is->fileUrl.toUtf8(),
                                   is->iformat, &formatOpts);
    handlerInterupt->end();
    if (ret < 0) {
        qDebug() << "Open input fail " << is->fileUrl;
        unload();
        return false;
    }
    AVDictionaryEntry * tag;
    while ((tag = av_dict_get(is->ic->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        qDebug("%s=%s\n", tag->key, tag->value);

    is->ic->flags |= AVFMT_FLAG_GENPTS;
    is->ic->flags |= AVFMT_FLAG_DISCARD_CORRUPT;
    is->ic->flags |= AVFMT_FLAG_NOBUFFER;
//    is->ic->flags |= AVFMT_FLAG_FLUSH_PACKETS; // flush avio context every packet (using for decrease buffer, flush old packet)

    av_format_inject_global_side_data(is->ic);
   // go to here ok

    //find stream info
    handlerInterupt->begin(HandlerInterupt::Action::FindStreamInfo);
    ret = avformat_find_stream_info(is->ic, nullptr);
    handlerInterupt->end();
    if (ret < 0) {
        unload();
        qDebug() << "could not find codec parameters";
        return  ret;
    }
    if (is->ic->pb) {
        is->ic->pb->eof_reached = 0;
    }
    if (seek_by_bytes < 0) {
        seek_by_bytes = !!(is->ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", is->ic->iformat->name);
    }
    is->max_frame_duration = (is->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    /* if seeking requested, we execute it */
    if (startTime != AV_NOPTS_VALUE) {
        auto timestamp = startTime;
        if (is->ic->start_time != AV_NOPTS_VALUE) {
            timestamp += is->ic->start_time;
        }
        ret = avformat_seek_file(is->ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            qDebug() << is->fileUrl << " could not seek to position " << timestamp / AV_TIME_BASE;
        }
    }
    is->realtime = is->isRealtime();
    for (unsigned int i = 0; i < is->ic->nb_streams; i++) {
        auto type = is->ic->streams[i]->codecpar->codec_type;
        if (type != AVMEDIA_TYPE_UNKNOWN) {
            stIndex[type] = int(i);
//            break;
        }
    }
    stIndex[AVMEDIA_TYPE_VIDEO] = findBestStream(
                is->ic, AVMEDIA_TYPE_VIDEO, stIndex[AVMEDIA_TYPE_VIDEO]);

    // go to here ok
    // open stream
    if (streamOpenCompnent(stIndex[AVMEDIA_TYPE_VIDEO]) < 0) {
        qDebug() << "Failed to open file %s " << is->fileUrl << " or configure filtergraph";
//        if (is->ic != nullptr) {
//            avformat_close_input(&is->ic);
//            is->ic = nullptr;
//        }
        unload();
        return -1;
    }
    if (infinityBuff < 0 && is->realtime)
        infinityBuff = 1;
    infinityBuff = -1;
    return true;
}

void Demuxer::unload()
{    
    if (is->ic != nullptr) {
        avformat_close_input(&is->ic);
        is->ic = nullptr;
    }
    if (avctx != nullptr) {
        avcodec_free_context(&avctx);
    }
    avctx = nullptr;
    handlerInterupt->setStatus(0);
    is->resetStream();
}

int Demuxer::readFrame()
{    
//    qDebug() << "read frame " << QThread::currentThreadId();
    if (is->paused != is->last_paused) {
        is->last_paused = is->paused;
        if (is->paused) {
            is->read_pause_return = av_read_pause(is->ic);
        } else {
            av_read_play(is->ic);
        }
    }

    if (is->paused &&
                    (!strcmp(is->ic->iformat->name, "rtsp") ||
                     (is->ic->pb && !is->fileUrl.startsWith("mmsh:")))) {
    //  need wait 10 ms to avoid trying to get another packet
        curThread->msleep(10);
        return 0;
    }
//    qDebug() << " read frame ";
    if (is->seek_req) {
        seek();
    }
    if (is->queue_attachments_req) {
        auto ret = doQueueAttachReq();
        if (ret < 0) {
            // panic because alloc mem error, show error
            return ret;
        }
    }
    auto enoughtPkt = streamHasEnoughPackets(is->video_st, is->video_stream, is->videoq);
    if ((infinityBuff < 1 && is->videoq->size > MAX_QUEUE_SIZE)
            || enoughtPkt) {
//        qDebug() << "full queue";
        mutex.lock();
        is->continue_read_thread->wait(&mutex, 1); // wait max 10ms
        mutex.unlock();
        return 0;
    }
    if (!is->paused &&
            (is->video_st != nullptr
             || (is->viddec.finished == is->videoq->serial
                 && is->pictq.queueNbRemain() == 0))) {
        if (loop != 1 && (!loop || -- loop)) {
            streamSeek(startTime != AV_NOPTS_VALUE ? startTime : 0, 0, 0);
        } else {
            // do nothing. play done file
//            return; // todo
        }
    }
    handlerInterupt->begin(HandlerInterupt::Action::Read);
    auto ret = av_read_frame(is->ic, pkt);
    handlerInterupt->end();
    if (ret < 0) {
        if ((ret == AVERROR_EOF || avio_feof(is->ic->pb)) && !is->eof) {
            if (is->video_stream >= 0) {
                is->videoq->putNullPkt(is->video_stream);
            }
            is->eof = 1;
        }
        if (is->ic->pb && is->ic->pb->error) {
            return - 1;
        }
        mutex.lock();
        is->continue_read_thread->wait(&mutex, 10); // wait max 10ms
        mutex.unlock();
        return 0;
    } else {
        is->eof = 0;
    }
    if (pkt->stream_index == is->video_stream
            && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
        is->videoq->put(pkt);
    } else {
        av_packet_unref(pkt);
    }
//    qDebug() << "queue size " << is->videoq.size;
    return 0;
}

QString Demuxer::getFileUrl() const
{
    return is->fileUrl;
}

void Demuxer::setFileUrl(const QString &value)
{
    is->fileUrl = value;
}

void Demuxer::start()
{
    if (curThread->isRunning()) {
        return;
    }
    reqStop = false;
    curThread->start();
    handlerInterupt->setReqStop(false);

//    class DemuxerRun: public QRunnable {
//    public:
//        DemuxerRun(Demuxer *demuxer) {
//            this->demuxer = demuxer;
//        }
//        void run() {
//            demuxer->process();
//        }
//    private:
//        Demuxer *demuxer;
//    };
//    QThreadPool::globalInstance()->start(new DemuxerRun{this});
}

void Demuxer::stop()
{        
    reqStop = true;
    handlerInterupt->setReqStop(true);
    cond.wakeAll();
//    thread->quit();
    while (curThread->isRunning()) {
        curThread->wait(400);
    }
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

void Demuxer::parseAvFormatOpts()
{
    if (formatOpts) {
        av_dict_free(&formatOpts);
    }
    auto keys = avformat.keys();
    foreach (auto key, keys) {
        auto val = avformat.value(key).toByteArray();
        av_dict_set(&formatOpts, key.toUtf8(), val, 0);
    }
}

int Demuxer::streamOpenCompnent(int stream_index)
{
    if (stream_index < 0 || stream_index >= int(is->ic->nb_streams)){
        return -1;
    }
    avctx = avcodec_alloc_context3(nullptr);
    if (!avctx)
        return AVERROR(ENOMEM);
    auto ret = avcodec_parameters_to_context(avctx, is->ic->streams[stream_index]->codecpar);
    if (ret < 0) {
        avcodec_free_context(&avctx);
    }
    // go to here ok
    avctx->pkt_timebase = is->ic->streams[stream_index]->time_base;
    auto codec = avcodec_find_decoder(avctx->codec_id);
    switch(avctx->codec_type){
        case AVMEDIA_TYPE_VIDEO: {
            is->last_video_stream  = stream_index;
            break;
        }
    }

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

    avctx->flags2 |= AV_CODEC_FLAG2_FAST;
    AVDictionary *opts;
    opts = nullptr;
//    av_dict_set(&opts, "threads", "auto", 0);
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
    is->eof = 0;
    is->ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    // todo imple audio, sub
        case AVMEDIA_TYPE_VIDEO:{
            is->video_stream = stream_index;
            is->video_st = is->ic->streams[stream_index];
            is->viddec.init(is->videoq, avctx, is->continue_read_thread);
            is->vidclk.init(&is->videoq->serial);
            is->queue_attachments_req = 1;
            is->viddec.start();
            is->extclk.init(&is->extclk.serial);
            break;
        }
    }
    return 0;
}

void Demuxer::seek()
{
    int64_t seek_target = is->seek_pos;
    int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
    int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
    auto ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
    if (ret < 0 ) {
        qDebug() << is->ic->url << " error while seeking";
    } else {
        // todo add flush for audio queue, subs queue
        is->videoq->flush();
        is->videoq->putFlushPkt();
        if (is->seek_flags == AVSEEK_FLAG_BYTE) {
            is->extclk.set(NAN, 0);
        } else {
            is->extclk.set(seek_target / double(AV_TIME_BASE), 0);
        }
    }
    is->seek_req = 0;
    is->queue_attachments_req = 1;
    is->eof = 0;
    if (is->paused) {
        stepToNextFrame();
    }    
}

int Demuxer::doQueueAttachReq()
{
    if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        AVPacket copy = { nullptr };
        auto ret = -1;
        if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0) {
//            if (is->ic) {
//              avformat_close_input(&ic);
//              is->abort_request = 1;
              return ret;
//            }
        }
        is->videoq->put(&copy);
        is->videoq->putNullPkt(is->video_stream);
    }
    is->queue_attachments_req = 0;
    return 0;
}

void Demuxer::stepToNextFrame()
{
    if (is->paused) {
        is->streamTogglePause();
    }
    is->step = 1;
}


void Demuxer::streamSeek(int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (is->seek_req) {
        is->seek_pos = int(pos);
        is->seek_rel = int(rel);
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        is->continue_read_thread->wakeOne();
    }
}

void Demuxer::process()
{
//    forever {
//        if (!is->abort_request) {
//            thread->msleep(1);
//        } else {
//            break;
//        }
//    }
//    return;

    isRun = true;    
    emit  started();
    qDebug() << "!!!Demuxer Thread start";
    int countSleed = -1; // count sleep 3s
    int numTry = 0;
//    forever     {
//        if (is->abort_request) {
//            break;
//        }
//        if (countSleed > 0) {
//            thread->msleep(100);
//            countSleed--;
//            continue;
//        }
//        if (load()) {
////            emit loadFailed();
////            return;
//            break;
//        } else {
//            if (numTry < 15) {
//                numTry++;
//            }
//            countSleed = 10 * numTry;
//            qDebug() << "Try open input after " << countSleed * 100 << "ms";
//            continue;
//        }
//    }
    if (!load() && !reqStop) {
        emit loadFailed();
        isRun = false;
        curThread->terminate();
        return;
    }

    if (!reqStop) {
        qDebug() << "Start read frame";
        emit loadDone();
        int ret = 0;
        forever{
            if (reqStop) {
                break;
            }
            ret = readFrame();
            if (ret < 0) {
                qDebug() << "Read frame error " << ret;
                emit readFrameError(); // read frame error
                break;
            }
        }
    }
//    if (is->ic && is->ic != nullptr) {
//        avformat_close_input(&is->ic);
//        is->ic = nullptr;
//    }
    isRun = false;
    emit stopped();
    qDebug() << "!!!Demuxer Thread exit";
    curThread->terminate();
}

void Demuxer::setIs(VideoState *value)
{
    is = value;
}

bool Demuxer::isRunning() const
{
//    return isRun;
//    return thread()->isRunning();
    return curThread->isRunning();
}
}
