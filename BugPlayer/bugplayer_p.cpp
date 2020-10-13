#include "bugplayer_p.h"
#include <QDebug>
#include "common/videostate.h"
#include "Decoder/videodecoder.h"
#include "Demuxer/demuxer.h"
#include "Render/render.h"
#include <common/define.h>

namespace BugAV {
BugPlayerPrivate::BugPlayerPrivate(BugPlayer *q, ModePlayer mode)
    : q_ptr{q}
{
//    moveToThread(QThreadPool::globalInstance());
    def = new Define;
    def->setModePlayer(mode);

    is = new VideoState{def};
    demuxer = new Demuxer{is, def};
    vDecoder = new VideoDecoder{is};
    render = new Render{is};

    demuxerRunning = false;
    vDecoderRunning = false;
    renderRunning = false;
    enableFramedrop = true;

    speed = 1.0;
    av_log_set_level(AV_LOG_QUIET);
}

BugPlayerPrivate::~BugPlayerPrivate()
{

    qDebug() << "desktroy bugplayer";
    stop();

    delete vDecoder;
    delete render;
    delete demuxer;
    delete is;
}

void BugPlayerPrivate::setFile(const QString &file)
{
    curFile = file;
}

QString BugPlayerPrivate::getFile() const
{
    return curFile;
}

// 0: current playing. do not thing
// 1: starting play, need emit loading state
int BugPlayerPrivate::play()
{
    // ignore if on playing
    auto playing = isPlaying();
    if (playing ) {
//        qDebug() << "is playing. Ignore play action";
        if (isSourceChange()) {
            qDebug() << "source change, do stop before play " << is->fileUrl << " --> " << curFile;
            stop();            
        }
        return 0;
    }
    if (isPause()) {
        togglePause();
        return 0;
    }
//    emit stateChanged(BugAV::BugPlayer::AVState::LoadingState);
    return playPriv();
}

int BugPlayerPrivate::play(const QString &file)
{
    setFile(file);
    return play();
}

// 0: current is pause. Do nothing
// 1: do pause
int BugPlayerPrivate::pause()
{
//    if (!isPlaying() || is->paused) {
//        return;
//    }
    if (!is->paused) {
        togglePause();
        return 1;
//        emit stateChanged(BugAV::BugPlayer::AVState::PausedState);
    }
    return 0;
}

void BugPlayerPrivate::togglePause()
{
    is->streamTogglePause();
    is->step = 0;
}

void BugPlayerPrivate::stop()
{
    is->abort_request = 1;    
    demuxer->stop();   
    is->videoq->abort();
    is->pictq->wakeSignal();
//    is->viddec.stop();
    vDecoder->stop();
    render->stop();
    demuxer->unload();
}

void BugPlayerPrivate::refresh()
{
    stop();
    play();
}

bool BugPlayerPrivate::isPlaying() const
{    
    return (demuxer->isRunning()
            && vDecoder->isRunning()
            && render->isRunning() && !is->paused)
            ;
}

bool BugPlayerPrivate::isPause() const
{
    return (demuxer->isRunning()
            && vDecoder->isRunning()
            && render->isRunning() && is->paused)
            ;
}

bool BugPlayerPrivate::isSourceChange() const
{
    return (curFile.compare(is->fileUrl) != 0);
}

qint64 BugPlayerPrivate::buffered() const
{
    return qint64(is->pictq->queueNbRemain());
}

qreal BugPlayerPrivate::bufferPercent() const
{
    return is->pictq->bufferPercent();
}

void BugPlayerPrivate::setRenderer(IBugAVRenderer *renderer)
{
    this->render->setRenderer(renderer);
}

IBugAVRenderer *BugPlayerPrivate::getRenderer()
{
    return this->render->getRenderer();
}

void BugPlayerPrivate::setOptionsForFormat(QVariantHash avFormat)
{
    this->demuxer->setAvformat(avFormat);
}

QString BugPlayerPrivate::statistic()
{
    auto s = QString("Render %1").arg(render->statistic());
    return s;    
}

bool BugPlayerPrivate::setSaveRawImage(bool save)
{
    this->render->setSaveRawImage(save);
    return true;
}


void BugPlayerPrivate::initPriv()
{
    is->reset();
    if (enableFramedrop) {
        is->framedrop = -1;
    } else {
        is->framedrop = 0;
    }
}

int BugPlayerPrivate::playPriv()
{
    stop();
    initPriv();
    is->abort_request = 0;
    is->eof = 0;
    render->setRequestStop(false);
//    render->stop();
//    vDecoder->stop();
    is->fileUrl = curFile;
    if (is->fileUrl.isEmpty()) {
        return 0;
    }
    demuxer->start();
    return 1;
//    render->start();
//    vDecoder->start();
    // will be emit state playing when loadDone stream
}

void BugPlayerPrivate::setEnableFramedrop(bool value)
{
    enableFramedrop = value; // set before play, if not it will effect when player refresh
}

void BugPlayerPrivate::setSpeed(const double & speed)
{
    auto oldSpeed = this->speed;
//    this->is->speed = speed;
    is->setSpeed(speed);
//    this->vDecoder->setSpeedRate(speed);
    if (isPlaying()) {
        // this->is->pictq->syncAllFrameToNewPts(oldSpeed, speed);
    }
}

void BugPlayerPrivate::setStartPosition(const qint64 &time)
{
    demuxer->setStartTime(time);
}

qint64 BugPlayerPrivate::getStartPosition() const
{
    return demuxer->getStartTime();
}

qint64 BugPlayerPrivate::getDuration() const
{
    return is->duration;
}

void BugPlayerPrivate::seek(const double &position)
{       
    demuxer->doSeek(position);
}


} // namespace
