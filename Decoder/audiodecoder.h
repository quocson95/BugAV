#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QThread>

#include <common/videostate.h>

namespace BugAV {

class AudioDecoder : public QObject //, public QRunnable
{
    Q_OBJECT
public:
    AudioDecoder(VideoState *is);
    ~AudioDecoder();   

    void start();
    void stop();

    bool isRunning() const;
public: signals:
    void started();
    void stopped();

//protected:
//    void audioCallback(quint8 *stream, int len);
private:
    int getAudioFrame(AVFrame *frame);

//    int audioDecodeFrame();

//    int synchronizeAudio(int nb_samples);

private slots:
    void process();

private:
    QThread *thread;
    VideoState *is;    

    AVFrame *frame;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;
    bool reqStop;
};
}
#endif // AUDIODECODER_H
