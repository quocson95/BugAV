#include "audiorender.h"
#include <QtMath>
#include <QAudioOutput>
#include <QDebug>
#include <QBuffer>
#include <QAudioEncoderSettingsControl>
#include <QTimerEvent>
#include <QFile>

extern "C" {
#include "libavutil/time.h"
#include "libavutil/channel_layout.h"
}

#include "audioopenalbackend.h"

BugAV::AudioRender::AudioRender(BugAV::VideoState *is):
    QObject(nullptr)
  ,is{is}
  ,thread{nullptr}
  ,audio{nullptr}
  ,kCheckBufferAudio{-1}
  ,speed{1.0}
{
    connect(this, &AudioRender::initAudioFormatDone, this, &AudioRender::hanlerAudioFormatDone);
}

BugAV::AudioRender::~AudioRender()
{
    if (audio != nullptr) {
        audio->stop();
        input->setBuffer(nullptr);
    }
}

void BugAV::AudioRender::start()
{
    reqStop = false;
    if (thread == nullptr) {
        thread = new QThread{this};
        moveToThread(thread);
        connect(thread, &QThread::started, this, &AudioRender::process);
    }
    if (thread->isRunning()) {
        return;
    }
    thread->start();

}

void BugAV::AudioRender::stop()
{
    reqStop = true;
    if (thread == nullptr) {
        return;
    }
    thread->quit();
    do {
        thread->wait(500);
    } while(thread->isRunning());
    if (audio != nullptr) {
        audio->stop();
    }
}

