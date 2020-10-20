#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include <common/videostate.h>

#include <QAudioFormat>
#include <QThread>
#include <qaudio.h>

constexpr qint16 AUDIO_BUFF_SIZE = 4096;

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
    qint64 audioLength(const AudioParams &audioParam, qint64 microSeconds);
private:
    VideoState *is;
    QThread *thread;

    char buffer[AUDIO_BUFF_SIZE];

//    AudioOpenALBackEnd *backend;
    bool reqStop;

    // QObject interface
protected:
    void timerEvent(QTimerEvent *event);
};
}
#endif // AUDIORENDER_H
