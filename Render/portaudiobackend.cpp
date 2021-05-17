#include "portaudiobackend.h"

#include <QThread>
#include <QDebug>

BugAV::PortAudioBackend::PortAudioBackend()
    :initialized{false}
    ,outputParameters{nullptr}
    ,stream{nullptr}

{
    init();
}

BugAV::PortAudioBackend::~PortAudioBackend()
{
    close();
    if (outputParameters) {
        delete outputParameters;
        outputParameters = nullptr;
    }
    if (initialized)
        Pa_Terminate(); //Do NOT call this if init failed. See document
    initialized = false;
}

bool BugAV::PortAudioBackend::isInit()
{
    return initialized;
}

void BugAV::PortAudioBackend::init()
{
    if (initialized) {
        return;
    }
    PaError err = paNoError;
    if ((err = Pa_Initialize()) != paNoError) {
        qWarning("Error when init portaudio: %s", Pa_GetErrorText(err));
        return;
    }

    initialized = true;

    int numDevices = Pa_GetDeviceCount();
    for (int i = 0 ; i < numDevices ; ++i) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo) {
            const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            QString name = QString::fromUtf8(hostApiInfo->name) + QStringLiteral(": ") + QString::fromLocal8Bit(deviceInfo->name);
            qDebug("audio device %d: %s", i, name.toUtf8().constData());
            qDebug("max in/out channels: %d/%d", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
        }
    }
    outputParameters = new PaStreamParameters;
    memset(outputParameters, 0, sizeof(PaStreamParameters));
    outputParameters->device = Pa_GetDefaultOutputDevice();
    if (outputParameters->device == paNoDevice) {
        qWarning("PortAudio get device error!");
        return;
    }
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outputParameters->device);
    qDebug("DEFAULT max in/out channels: %d/%d", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
    qDebug("audio device: %s", QString::fromLocal8Bit(Pa_GetDeviceInfo(outputParameters->device)->name).toUtf8().constData());
    outputParameters->hostApiSpecificStreamInfo = NULL;
    outputParameters->suggestedLatency = Pa_GetDeviceInfo(outputParameters->device)->defaultHighOutputLatency;
}

int BugAV::PortAudioBackend::open(AudioParams audio_hw_params)
{

    close();
    isSupport = false;    

    this->audio_hw_params = audio_hw_params;
    outputParameters->sampleFormat = paInt16;
    outputParameters->channelCount = this->audio_hw_params.channels;

    isSupport = Pa_IsFormatSupported(nullptr, outputParameters, this->audio_hw_params.freq) == paNoError;
    if (!isSupport) {
        return -1;
    }
    PaError err = Pa_OpenStream(&stream, NULL, outputParameters, this->audio_hw_params.freq,
                                paFramesPerBufferUnspecified, paNoFlag, NULL, NULL);
    if (err != paNoError) {
       qWarning("Open portaudio stream error: %s", Pa_GetErrorText(err));
       return -1;
    }
    outputLatency = Pa_GetStreamInfo(stream)->outputLatency;
    auto size = this->audio_hw_params.bytes_per_sec / 8 ;

    return size;
}

void BugAV::PortAudioBackend::close()
{
    QMutexLocker lock(&mutex);
    if (stream == nullptr) {
        return ;
    }
    PaError err = Pa_AbortStream(stream); //may be already stopped: paStreamIsStopped
    if (err != paNoError) {
        qWarning("Stop portaudio stream error: %s", Pa_GetErrorText(err));
        //return err == paStreamIsStopped;
    }
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        qWarning("Close portaudio stream error: %s", Pa_GetErrorText(err));
    }
    stream = nullptr;
}

int BugAV::PortAudioBackend::write(uint8_t *data, int size)
{
    QMutexLocker locker(&mutex);
    if (size <= 0) {
        return 0;
    }
    if (!isSupport) {
        return 0;
    }
    if (Pa_IsStreamStopped(stream)) {
        auto err = Pa_StartStream(stream);
        if (err > 0) {
            qDebug() << "Write portaudio stream error: "<<  Pa_GetErrorText(err);
        }
        return 0;
    }
    auto frames = size / audio_hw_params.frame_size;
    if (frames <= 0) {
        return true;
    }
//    qDebug() << "Write audio " << size;
    PaError err = Pa_WriteStream(stream, data, frames );
    if (err == paUnanticipatedHostError) {
        qDebug() << "Write portaudio stream error: "<<  Pa_GetErrorText(err);
        return   0;
    }
    return 0;
}

