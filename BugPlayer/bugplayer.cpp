#include "bugplayer.h"
#include <QDebug>
#include "common/videostate.h"
#include "Decoder/videodecoder.h"
#include "Demuxer/demuxer.h"
#include "Render/render.h"

BugPlayer::BugPlayer()
    :BugGLWidget()
{
//    moveToThread(QThreadPool::globalInstance());
    is = new VideoState;
    demuxer = new Demuxer{is};
    vDecoder = new VideoDecoder{is};
    render = new Render{this, is};

    demuxerRunning = false;
    vDecoderRunning = false;
    renderRunning = false;

    connect(demuxer, &Demuxer::loadDone, this, &BugPlayer::streamLoaded);
//    connect(demuxer, &Demuxer::started, this, &BugPlayer::workerStarted);
//    connect(demuxer, &Demuxer::stopped, this, &BugPlayer::workerStopped);

//    connect(vDecoder, &VideoDecoder::started, this, &BugPlayer::workerStarted);
//    connect(vDecoder, &VideoDecoder::stopped, this, &BugPlayer::workerStopped);

//    connect(render, &Render::started, this, &BugPlayer::workerStarted);
//    connect(render, &Render::stopped, this, &BugPlayer::workerStopped);

    QVariantMap avformat;
    avformat["probesize"] = 4096000;
    avformat["analyzeduration"] = 1000000;
    demuxer->setAvformat(avformat);
}

BugPlayer::~BugPlayer()
{
    is->abort_request = 1;
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
    curFile = file;
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
//                if (!is->realtime) {
                    is->streamTogglePause();
                    is->step = 0;
//                } else {
//                    reset();
//                }
            }
        }
        return;
    }
    playPriv();
}

void BugPlayer::pause()
{
//    if (!isPlaying() || is->paused) {
//        return;
//    }
    is->streamTogglePause();
    is->step = 0;
}

void BugPlayer::stop()
{
    is->abort_request = 1;
    render->stop();
    demuxer->stop();
    is->vidDecoderAbort();
    vDecoder->stop();
}

void BugPlayer::reset()
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

void BugPlayer::initPriv()
{
    is->reset();
}

void BugPlayer::playPriv()
{
    initPriv();
    render->setRequestStop(false);
    is->fileUrl = curFile;
    demuxer->start();
//    threadvDecoder->start();
//    threadRender->start();

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
        vDecoder->start();
        render->start();
    }
}
