#include "bugplayer.h"
#include "bugplayer_p.h"
#include <QVariant>
#include <Demuxer/demuxer.h>
#include <Render/render.h>
#include "common/videostate.h"
#include <Decoder/videodecoder.h>


namespace BugAV {

BugPlayer::BugPlayer(QObject *parent)
    :QObject(parent)
    ,d_ptr{new BugPlayerPrivate{this}}
{
    connect(d_ptr->demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
    connect(d_ptr->demuxer, &Demuxer::loadFailed, this, &BugPlayer::streamLoadedFailed);
    connect(d_ptr->demuxer, &Demuxer::readFrameError, this, &BugPlayer::readFrameError);
    connect(d_ptr->render, &Render::firstFrameComming, this, &BugPlayer::firstFrameComming);
    connect(d_ptr->render, &Render::noRenderNewFrameLongTime, this, &BugPlayer::noRenderNewFrameLongTime);
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
        d_ptr->render->start();
        d_ptr->vDecoder->start();
        emit stateChanged(BugAV::BugPlayer::AVState::LoadingState);
    }
}

void BugPlayer::streamLoadedFailed()
{
    emit stateChanged(BugAV::BugPlayer::AVState::StoppedState);
    emit error(QLatin1String("load failed"));
}

void BugPlayer::readFrameError()
{
    stop();
    emit stateChanged(BugAV::BugPlayer::AVState::StoppedState);
    emit error(QLatin1String("Read frame error"));
}

void BugPlayer::firstFrameComming()
{
    emit mediaStatusChanged(BugAV::BugPlayer::MediaStatus::FirstFrameComing);
    emit stateChanged(BugAV::BugPlayer::AVState::PlayingState);
}

void BugPlayer::noRenderNewFrameLongTime()
{
    emit mediaStatusChanged(BugAV::BugPlayer::MediaStatus::NoFrameRenderTooLong);
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
    auto status = d_ptr->play();
    if (status > 0) {
        emit stateChanged(BugAV::BugPlayer::AVState::LoadingState);
    }

}

void BugPlayer::play(const QString &file)
{
    auto status  = d_ptr->play(file);
    if (status > 0) {
        emit stateChanged(BugAV::BugPlayer::AVState::LoadingState);
    }
}

void BugPlayer::pause()
{
    auto status = d_ptr->pause();
    if (status > 0) {
        emit stateChanged(BugAV::BugPlayer::AVState::PausedState);
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
    d_ptr->refresh();
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


}
