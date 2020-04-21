#include "bugplayer.h"
#include <QThread>
#include <QDebug>

#include "demuxer.h"
#include "render.h"
#include "videodecoder.h"
#include "videostate.h"

BugPlayer::BugPlayer()
    :BugGLWidget()
{
    is = new VideoState;
    demuxer = new Demuxer{is};
    vDecoder = new VideoDecoder{is};
    render = new Render{this, is};
}

BugPlayer::~BugPlayer()
{
    is->abort_request = 1;
//    render->setRenderer(nullptr);
    render->stop();

    demuxer->stop();

    is->vidDecoderAbort();
    vDecoder->stop();

    delete demuxer;
    delete vDecoder;
    delete is;
}

void BugPlayer::setLog()
{
    av_log_set_flags(AV_LOG_SKIP_REPEATED);
}

void BugPlayer::setFile(const QString &file)
{
    is->fileUrl = file;
}

void BugPlayer::start()
{
    demuxer->start();
    vDecoder->start();
    render->start();
}
