#include "bugplayer.h"
#include <QDebug>
#include <QTimerEvent>
#include "common/videostate.h"
#include "Decoder/videodecoder.h"
#include "Demuxer/demuxer.h"
#include "Render/render.h"

namespace BugAV {
BugPlayer::BugPlayer(QObject *parent)
    :QObject(parent)
{
//    moveToThread(QThreadPool::globalInstance());
    is = new VideoState;
    demuxer = new Demuxer{is};
    vDecoder = new VideoDecoder{is};
    render = new Render{is};

    demuxerRunning = false;
    vDecoderRunning = false;
    renderRunning = false;

//    this->taskScheduler = taskScheduler;

    connect(demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
    connect(demuxer, &Demuxer::loadFailed, this, &BugPlayer::streamLoadedFailed);
    connect(demuxer, &Demuxer::stopped, this, &BugPlayer::demuxerStopped);
    //    connect(demuxer, &Demuxer::started, this, &BugPlayer::workerStarted);
//    connect(demuxer, &Demuxer::stopped, this, &BugPlayer::workerStopped);

//    connect(vDecoder, &VideoDecoder::started, this, &BugPlayer::workerStarted);
//    connect(vDecoder, &VideoDecoder::stopped, this, &BugPlayer::workerStopped);

//    connect(render, &Render::started, this, &BugPlayer::workerStarted);
//    connect(render, &Render::stopped, this, &BugPlayer::workerStopped);

    QVariantHash avformat;
    avformat["probesize"] = 4096000;
    avformat["analyzeduration"] = 1000000;
    demuxer->setAvformat(avformat);

    kUpdateStatistic = 0;
    //    if (kUpdateStatistic <= 0 ) {
    //        kUpdateStatistic = startTimer(2000);
    //    }
}

BugPlayer::~BugPlayer()
{    
    disconnect(demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
    disconnect(demuxer, &Demuxer::loadFailed, this, &BugPlayer::streamLoadedFailed);
    disconnect(demuxer, &Demuxer::stopped, this, &BugPlayer::demuxerStopped);

    qDebug() << "desktroy bugplayer";
    stop();

//    TaskScheduler::instance().removeTask(render);
//    TaskScheduler::instance().removeTask(vDecoder);


    delete vDecoder;
    delete render;
    delete demuxer;
    delete is;
}

void BugPlayer::setLog()
{
    av_log_set_level(AV_LOG_FATAL);
}

void BugPlayer::setFile(const QString &file)
{
    curFile = file;
}

QString BugPlayer::getFile() const
{
    return curFile;
}

void BugPlayer::play()
{
    // ignore if on playing
    if (isPlaying()) {
//        qDebug() << "is playing. Ignore play action";
        if (isSourceChange()) {
            qDebug() << "source change, do stop before play";
            stop();            
        } else {
            if (is->paused) {
                togglePause();
            }
        }
        return;
    }
    emit stateChanged(AVState::LoadingState);
    playPriv();
}

void BugPlayer::play(const QString &file)
{
    setFile(file);
    play();
}

void BugPlayer::pause()
{
//    if (!isPlaying() || is->paused) {
//        return;
//    }
    if (!is->paused) {
        togglePause();
        emit stateChanged(AVState::PausedState);
    }
}

void BugPlayer::togglePause()
{
    is->streamTogglePause();
    is->step = 0;
}

void BugPlayer::stop()
{
    is->abort_request = 1;
    demuxer->stop();
    is->videoq->abort();
    is->pictq.wakeSignal();
    vDecoder->stop();
    render->stop();   
}

void BugPlayer::refresh()
{
    stop();
    play();
}

bool BugPlayer::isPlaying() const
{    
    return (demuxer->isRunning()
            && vDecoder->isRunning()
            && render->isRunning())
            ;
}

bool BugPlayer::isSourceChange() const
{
    return (curFile.compare(is->fileUrl));
}

qint64 BugPlayer::buffered() const
{
    return qint64(is->pictq.queueNbRemain());
}

qreal BugPlayer::bufferPercent() const
{
    return is->pictq.bufferPercent();
}

void BugPlayer::setRenderer(IBugAVRenderer *renderer)
{
    this->render->setRenderer(renderer);
}

IBugAVRenderer *BugPlayer::getRenderer()
{
    return this->render->getRenderer();
}

void BugPlayer::setOptionsForFormat(QVariantHash avFormat)
{
    this->demuxer->setAvformat(avFormat);
}

QString BugPlayer::statistic()
{
    auto s = QString("Render %1").arg(render->statistic());
    return s;    
}

bool BugPlayer::setSaveRawImage(bool save)
{
    this->render->setSaveRawImage(save);
}

void BugPlayer::initPriv()
{
    is->reset();
}

void BugPlayer::playPriv()
{
    initPriv();
    is->eof = 0;
    render->setRequestStop(false);
//    render->stop();
//    vDecoder->stop();
    is->fileUrl = curFile;
    demuxer->start();

    render->start();
    vDecoder->start();
    // will be emit state playing when loadDone stream
}

void BugPlayer::workerStarted()
{
    {
        auto worker = qobject_cast<Demuxer *>(sender());
        if (worker != nullptr) {
            demuxerRunning = true;
            return;
        }

    }
    {
        auto worker = qobject_cast<VideoDecoder *>(sender());
        if (worker != nullptr) {
            vDecoderRunning = true;
            return;
        }
    }

    {
        auto worker = qobject_cast<Render *>(sender());
        if (worker != nullptr) {
            renderRunning = true;
            return;
        }
    }
}

void BugPlayer::workerStopped()
{
    {
        auto worker = qobject_cast<Demuxer *>(sender());
        if (worker != nullptr) {
            demuxerRunning = false;
            return;
        }

    }
    {
        auto worker = qobject_cast<VideoDecoder *>(sender());
        if (worker != nullptr) {
            vDecoderRunning = false;
            return;
        }
    }

    {
        auto worker = qobject_cast<Render *>(sender());
        if (worker != nullptr) {
            renderRunning = false;
            return;
        }
    }

}

void BugPlayer::streamLoaded()
{
    if (!is->abort_request) {
        render->start();
        vDecoder->start();
//        TaskScheduler::instance().addTask(render);
//        TaskScheduler::instance().addTask(vDecoder);
        emit stateChanged(AVState::PlayingState);
    }
}

void BugPlayer::streamLoadedFailed()
{
    emit stateChanged(AVState::StoppedState);
}

void BugPlayer::demuxerStopped()
{
    stop();
}

void BugPlayer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == kUpdateStatistic) {
        qDebug() << this->render->statistic() << "\r\n";
    }
}

} // namespace
