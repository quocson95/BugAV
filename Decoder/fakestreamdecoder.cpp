#include "fakestreamdecoder.h"
#include <QDebug>

extern "C" {
    #include "libavformat/avformat.h"
}

#include <QThread>
#include <QUuid>

namespace BugAV {
FakeStreamDecoder::FakeStreamDecoder(VideoState *is):
    QObject(nullptr)
  ,is{is}
  ,waitForKeyFrame{true}
  ,stOutput{nullptr}
  ,ctxOutput{nullptr}
{    
    thread = new QThread{this};
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    moveToThread(thread);
    path = "./" + QUuid::createUuid().toByteArray();
}

FakeStreamDecoder::~FakeStreamDecoder()
{
}

void FakeStreamDecoder::start()
{
    waitForKeyFrame = true;
    freeGOP();
    freeOutput();
}

void FakeStreamDecoder::stop()
{
    if (ctxOutput != nullptr) {
        writeSegment();
        endSegment();
        freeGOP();
    }
}

int FakeStreamDecoder::openFileOutput()
{
    nameFileOuput = path + "/" + QString::number(indexForFile).toUtf8();
    lastMuxDTSRecord = 0;
    if (is->video_st == nullptr) {
        qDebug() << "video stream is null, no stream available for output.";
        return -10000;
    }
    if (ctxOutput != nullptr) {
        qDebug() << "ctxOutput not null, auto free it before open output, check it";
        freeOutput();
    }
    auto err = avformat_alloc_output_context2(&ctxOutput, nullptr, nullptr, nameFileOuput);
    if (ctxOutput == nullptr) {
        return err;
    }
    stOutput = avformat_new_stream(ctxOutput, nullptr);
    if (stOutput == nullptr) {
        qDebug() << "avformat_new_stream output error";
        freeOutput();
        return -10001;
    }
    err = avcodec_parameters_copy(stOutput->codecpar, is->video_st->codecpar);
    if (err < 0) {
        qDebug() << "avcodec_parameters_copy output error: " << err;
        freeOutput();
    }

    av_dump_format(ctxOutput, 0, nameFileOuput, 1);

    err = avio_open(&ctxOutput->pb, nameFileOuput, AVIO_FLAG_WRITE);
    if (err < 0) {
        qDebug() << "Could not open file " << nameFileOuput << " got error: " << err;
        freeOutput();
        return err;
    }
    err = avformat_write_header(ctxOutput, nullptr);
    if (err < 0) {
        qDebug() << (stderr, "Error occurred when opening output file\n");
        return err;
    }
    indexForFile++;
    return  0;

}

int FakeStreamDecoder::writeSegment()
{
    if (ctxOutput == nullptr) {
        openFileOutput();
    }
    int writeErrorCode = 0;
    foreach(auto pkt, gop) {
        av_packet_rescale_ts(pkt, is->video_st->time_base, stOutput->time_base);
        auto max = lastMuxDTSRecord+1;
        if (pkt->dts < max ) {
            if (pkt->pts >= pkt->dts) {
                pkt->pts = FFMAX(pkt->pts, max);
            }
            pkt->dts = max;
        }
        lastMuxDTSRecord = pkt->dts;
        auto err = av_interleaved_write_frame(ctxOutput, pkt);
        if (err < 0) {
            qDebug() << "error on write packet, got " << err;
            writeErrorCode = err;
            break;
        }
    }
    if (writeErrorCode != 0) {
        endSegment();
    }
    return writeErrorCode;

}

void FakeStreamDecoder::endSegment()
{
    if (ctxOutput == nullptr) {
        return;
    }
    qDebug() << "endSegment " << nameFileOuput;
    fileQueue.push(nameFileOuput);
    auto err = av_interleaved_write_frame(ctxOutput, nullptr);
    if (err != 0) {
        qDebug() << "Write endsegment " << nameFileOuput << " error " << err;
        freeOutput();
        return;
    }
    err = av_write_trailer(ctxOutput);
    if (err != 0)  {
        qDebug() << "av_write_trailer " << nameFileOuput << " error " << err;
        freeOutput();
        return;
    }
    freeOutput();
}

void FakeStreamDecoder::freeOutput()
{
    if (ctxOutput != nullptr) {
        if (!(ctxOutput->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&ctxOutput->pb);
        }
        avformat_free_context(ctxOutput);
        stOutput = nullptr;
        ctxOutput = nullptr;
    }
}

void FakeStreamDecoder::freeGOP()
{
    if (gop.empty() == 0) {
        return;
    }
    foreach(auto pkt, gop) {
        av_packet_free(&pkt);
    }
    gop.clear();
}

void FakeStreamDecoder::processPacket(AVPacket *pkt)
{
    if (waitForKeyFrame) {
        if (!IsKeyFrame(pkt)) {
            av_packet_free(&pkt);
            return;
        }
        waitForKeyFrame = false;
    }
    if (IsKeyFrame(pkt)) {
        if (!gop.empty()) {
            writeSegment();
            freeGOP();
        }
        gop.emplace_back(pkt);
        return;
    }
    gop.emplace_back(pkt);
}

void BugAV::FakeStreamDecoder::process()
{

}

}
