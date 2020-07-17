#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QRunnable>
#include <QThread>
extern "C" {
#include <libavutil/frame.h>
}

class QThread;
namespace BugAV {

class VideoState;
class BugFilter;

class VideoDecoder: public QObject //, public QRunnable
{
    Q_OBJECT
public:
    VideoDecoder();
    VideoDecoder(VideoState *is);
    ~VideoDecoder();

    void start();
    void stop();

    void setIs(VideoState *value);

    bool isRunning() const;

//    void setSpeedRate(const double& speedRate);

signals:
    void started();
    void stopped();
private:
    int initPriv();
    int isStreamInputAvail();
    int calcInfo();
    int getFrame();
    int addQueuFrame();

    int getVideoFrame(AVFrame *frame);
    AVFrame *filterFrame(AVFrame *frame);
    int queuePicture(AVFrame *srcFrame, double pts, double duration, int64_t pos, int serial);
private slots:
    void process();
    void threadFinised();
private:
    enum class PrivateState {
        StopState = -1,
        InitState,
        CheckStreamAvailState,
        CalcInfoState,
        GetFrameState,
        AddQueueFrameState,
    };
    VideoState *is;
    QThread *thread;
    AVFrame *frame;
    double pts;
    double duration;
    int ret;
    AVRational tb;
    AVRational frame_rate;    
//    double speedRate;

    bool isRun;

    PrivateState privState;
    bool requetsStop;
    BugFilter *filter;

    // QRunnable interface
public:
    void run();
};
}

#endif // VIDEODECODER_H
