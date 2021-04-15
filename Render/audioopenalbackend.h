//#ifdef xxx
#ifndef AUDIOOPENALBACKEND_H
#define AUDIOOPENALBACKEND_H

#include <AL/al.h>
#include <AL/alc.h>

#include <QMutex>
#include <QVector>
#include <QWaitCondition>

#include <common/common.h>

namespace BugAV {
constexpr quint8 NUM_BUFFERS_OPENAL = 25;
class AudioOpenALBackEnd
{    
public:
    AudioOpenALBackEnd();
    ~AudioOpenALBackEnd();

    bool open();
    bool close();

    void setAudioParam(const AudioParams &value);

//    bool write(const QByteArray data);
    int write(uint8_t *data, int size);
    bool play();

    ALint waitBufferProcessed();

    bool isBufferProcessed();

    bool openDevice();

    static ALenum to_al_format(short channels, short samples);

private:

    ALCdevice *device;
    ALCcontext *context;
    ALenum format_al;
//    QVector<ALuint> buffer;
    ALuint buffer[NUM_BUFFERS_OPENAL];
    ALuint buffer_count = NUM_BUFFERS_OPENAL;
    ALuint source;
    ALint state;
    bool available;
    QMutex mutex;
    QWaitCondition cond;
    AudioParams audioParam;
    bool initBuffer;
    int bufferNumer;
    // used for 1 context per instance. lock when makeCurrent
        static QMutex global_mutex;
};
}

#endif // AUDIOOPENALBACKEND_H
//#endif
