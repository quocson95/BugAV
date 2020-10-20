#include "audioopenalbackend.h"
#include <QDebug>
#define NUM_BUFFERS 2


BugAV::AudioOpenALBackEnd::AudioOpenALBackEnd():
    device{nullptr}
  ,context{nullptr}
  ,buffer_count{NUM_BUFFERS}
  ,state{0}
  ,available{true}  
  ,initBuffer{true}
  ,bufferNumer{0}
{
    QVector<QByteArray> _devices;
    const char *p = NULL;
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        // ALC_ALL_DEVICES_SPECIFIER maybe not defined
        p = alcGetString(NULL, alcGetEnumValue(NULL, "ALC_ALL_DEVICES_SPECIFIER"));
    } else {
        p = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }
    while (p && *p) {
        _devices.push_back(p);
        p += _devices.last().size() + 1;
    }
    qDebug() << _devices;
    available = openDevice(); //ensure isSupported(AudioFormat) works correctly
}

BugAV::AudioOpenALBackEnd::~AudioOpenALBackEnd()
{
    close();
}

QMutex BugAV::AudioOpenALBackEnd::global_mutex;
#define AL_ENSURE(expr, ...) \
    do { \
        expr; \
        const ALenum err = alGetError(); \
        if (err != AL_NO_ERROR) { \
            qWarning("AudioOutputOpenAL Error>>> " #expr " (%d) : %s", err, alGetString(err)); \
            return __VA_ARGS__; \
        } \
    } while(0)

#define SCOPE_LOCK_CONTEXT() \
    QMutexLocker ctx_lock(&global_mutex); \
    Q_UNUSED(ctx_lock); \
    if (context) \
        alcMakeContextCurrent(context)

bool BugAV::AudioOpenALBackEnd::open()
{
    if (!openDevice())
            return false;
        {
        SCOPE_LOCK_CONTEXT();
        // alGetString: alsoft needs a context. apple does not
        qDebug("OpenAL %s vendor: %s; renderer: %s", alGetString(AL_VERSION), alGetString(AL_VENDOR), alGetString(AL_RENDERER));
        //alcProcessContext(ctx); //used when dealing witg multiple contexts
        ALCenum err = alcGetError(device);
        if (err != ALC_NO_ERROR) {
            qWarning("AudioOutputOpenAL Error: %s", alcGetString(device, err));
            return false;
        }
        qDebug("device: %p, context: %p", device, context);
        //init params. move to another func?
        format_al = to_al_format(audioParam.channels, 16);

//        buffer.resize(buffer_count);
        alGenBuffers(NUM_BUFFERS, buffer);
//        alGenBuffers(1, &buffer);
        err = alGetError();
        if (err != AL_NO_ERROR) {
            qWarning("Failed to generate OpenAL buffers: %s", alGetString(err));
            goto fail;
        }
        alGenSources(1, &source);
        err = alGetError();
        if (err != AL_NO_ERROR) {
            qWarning("Failed to generate OpenAL source: %s", alGetString(err));
            alDeleteBuffers(NUM_BUFFERS, buffer);
//            alDeleteBuffers(1, &buffer);
            goto fail;
        }
        alSourcei(source, AL_LOOPING, AL_FALSE);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(source, AL_ROLLOFF_FACTOR, 0);
        alSource3f(source, AL_POSITION, 0.0, 0.0, 0.0);
        alSource3f(source, AL_VELOCITY, 0.0, 0.0, 0.0);
        alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
        state = 0;
        qDebug("AudioOutputOpenAL open ok...");
        }
        return true;
    fail:
        alcMakeContextCurrent(NULL);
        alcDestroyContext(context);
        alcCloseDevice(device);
        context = 0;
        device = 0;
        return false;
}

bool BugAV::AudioOpenALBackEnd::close()
{
    state = 0;
    if (!context)
        return true;
    SCOPE_LOCK_CONTEXT();
    alSourceStop(source);
    do {
        alGetSourcei(source, AL_SOURCE_STATE, &state);
    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING);
    ALint processed = 0; //android need this!! otherwise the value may be undefined
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    ALuint buf;
    while (processed-- > 0) { alSourceUnqueueBuffers(source, 1, &buf); }
    alDeleteSources(1, &source);
    alDeleteBuffers(NUM_BUFFERS, buffer);
//    alDeleteBuffers(1, &buffer);

    alcMakeContextCurrent(NULL);
    qDebug("alcDestroyContext(%p)", context);
    alcDestroyContext(context);
    ALCenum err = alcGetError(device);
    if (err != ALC_NO_ERROR) { //ALC_INVALID_CONTEXT
        qWarning("AudioOutputOpenAL Failed to destroy context: %s", alcGetString(device, err));
        return false;
    }
    context = nullptr;
    if (device) {
        qDebug("alcCloseDevice(%p)", device);
        alcCloseDevice(device);
        // ALC_INVALID_DEVICE now
        device = nullptr;
    }
    return true;
}

bool BugAV::AudioOpenALBackEnd::write(const QByteArray data)
{    
    if (data.isEmpty())
        return false;
    if (context == nullptr) {
        return false;
    }
    SCOPE_LOCK_CONTEXT();
    if (initBuffer) {
        AL_ENSURE(alBufferData(buffer[bufferNumer], format_al, data.constData(), data.size(), audioParam.freq), false);
        if (bufferNumer == NUM_BUFFERS - 1) {
            AL_ENSURE(alSourceQueueBuffers(source, NUM_BUFFERS, buffer), false);
            alSourcePlay(source);
            initBuffer = 0;
        }
        // update buffer number to fill
            bufferNumer = (bufferNumer + 1) % 3;
    } else {
        ALuint buffer;
        auto val = waitBufferProcessed();
        AL_ENSURE(alSourceUnqueueBuffers(source, 1, &buffer), false);
        AL_ENSURE(alBufferData(buffer, format_al, data.constData(),
            data.size(), audioParam.freq), false);
        AL_ENSURE(alSourceQueueBuffers(source, 1, &buffer), false);
        if (alGetError() != AL_NO_ERROR) {
                                return 1;
                            }
        alGetSourcei(source, AL_SOURCE_STATE, &val);
        if (val != AL_PLAYING)
            alSourcePlay(source);
    }
//    ALuint buf = 0;
//    if (state <= 0) { //state used for filling initial data
//        buf = buffer[(-state)%buffer_count];
//        --state;
//    } else {
//        AL_ENSURE(alSourceUnqueueBuffers(source, 1, &buf), false);
//    }
//    AL_ENSURE(alBufferData(buf, format_al, data.constData(), data.size(), audioParam.freq), false);
//    AL_ENSURE(alSourceQueueBuffers(source, 1, &buf), false);
    return true;
}

bool BugAV::AudioOpenALBackEnd::play()
{
    SCOPE_LOCK_CONTEXT();
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {        
        qDebug("AudioOutputOpenAL: !AL_PLAYING alSourcePlay");
        alSourcePlay(source);
    }
    return true;
}

ALint BugAV::AudioOpenALBackEnd::waitBufferProcessed()
{
    if (initBuffer) {
        return 0;
    }
    ALint val;   
    do {        
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
    } while (val <= 0);
    return val;
}

bool BugAV::AudioOpenALBackEnd::isBufferProcessed()
{
    if (initBuffer) {
        return 0;
    }
    ALint val;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &val);
    return  val <= 0;
}

bool BugAV::AudioOpenALBackEnd::openDevice()
{
    if (context)
        return true;
    const ALCchar *default_device = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    qDebug("OpenAL opening default device: %s", default_device);
    device = alcOpenDevice(NULL); //parameter: NULL or default_device
    if (!device) {
        qWarning("OpenAL failed to open sound device: %s", alcGetString(0, alcGetError(0)));
        return false;
    }
    qDebug("AudioOutputOpenAL creating context...");
    context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);
    return true;
}

ALenum BugAV::AudioOpenALBackEnd::to_al_format(short channels, short samples)
{    
//    return AL_FORMAT_STEREO16;
        bool stereo = (channels > 1);
        switch (samples) {
        case 16:
                if (stereo)
                        return AL_FORMAT_STEREO16;
                else
                        return AL_FORMAT_MONO16;
        case 8:
                if (stereo)
                        return AL_FORMAT_STEREO8;
                else
                        return AL_FORMAT_MONO8;
        default:
                return -1;
        }
}

void BugAV::AudioOpenALBackEnd::setAudioParam(const AudioParams &value)
{
    audioParam = value;
}