int BugAV::AudioRender::initAudioFormat(int64_t wanted_channel_layout,
                                   int wanted_nb_channels,
                                   int wanted_sample_rate,
                                   AudioParams *audio_hw_params)
{
    if (audio != nullptr) {
        return 0;
    }
    QAudioFormat wanted_spec, spec;

    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.setChannelCount(wanted_nb_channels);
    wanted_spec.setSampleRate(wanted_sample_rate);
    if (wanted_spec.sampleRate() <= 0 || wanted_spec.channelCount() <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    auto next_sample_rate_idx = FF_ARRAY_ELEMS(NEXT_SAMPLE_RATES) - 1;
    while (next_sample_rate_idx && NEXT_SAMPLE_RATES[next_sample_rate_idx] >= wanted_spec.sampleRate()) {
        next_sample_rate_idx--;
    }
//    wanted_spec.setByteOrder(QAudioFormat::LittleEndian);
    wanted_spec.setSampleType(QAudioFormat::SignedInt);
    wanted_spec.setCodec("audio/pcm");
//    wanted_spec.setSampleSize(FFMAX(
//                                  SDL_AUDIO_MIN_BUFFER_SIZE,
//                                  2 << av_log2(wanted_spec.sampleRate() / SDL_AUDIO_MAX_CALLBACKS_PER_SEC))
//                              );
    wanted_spec.setSampleSize(16);
    forever {
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        auto isSupport = info.isFormatSupported(wanted_spec);
        if (isSupport) {
            break;
        }
         QList<int> sampleRatesList ;
        sampleRatesList += info.supportedSampleRates();
        sampleRatesList = sampleRatesList.toSet().toList(); // remove duplicates
        qSort(sampleRatesList);
        qDebug() << "Engine::initialize frequenciesList" << sampleRatesList;

        QList<int> channelsList;
        channelsList += info.supportedChannelCounts();
        channelsList = channelsList.toSet().toList();
        qDebug() << "Engine::initialize channelsList" << channelsList;
        qWarning() << "wanted_spec " << wanted_spec.channelCount()<< "channels, "
                   << wanted_spec.sampleRate() << " Hz "
                   << "sample size " << wanted_spec.sampleSize()
                   << "format not supported by backend, trying another spec.";
        auto channels = NEXT_NB_CHANNELS[FFMIN(7, wanted_spec.channelCount())];
        wanted_spec.setChannelCount(channels);
        if (!wanted_spec.channelCount()) {
            wanted_spec.setSampleRate(NEXT_SAMPLE_RATES[next_sample_rate_idx--]);
            wanted_spec.setChannelCount(wanted_nb_channels);
            if (!wanted_spec.sampleRate()) {
                av_log(NULL, AV_LOG_ERROR,
                                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channelCount());
    }

    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = wanted_spec.sampleRate();
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels = wanted_spec.channelCount();
    audio_hw_params->frame_size = av_samples_get_buffer_size(nullptr, audio_hw_params->channels,
                                                             1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(nullptr, audio_hw_params->channels,
                                                                audio_hw_params->freq, audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
                return -1;
    }
    wanted_spec.setSampleType(QAudioFormat::UnSignedInt);
    format = wanted_spec;
    emit initAudioFormatDone();
    return 0;
//    audio = new QAudioOutput(wanted_spec, this);
//    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
//    byteBuffer = new QByteArray();
//    byteBuffer->resize(audio->bufferSize());
//    input = new QBuffer(byteBuffer, this);
//    input->open(QIODevice::ReadOnly);
//    audio->start(input);
//    return audio->bufferSize();
//    format.setSampleRate(8000);
//    format.setChannelCount(1);
//    format.setSampleSize(8);
//    format.setCodec("audio/pcm");
//    format.setByteOrder(QAudioFormat::LittleEndian);
//    format.setSampleType(QAudioFormat::UnSignedInt);
//    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
//    if (!info.isFormatSupported(format)) {
//        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
//        return;
//    }

//    audio = new QAudioOutput(format, this);
    //    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
}

void BugAV::AudioRender::setSpeed(const double &speed)
{
    if (this->speed == speed) {
        return;
    }
    if (speed == 1.0) {
//        start();
    } else {
        is->sampq->wakeSignal();
//        stop();
        is->sampq->reset();
    }

    this->speed = speed;
}

char * BugAV::AudioRender::audioCallback(char * stream, int len)
{

//    int len = stream->capacity();
//    auto len = audioLength(format, 100000);
//    qDebug() << "audioCallback len =" <<len;
    int audioSize, len1;
    auto audio_callback_time = av_gettime_relative();
//    audioDecodeFrame();
//    return (char *)is->audio_buf;
    qDebug() << "audioCallback " << len;
    while (len > 0) {
        if (is->audio_buf_index >= int(is->audio_buf_size)) {
            audioSize = audioDecodeFrame();
            if (audioSize < 0) {
                is->audio_buf = nullptr;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
            } else {
                is->audio_buf_size = audioSize;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        if (!is->muted && is->audio_buf && is->audio_volume == MIX_MAX_VOLUME) {
            // memcp
//            stream->replace(0, len1, (const char *)(is->audio_buf + is->audio_buf_index));
//            stream->replace(0, len1, (unsigned char *)(is->audio_buf + is->audio_buf_index));
//            auto d = stream->data();
            memcpy(stream, (char *)is->audio_buf + is->audio_buf_index, len1);
//            stream->setRawData(d);
        } else {
            // memset
//            stream->fill(0, len1);
             memset(stream, 0, len1);
            if (!is->muted && is->audio_buf) {
                memcpy(stream, (char *)is->audio_buf + is->audio_buf_index, len1);
                // SDL_MixAudioFormat
            }
        }
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    if (!qIsNaN(is->audio_clock)) {
        auto clockVal = is->audio_clock -
                double(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / double(is->audio_tgt.bytes_per_sec);
        is->audclk.setAt(clockVal, is->audio_clock_serial, audio_callback_time / 1000000.0);
        is->extclk.syncToSlave(&is->audclk);
    }    
     return stream;
}

int BugAV::AudioRender::audioDecodeFrame()
{
    int data_size, resampled_data_size;
   int64_t dec_channel_layout;
   int wanted_nb_samples;
   Frame *af;
   if (is->paused) {
       return  -1;
   }
   do {
#if defined(_WIN32)
        while (frame_queue_nb_remaining(&is->sampq) == 0) {
            if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
                return -1;
//            av_usleep (1000);
            return -100;
        }
#endif
        if (!(af = is->sampq->peekReadable())) {
            return -1;
        }
        is->sampq->queueNext();
   } while (af->serial != is->audioq->serial);
   auto avFrame = af->frame;
   data_size = av_samples_get_buffer_size(nullptr, avFrame->channels,
                                          avFrame->nb_samples,
                                          AVSampleFormat(avFrame->format),
                                          1);
   dec_channel_layout =
           (avFrame->channel_layout && avFrame->channels == av_get_channel_layout_nb_channels(avFrame->channel_layout)) ?
               avFrame->channel_layout : av_get_default_channel_layout(avFrame->channels);
   // sync audio
   wanted_nb_samples = synchronizeAudio(af->frame->nb_samples);
   if (avFrame->format          != is->audio_src.fmt                ||
           dec_channel_layout   != is->audio_src.channel_layout     ||
           avFrame->sample_rate != is->audio_src.freq               ||
           (wanted_nb_samples   != avFrame->nb_samples && !is->swr_ctx)) {
       swr_free(&is->swr_ctx);
       is->swr_ctx = swr_alloc_set_opts(nullptr,
                                        is->audio_tgt.channel_layout, AVSampleFormat(is->audio_tgt.fmt), is->audio_tgt.freq,
                                        dec_channel_layout, AVSampleFormat(avFrame->format), avFrame->sample_rate,
                                        0, nullptr);
       if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
           av_log(NULL, AV_LOG_ERROR,
                  "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(AVSampleFormat(avFrame->format)), af->frame->channels,
                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
           swr_free(&is->swr_ctx);
           return -1;
       }
       is->audio_src.channel_layout = dec_channel_layout;
       is->audio_src.channels = avFrame->channels;
       is->audio_src.freq = avFrame->sample_rate;
       is->audio_src.fmt = AVSampleFormat(avFrame->format);
   }
    if (is->swr_ctx) {
        const auto& in = (const quint8 **)(af->frame->extended_data);
        auto out = &is->audio_buf1;
        int out_count = qint64(wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256);
        int out_size = av_samples_get_buffer_size(nullptr, is->audio_tgt.channels, out_count,
                                                 AVSampleFormat(is->audio_tgt.fmt), 0);
        if (out_size < 0) {
           av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
           if (swr_set_compensation(is->swr_ctx,
                                    (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                                    wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0 ) {
               av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                               return -1;
           }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
           av_log(NULL, AV_LOG_ERROR, "av_fast_malloc(audio_buf1) failed\n");
                           return -1;
           return AVERROR(ENOMEM);
        }
        auto len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0) {
                swr_free(&is->swr_ctx);
            }
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(AVSampleFormat(is->audio_tgt.fmt));
    } else {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
//        qDebug() << "Audio src freq " << is->audio_src.freq;
    }
    auto audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!qIsNaN(af->getPts())) {
        is->audio_clock = af->getPts() + double(af->frame->nb_samples) / double(af->frame->sample_rate);
    } else {
        is->audio_clock = NAN;
    }
    is->audio_clock_serial = af->serial;
    if (is->debug) {
        static double last_clock;
        qDebug("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
               is->audio_clock - last_clock,
               is->audio_clock, audio_clock0);
        last_clock = is->audio_clock;
    }
    return  resampled_data_size;
}

int BugAV::AudioRender::synchronizeAudio(int nb_samples)
{
    auto wanted_nb_samples = nb_samples;
    if (is->isAudioClock()) {
        auto diff = is->audclk.get() - is->getMasterClock();
        if (!qIsNaN(diff) && qFabs(diff) < AV_NOSYNC_THRESHOLD) {
            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                /* not enough measures to have a correct estimate */
                is->audio_diff_avg_count++;
            } else {
                /* estimate the A-V difference */
                auto avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                if (qFabs(avg_diff) >- is->audio_diff_threshold) {
                    wanted_nb_samples = nb_samples + int(diff * double(is->audio_src.freq));
                    auto min_min_nb_samples = nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    auto max_nb_samples = nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                    wanted_nb_samples = av_clip(wanted_nb_samples, min_min_nb_samples, max_nb_samples);
                }
                if (is->debug) {
                    av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
                                            diff, avg_diff, wanted_nb_samples - nb_samples,
                                            is->audio_clock, is->audio_diff_threshold);
                }
            }
        } else {
            /* too big difference : may be initial PTS errors, so
                           reset A-V filter */
            is->audio_diff_avg_count = 0;
            is->audio_diff_cum = 0;
            if (is->debug) {
                av_log(NULL, AV_LOG_TRACE, "Audio too big difference : may be initial PTS errors, so reset A-V filter ");
            }
        }
    }
    return wanted_nb_samples;
}

void BugAV::AudioRender::process()
{
    backend = new AudioOpenALBackEnd();    
    byteBuffer.resize(4096);
    backend->setAudioParam(is->audio_tgt);
    backend->open();
    while (!reqStop) {
        auto data = audioCallback(buffer, 4096);
        if (reqStop) {
            break;
        }
        backend->write(QByteArray::fromRawData(buffer, 4096));
        while (!reqStop && backend->isBufferProcessed()) {
            thread->msleep(10);
        }
    }       
    delete backend;
    thread->terminate();
    emit stopped();
}

void BugAV::AudioRender::audioNotify()
{
//    audioCallback(byteBuffer);
}

void BugAV::AudioRender::hanlerAudioFormatDone()
{
//    audio = new QAudioOutput(format, nullptr);
//    audio->start(input);
//    audioCallback(byteBuffer);
//    kCheckBufferAudio = startTimer(50);
    start();
}

void BugAV::AudioRender::handleStateChanged(QAudio::State audioState)
{
     qDebug() << "Audio state change " << audioState;
    if (audioState == QAudio::State::IdleState) {
        qDebug() << "AudioRender callback";
//        audioCallback(byteBuffer);
    }
}

qint64 BugAV::AudioRender::audioLength(const QAudioFormat &format, qint64 microSeconds)
{
    qint64 result = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8))
        * microSeconds / 100000;
    result -= result % (format.channelCount() * format.sampleSize());
    return result;
}

void BugAV::AudioRender::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == kCheckBufferAudio) {
//        audioCallback(byteBuffer);
    }
}
