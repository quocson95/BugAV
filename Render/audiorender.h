#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include <common/videostate.h>

#include <QAudioFormat>
#include <QThread>
#include <qaudio.h>

class QAudioOutput;
class QBuffer;

namespace BugAV {
class AudioOpenALBackEnd;
class AudioRender: public QObject
{
    Q_OBJECT
public:
    AudioRender(VideoState *is);
    ~AudioRender();

    void start();
    void stop();

    int initAudioFormat(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, AudioParams *audio_hw_params);

    void setSpeed(const double& speed);

public: signals:
    void started();
    void stopped();

protected:
    char *audioCallback(char *stream, int len);

    int audioDecodeFrame();

    int synchronizeAudio(int nb_samples);

private: signals:
    void initAudioFormatDone();
private slots:
    void process();

    void audioNotify();

    void hanlerAudioFormatDone();
    void handleStateChanged(QAudio::State audioState);

private:
    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);
private:
    VideoState *is;
    QThread *thread;
    QBuffer* input;
    QByteArray byteBuffer;

    QAudioOutput* audio; // class member.
    QAudioFormat format;

    char buffer[4096];
    int kCheckBufferAudio;

    AudioOpenALBackEnd *backend;
    double speed;
    bool reqStop;

    // QObject interface
protected:
    void timerEvent(QTimerEvent *event);
};
}
#endif // AUDIORENDER_H
