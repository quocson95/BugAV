#ifndef PORTAUDIOBACKEND_H
#define PORTAUDIOBACKEND_H
#include <QObject>
#include <QtGlobal>
#include <portaudio.h>

#include <common/common.h>

namespace BugAV {

class AudioParams;

class PortAudioBackend
{
public:
    PortAudioBackend();
    ~PortAudioBackend();

    bool isInit();

    int open(AudioParams audio_hw_params);

    void close();

    int write(uint8_t *data, int size);

private:
    void init();
private:
    bool initialized;
    PaStreamParameters *outputParameters;
    PaStream *stream;
    double outputLatency;
    bool isSupport;
    AudioParams audio_hw_params;

    QMutex mutex;
};


#define PortAudioBackendDefault BugAV::PortAudioBackendImpl::instance()
class PortAudioBackendImpl {
public:
    static PortAudioBackend *instance() {
        static PortAudioBackendImpl ins;
        return ins.audioBackend;
    }

    PortAudioBackendImpl(const PortAudioBackendImpl &) = delete;
    void operator=(PortAudioBackendImpl const&) = delete;

private:
    PortAudioBackendImpl() {
        audioBackend = new PortAudioBackend;
    };
    ~PortAudioBackendImpl() {
        delete audioBackend;
    }
    PortAudioBackend *audioBackend;
};
}
#endif // PORTAUDIOBACKEND_H
