#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include <common/videostate.h>

//#include <QAudioFormat>
#include <QThread>
//#include <qaudio.h>
#ifdef _WIN32
#include "SDL_audio.h"
#elif __linux__
#include <SDL2/SDL_audio.h>
#endif

#ifdef Q_OS_MAC
#include <SDL.h>
#define Uint8 quint8
#endif


constexpr qint16 AUDIO_BUFF_SIZE_LIVE = 512; // byte
constexpr qint16 AUDIO_BUFF_SIZE_VOD = AUDIO_BUFF_SIZE_LIVE * 4; // byte, 2048

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

    bool isRunning() const;

    int initAudioFormat(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, AudioParams *audio_hw_params);

    /* prepare a new audio buffer */
    static void sdl_audio_callback(void *opaque, Uint8 *stream, int len);
//public: signals:
//    void started();
//    void stopped();

protected:
//    bool audioCallback(uint8_t *stream, int len);

    int audioDecodeFrame();

    int synchronizeAudio(int nb_samples);

//private: signals:
//    void initAudioFormatDone();
//private slots:
//    void process();

//    void audioNotify();

//    void hanlerAudioFormatDone();
//    void handleStateChanged(QAudio::State audioState);

//private:
//    void playInitialData(AudioOpenALBackEnd *backend);
//    qint64 audioLength(const AudioParams &audioParam, qint64 microSeconds);
private:
    VideoState *is;
//    QThread *thread;

//    uint8_t buffer[AUDIO_BUFF_SIZE_VOD];
//    int sizeBuffer;

////    AudioOpenALBackEnd *backend;
    bool reqStop;
//    int msSleep;
//    bool hasInitAudioParam;

//    double firstPktAudioPts;

    SDL_AudioDeviceID audio_dev;
    int64_t audio_callback_time;
    QMutex mutexSDLCallback;


    // QObject interface
//protected:
//    void timerEvent(QTimerEvent *event);
};
}
#endif // AUDIORENDER_H
