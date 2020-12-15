#include "fakestreamdecoder.h"
#include <QDebug>

extern "C" {
    #include "libavformat/avformat.h"
}

#include <QDir>
#include <QThread>
#include <QUuid>
#include <QWidget>
#include <QWindow>
#include "PlayM4.h"

namespace BugAV {

using HWND = WId;
inline void checkError(std::string title, LONG nPort) {
    auto err = PlayM4_GetLastError(nPort);
    if (err > 0) {
        qDebug() << QString::fromStdString(title) << " error " << err;
    }
}

FakeStreamDecoder::FakeStreamDecoder(VideoState *is):
    QObject(nullptr)
  ,is{is}
  ,waitForKeyFrame{true}
  ,stOutput{nullptr}
  ,ctxOutput{nullptr}
  ,g_lPort{-1}
{    

    thread = new QThread{this};
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    moveToThread(thread);
    auto uuidStr = QUuid::createUuid().toString();
    auto rootPath = QLatin1String("./abc");
    auto pathStr =  rootPath + "/" + uuidStr.midRef(1, uuidStr.length() - 2 );
    path = pathStr.toUtf8();
    QDir dir {rootPath};
//    dir.removeRecursively();
    QDir dir2;
    dir2.mkpath(pathStr);

}

FakeStreamDecoder::~FakeStreamDecoder()
{
}

void FakeStreamDecoder::start()
{
    reqStop = false;
    if (thread->isRunning()) {
        return;
    }
    waitForKeyFrame = true;
    freeGOP();
    freeOutput();
    thread->start();
}

void FakeStreamDecoder::stop()
{
    if (ctxOutput != nullptr) {
        writeSegment();
        endSegment();
        freeGOP();
    }
    reqStop = true;
    while (thread->isRunning()) {
        thread->wait(400);
    }
}

void FakeStreamDecoder::setWindowForHIKSDK(QWidget *w)
{
//    wnID = id;
    if (g_lPort == -1)
    {
        LONG x;
        auto bFlag = PlayM4_GetPort(&x);
        if (bFlag == false)
        {
            PlayM4_GetLastError(g_lPort);
            return;
        }
        g_lPort = x;
    }
    PlayM4_SetStreamOpenMode(g_lPort, STREAME_REALTIME);
    checkError("PlayM4_SetStreamOpenMode", g_lPort);

//   PlayM4_OpenStream(g_lPort, (PBYTE)x, 40, 1024 * 1024);
   PlayM4_OpenStream(g_lPort, nullptr, 0, 1024 * 1024);
//    char name[] = "/home/sondq/Documents/dev/build-BugAV-Desktop_Qt_5_12_9_GCC_64bit-Debug/abc/fba57199-d08c-4cca-a937-d5d6fade9946/0.ts";
//    char name[] = "/home/sondq/Downloads/abc.mp4";
//    PlayM4_OpenFile(g_lPort, name);
    checkError("PlayM4_OpenFile", g_lPort);

    auto hWnd = PLAYM4_HWND(w->windowHandle()->winId());
    PlayM4_Play(g_lPort, hWnd);
    checkError("PlayM4_Play", g_lPort);
}

int FakeStreamDecoder::openFileOutput()
{
    nameFileOuput = path + "/" + QString::number(indexForFile).toUtf8() + ".ts";    
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
    auto fiq = new FIQ{};
    fiq->path = nameFileOuput;
    fiq->status = 0;
    fileQueue.push(fiq);
    indexForFile++;
    return  0;

}

int FakeStreamDecoder::writeSegment()
{
    if (ctxOutput == nullptr) {
        openFileOutput();
    }
    auto fiq = fileQueue.front();
    fiq->status = 3;
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

    auto fiq = fileQueue.front();
    fiq->status = 4;
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
    if (pkt == nullptr || pkt->size == 0) {
        return;
    }
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
    // wait for first file
    while (!reqStop) {
        if (fileQueue.empty()) {
            thread->msleep(1);
            continue;
        }
        break;
    }
    if (reqStop) {
        thread->terminate();
        return;
    }

    while (!reqStop) {
        auto fiq = fileQueue.front();
        if (fiq->status == 0) {
            thread->msleep(1);
            continue;
        }
        readFile(fiq);
        fiq->status = 4;
        thread->msleep(10);
    }
    thread->terminate();
}

void FakeStreamDecoder::readFile(FakeStreamDecoder::FIQ *fiq)
{
//    return;
    qDebug() << "Start read file " << fiq->path;
    QFile file{"/home/sondq/Documents/dev/build-BugAV-Desktop_Qt_5_12_9_GCC_64bit-Debug/abc/d9f3cd20-16ab-4387-8bef-d30bf993a09f/0.ts"};
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "open file " << fiq->path << " error";
        return;
    }
    thread->msleep(1000);
    int MAX_BYTE_READ = 200;
    auto buffer = new char[MAX_BYTE_READ];
    int currentByteRead = 0;
    auto fp = fopen("/home/sondq/Documents/dev/build-BugAV-Desktop_Qt_5_12_9_GCC_64bit-Debug/abc/d9f3cd20-16ab-4387-8bef-d30bf993a09f/0.ts", ("rb"));
    while (!reqStop) {
        qDebug() << " size " << file.size() << " -- " << currentByteRead;
        if ((currentByteRead + MAX_BYTE_READ) > file.size()) {
            continue;
            thread->msleep(1);
        }
        auto byteRead = fread(buffer, MAX_BYTE_READ, 1, fp);
//        auto byteRead = file.read(buffer, MAX_BYTE_READ);
        if (byteRead <= 0 && fiq->status == 3) {
            fiq->status = 4;
            return;
        }
        if (byteRead > 0) {
            auto x = (unsigned char *)(buffer);
            PlayM4_InputData(g_lPort, x, byteRead);
            checkError("PlayM4_InputData", g_lPort);
        }
        currentByteRead += MAX_BYTE_READ;
        thread->msleep(100);
    }
}

}
