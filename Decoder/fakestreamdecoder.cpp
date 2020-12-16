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

#include <RenderOuput/IBugAVRenderer.h>
#include <RenderOuput/bugglwidget.h>
#include <RenderOuput/ibugavdefaultrenderer.h>


namespace BugAV {

static void CALLBACK PlayM4DisplayCallBackEx(DISPLAY_INFO *pstDisplayInfo) {
    auto fakeStreamDec = reinterpret_cast<FakeStreamDecoder *>(pstDisplayInfo->nUser);
    fakeStreamDec->playM4DispCB();
}

static void WinIDFECChangedCallback(unsigned int wnID, QString id, void *opaque) {
    auto fakeStreamDec = reinterpret_cast<FakeStreamDecoder *>(opaque);
    fakeStreamDec->winIDFECChanged(wnID, id);
}

constexpr int MAX_DURATION_FILE = 60*1000; // ms
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
    ,lastMuxDTSRecord{0}
    ,indexForFile{0}
    ,g_lPort{-1}
  ,mainWn{nullptr}
  ,kCheckFrameDisp{-1}
{    
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
    PlayM4_SetStreamOpenMode(g_lPort, STREAME_FILE);
    checkError("PlayM4_SetStreamOpenMode", g_lPort);
    PlayM4_OpenStream(g_lPort, nullptr, 0, SOURCE_BUF_MAX);
    checkError("PlayM4_OpenFile", g_lPort);
    PlayM4_SetDisplayCallBackEx(g_lPort, PlayM4DisplayCallBackEx, this);

    thread = new QThread{this};
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    moveToThread(thread);
    auto uuidStr = QUuid::createUuid().toString();
    auto rootPath = QLatin1String("./abc");
    auto pathStr =  rootPath + "/" + uuidStr.midRef(1, uuidStr.length() - 2 );
    path = pathStr.toUtf8();
    QDir dir {rootPath};
    QDir dir2;
    dir2.mkpath(path);
//    dir.removeRecursively();

    elSinceFirstFrame = std::make_unique<QElapsedTimer>();

}

FakeStreamDecoder::~FakeStreamDecoder()
{
    stop();
    // remove all file
    QDir dir{path};
//    dir.mkpath(path);
    dir.removeRecursively();
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
    mainWn = w;
    if (mainWn == nullptr ) {
        PlayM4_Play(g_lPort, 0);
        return;
    }
    auto hWnd = PLAYM4_HWND(mainWn->windowHandle()->winId());
    PlayM4_Play(g_lPort, hWnd);
//    checkError("PlayM4_Play", g_lPort);

//    PlayM4_FEC_Enable(g_lPort);
//    checkError("PlayM4_FEC_Enable", g_lPort);

//    PlayM4_FEC_GetPort(g_lPort, &s_lPort, FEC_PLACE_CEILING, FEC_CORRECT_PTZ);
//    checkError("PlayM4_FEC_GetPort", g_lPort);

//    PlayM4_FEC_GetParam(g_lPort, s_lPort, stFEPara1);
//    checkError("PlayM4_FEC_GetParam", g_lPort);

//    stFEPara1->nUpDateType = FEC_UPDATE_PTZPARAM | FEC_UPDATE_PTZZOOM;
//    stFEPara1->stPTZParam.fPTZPositionX = 0.3f;
//    stFEPara1->stPTZParam.fPTZPositionY = 0.3f;
//    stFEPara1->fZoom                    = 0.00001f;

//    PlayM4_FEC_SetParam(g_lPort, s_lPort, stFEPara1);
//    checkError("PlayM4_FEC_SetParam", g_lPort);

//    PlayM4_FEC_SetWnd(g_lPort, s_lPort, reinterpret_cast<void *>(hWnd));
//    checkError("PlayM4_FEC_SetWnd", g_lPort);
}

void FakeStreamDecoder::setWindowFishEyeForHIKSDK(QString id, QWidget *w)
{
    if (fishEyeView.empty()) {
        PlayM4_FEC_Enable(g_lPort);
        checkError("PlayM4_FEC_Enable", g_lPort);
    }

    if (!fishEyeView.contains(id)) {
        FishEyeSubView *fec = new FishEyeSubView;
        unsigned int port;
        PlayM4_FEC_GetPort(g_lPort, &port, FEC_PLACE_CEILING, FEC_CORRECT_PTZ);
        checkError("PlayM4_FEC_GetPort", g_lPort);
        fec->port = port;
        fec->w = w;
        PlayM4_FEC_GetParam(g_lPort, port, &fec->param);
        checkError("PlayM4_FEC_GetParam", g_lPort);
        fishEyeView.insert(id, fec);
    }
    auto fec = fishEyeView.value(id);
    PlayM4_FEC_SetParam(g_lPort, fec->port, &fec->param);
    checkError("PlayM4_FEC_SetParam", g_lPort);

    auto wnID = w->winId();
    PlayM4_FEC_SetWnd(g_lPort, fec->port, reinterpret_cast<void *>(wnID));
    checkError("PlayM4_FEC_SetWnd" , g_lPort);

//    auto x = fec->w->windowHandle();
//    PlayM4_FEC_SetWnd(g_lPort, s_lPort, reinterpret_cast<void *>(fec->w->windowHandle()->winId()));
//    checkError("PlayM4_FEC_SetWnd", g_lPort);
//    auto render = reinterpret_cast<IBugAVDefaultRenderer *>(w);
//    render->registerWinIDChangedCB(WinIDFECChangedCallback, id, this);
}

int FakeStreamDecoder::openFileOutput()
{
    nameFileOuput = path + "/" + QString::number(indexForFile).toUtf8() + ".ts";
    qDebug() << "new file ouput " << nameFileOuput;
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
//    if (indexForFile == 0) {
    AVDictionary *formatOpts = nullptr;
    av_dict_set(&formatOpts,  "movflags", "frag_keyframe+empty_moov", 0);
    err = avformat_write_header(ctxOutput, &formatOpts);
    av_dict_free(&formatOpts);
    if (err < 0) {
        qDebug() << (stderr, "Error occurred when opening output file\n");
        return err;
    }
//    }
    auto fiq = new FIQ{};
    fiq->path = nameFileOuput;
    fiq->status = FileStatus::Init;
    currentFIQWrite = fiq;
    fileQueue.push(fiq);
    indexForFile++;
    return  0;

}

int FakeStreamDecoder::writeSegment()
{
    if (ctxOutput == nullptr) {
        openFileOutput();
        elRecordDur.start();
    }
    auto fiq = currentFIQWrite;
    fiq->status = FileStatus::Writing;
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
            auto eStr = av_err2str(err);
            qDebug() << "error on write packet, got " << QString::fromUtf8(eStr);
            writeErrorCode = err;
            break;
        }
    }
