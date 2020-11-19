#include "bugplayer.h"
#include "bugplayer_p.h"
#include <QVariant>
#include <Demuxer/demuxer.h>
#include <Render/render.h>
#include "Render/audiorender.h"
#include "common/videostate.h"
#include <Decoder/videodecoder.h>
#include "Decoder/audiodecoder.h"
#include <QDebug>


namespace BugAV {

BugPlayer::BugPlayer(QObject *parent, ModePlayer mode)
    :QObject(parent)
    ,d_ptr{new BugPlayerPrivate{this, mode}}
{
    connect(d_ptr->demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
    connect(d_ptr->demuxer, &Demuxer::loadFailed, this, &BugPlayer::streamLoadedFailed);
    connect(d_ptr->demuxer, &Demuxer::readFrameError, this, &BugPlayer::readFrameError);
    connect(d_ptr->demuxer, &Demuxer::seekFinished, this, &BugPlayer::seekFinished);

    connect(d_ptr->render, &Render::firstFrameComming, this, &BugPlayer::firstFrameComming);
    connect(d_ptr->render, &Render::noRenderNewFrameLongTime, this, &BugPlayer::noRenderNewFrameLongTime);
    connect(d_ptr->render, &Render::positionChanged, this, &BugPlayer::positionChanged);
    connect(d_ptr->render, &Render::noMoreFrame, this, &BugPlayer::noMoreFrame);
    needSeekCurrent = false;

}

BugPlayer::BugPlayer(BugPlayerPrivate &d, QObject *parent)
    : QObject(parent)
    ,d_ptr{&d}
{
    connect(d_ptr->demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
    connect(d_ptr->demuxer, &Demuxer::loadFailed, this, &BugPlayer::streamLoadedFailed);
    connect(d_ptr->demuxer, &Demuxer::readFrameError, this, &BugPlayer::readFrameError);
    connect(d_ptr->render, &Render::firstFrameComming, this, &BugPlayer::firstFrameComming);
}

void BugPlayer::streamLoaded()
{
    if (!d_ptr->is->abort_request) {
        auto denyStartAudioThread = d_ptr->is->audio_disable;
        if (!denyStartAudioThread) {
            d_ptr->aDecoder->start();
            d_ptr->audioRender->start();
        }
        d_ptr->render->start();
        d_ptr->vDecoder->start();

//        d_ptr->audioRender->audioOpen();
        emit stateChanged(BugAV::AVState::PlayingState);
        emit mediaStatusChanged(BugAV::MediaStatus::LoadedState);
        emit durationChanged(d_ptr->is->duration);
    }
}

void BugPlayer::streamLoadedFailed()
{
    emit stateChanged(BugAV::AVState::StoppedState);
    emit error(QLatin1String("load failed"));
}

void BugPlayer::readFrameError()
{    
    stop();
//    emit positionChanged(d_ptr->is->duration);
    emit stateChanged(BugAV::AVState::StoppedState);
    emit error(QLatin1String("Read frame error"));
}

void BugPlayer::firstFrameComming()
{
    emit mediaStatusChanged(BugAV::MediaStatus::FirstFrameComing);
    emit stateChanged(BugAV::AVState::PlayingState);
}

void BugPlayer::noRenderNewFrameLongTime()
{
    emit mediaStatusChanged(BugAV::MediaStatus::NoFrameRenderTooLong);
}

void BugPlayer::noMoreFrame()
{
    stop();
    emit emit stateChanged(BugAV::AVState::StoppedState);
    emit positionChanged(d_ptr->is->duration);
}

BugPlayer::~BugPlayer()
{    
}

void BugPlayer::setFile(const QString &file)
{
    d_ptr->curFile = file;
}

QString BugPlayer::getFile() const
{
    return d_ptr->curFile;
}

void BugPlayer::play()
{
    if (needSeekCurrent) {
        needSeekCurrent = false;
//        setEnableFramedrop(false);
        refreshAtCurrent();
        return;
    }
    auto status = d_ptr->play();   
    if (status > 0) {
        emit mediaStatusChanged(BugAV::MediaStatus::LoadedState);
        emit stateChanged(BugAV::AVState::PlayingState);       
    }
}

void BugPlayer::play(const QString &file)
{
    if (needSeekCurrent) {
        needSeekCurrent = false;
//        setEnableFramedrop(false);
        refreshAtCurrent();
        return;
    }
    auto status  = d_ptr->play(file);
    if (status > 0) {

        emit mediaStatusChanged(BugAV::MediaStatus::LoadedState);
        emit stateChanged(BugAV::AVState::PlayingState);               
    }

}

void BugPlayer::pause()
{
    auto status = d_ptr->pause();
    if (status > 0) {
        emit stateChanged(BugAV::AVState::PausedState);
    }
}

void BugPlayer::togglePause()
{
    d_ptr->togglePause();

}

void BugPlayer::stop()
{
    d_ptr->stop();
}

void BugPlayer::refresh()
{    
    d_ptr->demuxer->enableSkipNonKeyFrame(false);
    d_ptr->refresh();
}

void BugPlayer::refreshAtCurrent()
{
    auto startTime = d_ptr->render->getCurrentPosi();
//    stop();
//    d_ptr->demuxer->setStartTime(startTime);
//    play();
    play();
    seek(startTime);
}

bool BugPlayer::isPlaying() const
{
    return d_ptr->isPlaying();
}

bool BugPlayer::isSourceChange() const
{
    return d_ptr->isSourceChange();
}

qint64 BugPlayer::buffered() const
{
    return d_ptr->buffered();
}

qreal BugPlayer::bufferPercent() const
{
    return d_ptr->bufferPercent();
}

void BugPlayer::setRenderer(IBugAVRenderer *renderer)
{
    d_ptr->setRenderer(renderer);
}

IBugAVRenderer *BugPlayer::getRenderer()
{
    return d_ptr->getRenderer();
}

void BugPlayer::setOptionsForFormat(QVariantHash avFormat)
{
    d_ptr->setOptionsForFormat(avFormat);
}

QString BugPlayer::statistic()
{
    return d_ptr->statistic();
}

bool BugPlayer::setSaveRawImage(bool save)
{
    return d_ptr->setSaveRawImage(save);
}

void BugPlayer::setEnableFramedrop(bool value)
{
    d_ptr->setEnableFramedrop(value);
}

void BugPlayer::setPixFmtRGB32(bool value)
{
    if (value) {
        d_ptr->render->setPreferPixFmt(AVPixelFormat::AV_PIX_FMT_RGB32);
    } else {
//        if (d_ptr->render->getPreferPixFmt() == AVPixelFormat::AV_PIX_FMT_RGB32) {
//            // can not change from rgb to yuv, bug in render
//            return;
//        }
        d_ptr->render->setPreferPixFmt(AVPixelFormat::AV_PIX_FMT_NONE);
    }
}

void BugPlayer::setSpeed(const double &speed)
{
    // need reload
    auto curSpeed = d_ptr->is->getSpeed();
    if (curSpeed == speed) {
        return;
    }
    d_ptr->setSpeed(speed);    
    if (speed <= 1.0) {        
        d_ptr->demuxer->enableSkipNonKeyFrame(false);
        if (isPlaying()) {
            refreshAtCurrent();
        } else {
            needSeekCurrent = true;
        }
        return;
    }
    if (speed > 1.0 && curSpeed <= 1.0) {        
        d_ptr->demuxer->enableSkipNonKeyFrame();
        if (isPlaying()) {
            refreshAtCurrent();
        } else {
            needSeekCurrent = true;
        }
        return;
    }
}

void BugPlayer::setStartPosition(const qint64 & time)
{
    needSeekCurrent = false;
    d_ptr->setStartPosition(time);
}

qint64 BugPlayer::getStartPosition() const
{
    return d_ptr->getStartPosition();
}

qint64 BugPlayer::getDuration() const
{
    return d_ptr->getDuration();
}

void BugPlayer::seek(const double &position)
{
    d_ptr->seek(position);
}

void BugPlayer::setDisableAudio(bool value)
{
    d_ptr->setDisableAudio(value);
}

void BugPlayer::setMute(bool value)
{
    d_ptr->setMute(value);
}

bool BugPlayer::isMute() const
{
    return d_ptr->isMute();
}

QMap<QString, QString> BugPlayer::getMetadata() const
{
    return d_ptr->is->metadata;
}

void BugPlayer::positionChangedSlot(qint64 posi)
{
    // unit in micro second, emit with unit s
    auto inSec = posi / 1000000;
    qDebug() << inSec << d_ptr->is->ic->start_time;
    emit positionChanged(posi);
}

}
