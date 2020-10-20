#include "define.h"

namespace BugAV {
Define::Define()
{
    modePlayer = ModePlayer::RealTime;
}

void Define::setModePlayer(ModePlayer mode)
{
    modePlayer = mode;
}

qint64 Define::MaxQueueSize() const
{
    if (modePlayer == ModePlayer::RealTime) {
        return qint64(MAX_QUEUE_SIZE);
    }
    return qint64(MAX_QUEUE_SIZE_VOD);
}

int Define::VideoPictureQueueSize() const
{
    if (modePlayer == ModePlayer::RealTime) {
        return int(VIDEO_PICTURE_QUEUE_SIZE);
    }
    return int(VIDEO_PICTURE_QUEUE_SIZE_VOD);
}

qint64 Define::FramePictQueueSize() const
{
    if (modePlayer == ModePlayer::RealTime) {
        return qint64(VIDEO_PICTURE_QUEUE_SIZE);
    }
    return qint64(VIDEO_PICTURE_QUEUE_SIZE_VOD);
}

qint64 Define::FrameSampQueueSize() const
{
    if (modePlayer == ModePlayer::RealTime) {
        return qint64(SAMPLE_QUEUE_SIZE);
    }
    return qint64(SAMPLE_QUEUE_SIZE_VOD);
}

qint64 Define::AudioQueueSize() const
{
    if (modePlayer == ModePlayer::RealTime) {
        return qint64(SAMPLE_QUEUE_SIZE);
    }
    return qint64(SAMPLE_QUEUE_SIZE_VOD);
}

ModePlayer Define::getModePlayer() const
{
    return modePlayer;
}

bool Define::isInModeRealTime() const
{
    return  modePlayer == ModePlayer::RealTime;
}

bool Define::isInModeVOD() const
{
    return modePlayer == ModePlayer::VOD;
}
}
