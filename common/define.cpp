#include "define.h"

namespace BugAV {
Define::Define()
{
    liveMode = true;
}

void Define::setModeLive(bool modeLive)
{
    liveMode = modeLive;
}

qint64 Define::MaxQueueSize()
{
    if (liveMode) {
        return qint64(MAX_QUEUE_SIZE);
    }
    return qint64(MAX_QUEUE_SIZE_VOD);
}

int Define::VideoPictureQueueSize()
{
    if (liveMode) {
        return int(VIDEO_PICTURE_QUEUE_SIZE);
    }
    return int(VIDEO_PICTURE_QUEUE_SIZE_VOD);
}

qint64 Define::FrameQueueSize()
{
    if (liveMode) {
        return qint64(VIDEO_PICTURE_QUEUE_SIZE);
    }
    return qint64(VIDEO_PICTURE_QUEUE_SIZE_VOD);
}
}
