#include "bugplayer_p.h"
#include <QDebug>
#include <QElapsedTimer>
#include "common/videostate.h"
#include "Decoder/videodecoder.h"
#include "Decoder/audiodecoder.h"
#include "Demuxer/demuxer.h"
#include "Render/render.h"
#include <common/define.h>
//#include <Render/audiorender.h>
#include "Render/audiooutputportaudio.h"
//#include "Render/IAudioBackend.h"

namespace BugAV {
BugPlayerPrivate::BugPlayerPrivate(BugPlayer *q, ModePlayer mode)
    : q_ptr{q}
{
//    moveToThread(QThreadPool::globalInstance());
    def = new Define;
    def->setModePlayer(mode);

    is = new VideoState{def};  
    audioRender = new AudioOutputPortAudio{is};
    demuxer = new Demuxer{is, def};
    demuxer->setAudioRender(audioRender);

    vDecoder = new VideoDecoder{is};
    aDecoder = new AudioDecoder{is};

    render = new Render{is};    

    demuxerRunning = false;
    vDecoderRunning = false;
    renderRunning = false;
    enableFramedrop = true;

//    av_log_set_level(AV_LOG_INFO);
    av_log_set_level(AV_LOG_QUIET);
}

BugPlayerPrivate::~BugPlayerPrivate()
{

    qDebug() << "desktroy bugplayer";
    stop();

    delete vDecoder;
    delete render;
    delete demuxer;
    delete audioRender;
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
    is->elLastEmptyRead->restart();
    // ignore if on playing
    auto playing = isPlaying();
    if (playing ) {
//        qDebug() << "is playing. Ignore play action";
        if (isSourceChange()) {
            qDebug() << "source change, do stop before play " << is->fileUrl << " --> " << curFile;
            stop();
            return playPriv();
        }
        return 0;
    }
    if (isPause()) {
        qDebug() << "Toggle pause ";
        togglePause();
        return 1;
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
    stopAudio();
    stopVideo();
    demuxer->unload();
    is->reset();
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
    is->paused = 0;
    render->setRequestStop(false);
//    render->stop();
//    vDecoder->stop();
    is->fileUrl = curFile;
    if (is->fileUrl.isEmpty()) {
        return 0;
    }
    demuxer->start();
    return 0;
//    render->start();
//    vDecoder->start();
    // will be emit state playing when loadDone stream
}

void BugPlayerPrivate::stopAudio()
{    
    is->audioq->abort();
    is->sampq->wakeSignal();
    aDecoder->stop();    
    audioRender->close();
    is->resetAudioStream();
}

void BugPlayerPrivate::stopVideo()
{
    is->videoq->abort();
    is->pictq->wakeSignal();
    vDecoder->stop();
    render->stop();
}

void BugPlayerPrivate::setEnableFramedrop(bool value)
{
    enableFramedrop = value; // set before play, if not it will effect when player refresh
}

void BugPlayerPrivate::setSpeed(const double & speed)
{
    auto oldSpeed = is->getSpeed();
    if (oldSpeed == speed) {
        return;
    }
    is->setSpeed(speed);         
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

void BugPlayerPrivate::setDisableAudio(bool value)
{
//    is->disableAudio = value;
//    if (is->disableAudio) {
//        stopAudio();
//    }
    is->audio_disable = value;
}

void BugPlayerPrivate::setMute(bool value)
{
    if (is->muted == int(value)) {
        return;
    }
    is->muted = value;
    if (is->muted == false) {
        if (is->audio_tgt.bytes_per_sec > 0) {
            audioRender->open(is->audio_tgt.channel_layout, is->audio_tgt.channels, is->audio_tgt.freq, &is->audio_tgt);
        }
    }
    //    if (is->muted) {
//        stopAudio();
//        is->audioq->flush();
//        return;
//    }
//    if (is->audio_disable) {
//        return;
//    }
//    if (isPlaying()) {
//        if (!aDecoder->isRunning()) {
//            aDecoder->start();
//        }
//        if (!audioRender->isRunning()) {
//            audioRender->start();
//        }
//    }
//    if (!is->realtime) {
//        demuxer->reOpenAudioSt();
//        auto x = render->getCurrentPosi();
//        demuxer->doSeek(x);
//    }
}

bool BugPlayerPrivate::isMute() const
{
    return is->muted;
}


} // namespace
