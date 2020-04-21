#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
extern "C" {
#include <libavutil/frame.h>
}

class QThread;
class VideoState;
class VideoDecoder: public QObject
{
    Q_OBJECT
public:
    VideoDecoder();
    VideoDecoder(VideoState *is);
    ~VideoDecoder();

    void start();
    void stop();

    void setIs(VideoState *value);

private:
    int getVideoFrame(AVFrame *frame);
    int queuePicture(AVFrame *srcFrame, double pts, double duration, int64_t pos, int serial);
private slots:
    void process();
    void threadFinised();
private:
    VideoState *is;
    QThread *thread;
    AVFrame *frame;
    double pts;
    double duration;
    int ret;
    AVRational tb;
    AVRational frame_rate;    
};

#endif // VIDEODECODER_H