//    if (writeErrorCode != 0) {
//        endSegment();
//    }
    return writeErrorCode;

}

void FakeStreamDecoder::endSegment()
{
    if (ctxOutput == nullptr) {
        return;
    }
    qDebug() << "endSegment " << nameFileOuput;

    auto fiq = currentFIQWrite;
    fiq->status = FileStatus::WriteDone;
    auto err = av_interleaved_write_frame(ctxOutput, nullptr);
    if (err != 0) {
        qDebug() << "Write endsegment " << nameFileOuput << " error " << err;
        freeOutput();
        return;
    }
//    if (indexForFile == 1) {
    err = av_write_trailer(ctxOutput);
    if (err != 0)  {
        qDebug() << "av_write_trailer " << nameFileOuput << " error " << err;
        freeOutput();
        return;
    }
//    }
    freeOutput();
}

void FakeStreamDecoder::freeOutput()
{
    if (ctxOutput != nullptr) {
        if (!(ctxOutput->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&ctxOutput->pb);
        }
        avcodec_close(stOutput->codec);
        avformat_free_context(ctxOutput);
        stOutput = nullptr;
        ctxOutput = nullptr;
    }
}

void FakeStreamDecoder::freeGOP()
{
    if (gop.empty()) {
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
//    av_packet_unref(pkt);
    if (waitForKeyFrame) {
        if (!IsKeyFrame(pkt)) {
            av_packet_free(&pkt);
            return;
        }
        waitForKeyFrame = false;
    }
    if (IsKeyFrame(pkt)) {
        if (!gop.empty()) {
            auto writeErrCode = writeSegment();
            freeGOP();
            if (writeErrCode != 0 || elRecordDur.hasExpired(MAX_DURATION_FILE)) {
                endSegment();
                elRecordDur.invalidate();
            }
        }
        gop.emplace_back(pkt);
        return;
    }
    gop.emplace_back(pkt);
}

void FakeStreamDecoder::playM4DispCB()
{
    if (!elSinceFirstFrame->isValid()) {
        emit firstFrameComming();
        elSinceFirstFrame->start();
        startTimer(&kCheckFrameDisp, 1000);
    }
}

void FakeStreamDecoder::winIDFECChanged(unsigned int wnID, QString id)
{
    auto fec = fishEyeView.value(id);
    PlayM4_FEC_SetWnd(g_lPort, fec->port, reinterpret_cast<void *>(wnID));
    checkError("PlayM4_FEC_SetWnd" , g_lPort);
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

    setWindowForHIKSDK(mainWn);

    while (!reqStop) {
        if (fileQueue.empty()) {
            thread->msleep(1);
            continue;
        }
        auto fiq = fileQueue.front();      
        if (fiq->status == FileStatus::Init) {
            thread->msleep(1);
            continue;
        }
        fileQueue.pop();
        readFile(fiq);
        removeFile(fiq);
        delete  fiq;
//        thread->msleep(10);
    }
    thread->terminate();
}

void FakeStreamDecoder::readFile(FakeStreamDecoder::FIQ *fiq)
{
    if (fiq->status == FileStatus::ReadDone) {
        qDebug() << "File " << fiq->path << " alread read done, remove it";
    }    
    {
        QElapsedTimer el;
        el.start();
        while(!reqStop) {
            thread->msleep(100);
            if (el.hasExpired(4000)) {
                break;
            }
        }
        if (reqStop) {
            return;
        }
    }
    qDebug() << "Start read file " << fiq->path;
    QFile file{fiq->path};

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "open file " << fiq->path << " error";
        return;
    }
    int MAX_BYTE_READ = 256;
    auto buffer = new char[MAX_BYTE_READ];
    int currentByteRead = 0;

    int err;
//    while (!reqStop) {
//        auto fileSize = file.size();
//        if (fileSize > 40) {
//            file.read(buffer, 40);
//            break;
//        } else {
//            thread->usleep(1);
//        }
//    }
    memset(buffer, 0, sizeof(char)*MAX_BYTE_READ);
    auto fileSize = file.size();
    while (!reqStop)
    {
        if ((currentByteRead + MAX_BYTE_READ) > fileSize) {
            if (fiq->status != FileStatus::WriteDone) {
                thread->usleep(1);
                fileSize = file.size();
                continue;
            }
        }
        auto byteRead = file.read(buffer, MAX_BYTE_READ);
        if (byteRead < MAX_BYTE_READ) {
            qDebug() << " size " << fileSize << " -- " << currentByteRead << " read " << byteRead;
        }
        if (byteRead <= 0) {
            if (fiq->status == FileStatus::WriteDone) {
                fiq->status = FileStatus::ReadDone;
                return;
            }
            thread->usleep(1);
            continue;
        }

        if (byteRead > 0) {
            do {
                err = PlayM4_InputData(g_lPort, reinterpret_cast<PBYTE>(buffer), byteRead);
//                if (err == PLAYM4_BUF_OVER) {
//                    thread->usleep(1);
//                }
            } while(!reqStop && err == PLAYM4_BUF_OVER);
            memset(buffer, 0, sizeof(char)*MAX_BYTE_READ);
            currentByteRead += byteRead;
        }
    }

    file.close();
}

void FakeStreamDecoder::removeFile(FakeStreamDecoder::FIQ *fiq)
{
    if (fiq->status != FileStatus::ReadDone) {
        return;
    }
    QFile file{fiq->path};
    file.remove();
}

void FakeStreamDecoder::killTimer(int *kId)
{
    if (kId == nullptr || *kId < 0) {
        return;
    }
    QObject::killTimer(*kId);
    *kId = -1;
    return;
}

void FakeStreamDecoder::startTimer(int *kId, int interval)
{
    if (kId == nullptr) {
        return ;
    }
    killTimer(kId);
    *kId = QObject::startTimer(interval);
    return;
}

void FakeStreamDecoder::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == kCheckFrameDisp) {
        if (elSinceFirstFrame->hasExpired(10000)) {
            elSinceFirstFrame->invalidate();
            emit noRenderNewFrameLongTime();
            killTimer(&kCheckFrameDisp);
        }
    }
}

}
