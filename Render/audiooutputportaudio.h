#ifndef AUDIOOUTPUTPORTAUDIO_H
#define AUDIOOUTPUTPORTAUDIO_H
#include "IAudioBackend.h"
#include "audioopenalbackend.h"
extern "C" {
#include "libavutil/samplefmt.h"
}

#include <QObject>
#include <portaudio.h>
#include <QAudioFormat>


namespace BugAV {
class VideoState;
class AudioParams;

class AudioOutputPortAudio: public  QObject, public IAudioBackEnd
{
    Q_OBJECT
public:
    explicit AudioOutputPortAudio(VideoState *is);
    ~AudioOutputPortAudio();

    int open(int wanted_channel_layout,
              int wanted_nb_channels,
              int wanted_sample_rate,
              AudioParams *audio_hw_params);

    int close();

private slots:
      void process();
private:
    void renderAudio();

    int audioDecodeFrame();
    int synchronizeAudio(int nb_samples);
private:
    VideoState *is;
//    bool initialized;
//    PaStreamParameters *outputParameters;
//    PaStream *stream;
//    double outputLatency;

    QThread *curThread;

    bool reqStop;
//    bool isSupport;
    int64_t audio_callback_time;
//    uint8_t *buffer;

//    AudioOpenALBackEnd *openAl;


};
}

#endif // AUDIOOUTPUTPORTAUDIO_H
