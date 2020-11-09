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
#include <SDL2/SDL_audio.h>
#include "SDL2/SDL.h"

BugAV::AudioRender::AudioRender(BugAV::VideoState *is):
    QObject(nullptr)
  ,is{is}
  ,audio_dev{0}
//  ,thread{nullptr}
//  ,hasInitAudioParam{false}
{
//    connect(this, &AudioRender::initAudioFormatDone, this, &AudioRender::hanlerAudioFormatDone);
//    sizeBuffer = 0;
}

BugAV::AudioRender::~AudioRender()
{
//    stop();
}

void BugAV::AudioRender::start()
{
    reqStop = false;
    SDL_PauseAudioDevice(audio_dev, 0);
//    msSleep = 0;
//    if (is->isRealtime()) {
//        sizeBuffer = AUDIO_BUFF_SIZE_LIVE;
//    } else {
//        sizeBuffer = AUDIO_BUFF_SIZE_VOD;
//    }
//    if (thread == nullptr) {
//        thread = new QThread{this};
//        moveToThread(thread);
//        connect(thread, &QThread::started, this, &AudioRender::process);
//    }
//    if (thread->isRunning()) {
//        return;
//    }
//    thread->start();
}

void BugAV::AudioRender::stop()
{
    reqStop = true;
    if (audio_dev > 0) {
        SDL_CloseAudioDevice(audio_dev);
    }
    audio_dev = 0;
}

bool BugAV::AudioRender::isRunning() const
{
    return !reqStop;
}

int BugAV::AudioRender::initAudioFormat(int64_t wanted_channel_layout,
                                   int wanted_nb_channels,
                                   int wanted_sample_rate,
                                   AudioParams *audio_hw_params)
{    
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        qDebug("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    } else {
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
        SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
        qDebug("SDL_INIT_AUDIO ok");
    }

    SDL_AudioSpec wanted_spec, spec;
        const char *env;
        static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
        static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
        int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

        env = SDL_getenv("SDL_AUDIO_CHANNELS");
        if (env) {
            wanted_nb_channels = atoi(env);
            wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        }
        if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
            wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
            wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
        }
        wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
        wanted_spec.channels = wanted_nb_channels;
        wanted_spec.freq = wanted_sample_rate;
        if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
            av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
            return -1;
        }
        while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
            next_sample_rate_idx--;
        }
        wanted_spec.format = AUDIO_S16SYS;
        wanted_spec.silence = 0;
        wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
        wanted_spec.callback = BugAV::AudioRender::sdl_audio_callback;
        wanted_spec.userdata = this;
        while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
            qDebug("SDL_OpenAudio (%d channels, %d Hz): %s\n",
                   wanted_spec.channels, wanted_spec.freq, SDL_GetError());
            wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
            if (!wanted_spec.channels) {
                wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
                wanted_spec.channels = wanted_nb_channels;
                if (!wanted_spec.freq) {
                    av_log(NULL, AV_LOG_ERROR,
                           "No more combinations to try, audio open failed\n");
                    return -1;
                }
            }
            wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
        }
        if (spec.format != AUDIO_S16SYS) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised audio format %d is not supported!\n", spec.format);
            return -1;
        }
        if (spec.channels != wanted_spec.channels) {
            wanted_channel_layout = av_get_default_channel_layout(spec.channels);
            if (!wanted_channel_layout) {
                av_log(NULL, AV_LOG_ERROR,
                       "SDL advised channel count %d is not supported!\n", spec.channels);
                return -1;
            }
        }

        audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
        audio_hw_params->freq = spec.freq;
        audio_hw_params->channel_layout = wanted_channel_layout;
        audio_hw_params->channels =  spec.channels;
        audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
        audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
        if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
            return -1;
        }
        return spec.size;
}

void BugAV::AudioRender::sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
        AudioRender *ar = static_cast<AudioRender *>(opaque);
        auto is = ar->is;
        int audio_size, len1;

        ar->audio_callback_time = av_gettime_relative();

        while (len > 0) {
            if (is->audio_buf_index >= is->audio_buf_size) {
               audio_size = ar->audioDecodeFrame();
               if (audio_size < 0) {
                    /* if error, just output silence */
                   is->audio_buf = NULL;
                   is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
               } else {
                   is->audio_buf_size = audio_size;
               }
               is->audio_buf_index = 0;
            }
            len1 = is->audio_buf_size - is->audio_buf_index;
            if (len1 > len)
                len1 = len;
            if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
                memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
            else {
                memset(stream, 0, len1);
                if (!is->muted && is->audio_buf)
                    SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
            }
            len -= len1;
            stream += len1;
            is->audio_buf_index += len1;
        }
        is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
        /* Let's assume the audio driver that is used by SDL has two periods. */
        if (!isnan(is->audio_clock)) {
            is->audclk.setAt(is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, ar->audio_callback_time / 1000000.0);
            is->extclk.syncToSlave(&is->audclk);
        }
}

//bool BugAV::AudioRender::audioCallback(uint8_t * stream, int len)
//{

//    int audioSize, len1;
//    auto audio_callback_time = av_gettime_relative();
//    while (len > 0) {
//        if (is->audio_buf_index >= int(is->audio_buf_size)) {
//            audioSize = audioDecodeFrame();
//            if (audioSize < 0) {
//                is->audio_buf = nullptr;
//                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
//            } else {
//                is->audio_buf_size = audioSize;
//            }
//            is->audio_buf_index = 0;
//        }
//        len1 = is->audio_buf_size - is->audio_buf_index;
//        if (len1 > len) {
//            len1 = len;
//        }
//        if (!is->muted && is->audio_buf && is->audio_volume == MIX_MAX_VOLUME) {
//            // memcp
////            stream->replace(0, len1, (const char *)(is->audio_buf + is->audio_buf_index));
////            stream->replace(0, len1, (unsigned char *)(is->audio_buf + is->audio_buf_index));
////            auto d = stream->data();
//            memcpy(stream, is->audio_buf + is->audio_buf_index, len1);
////            stream->setRawData(d);
//        } else {
//            // memset
////            stream->fill(0, len1);
//             memset(stream, 0, len1);
//            if (!is->muted && is->audio_buf) {
//                memcpy(stream, is->audio_buf + is->audio_buf_index, len1);
//                // SDL_MixAudioFormat
//            }
//        }
//        len -= len1;
//        stream += len1;
//        is->audio_buf_index += len1;
//    }
//    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
//    if (!qIsNaN(is->audio_clock)) {
//        auto clockVal = is->audio_clock -
//                double(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / double(is->audio_tgt.bytes_per_sec);
//        is->audclk.setAt(clockVal, is->audio_clock_serial, audio_callback_time / 1000000.0);
//        is->extclk.syncToSlave(&is->audclk);
//    }
//     return true;
//}

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
       if (reqStop) {
           return -1;
       }
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
//    auto audio_clock0 = is->audio_clock;
//    /* update the audio clock with the pts */
//    if (!qIsNaN(af->getPts())) {
//        is->audio_clock = af->getPts() + double(af->frame->nb_samples) / double(af->frame->sample_rate);
//    } else {
//        is->audio_clock = NAN;
//    }
    is->audio_clock_serial = af->serial;
//    if (is->debug) {
//        static double last_clock;
//        qDebug("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
//               is->audio_clock - last_clock,
//               is->audio_clock, audio_clock0);
//        last_clock = is->audio_clock;
//    }    
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

//void BugAV::AudioRender::process()
//{
//    qDebug() << "Audio render start";
//    firstPktAudioPts = 0.0;
//    SDL_PauseAudioDevice(audio_dev, 0);
//    while (!reqStop && !hasInitAudioParam && !is->noAudioStFound) {
//        qDebug() << "Audio thread Wait init audio param";
//        thread->msleep(100);
//    }
//    if (reqStop || is->noAudioStFound) {
//        qDebug() << "Audio render stop becase no audio stream found";
//        thread->terminate();
//        emit stopped();
//        return;
//    }
//    do {
//        thread->msleep(100);
//    } while (!reqStop);
//    auto backend = new AudioOpenALBackEnd();
//    backend->setAudioParam(is->audio_tgt);
//    backend->open();
//    playInitialData(backend);
//    while (!reqStop) {
//        if (is->audio_st != nullptr && is->sampq->queueNbRemain() > 0) {
//            break;
//        }
//        thread->msleep(1);

//    }
//    int writeResult;
//    while (!reqStop) {
//        msSleep = 1;
//        auto audioSize = audioDecodeFrame();
//        if (audioSize < 0) {
//            thread->msleep(msSleep);
//            continue;
//        }
////        auto r = audioCallback(buffer, sizeBuffer);
////        if (!r) {
////            thread->msleep(msSleep);
////            continue;
////        }
////        backend->write(QByteArray::fromRawData(is->audio_buf, AUDIO_BUFF_SIZE));
//        writeResult = backend->write(is->audio_buf, audioSize);
//        backend->play();
//        thread->msleep(msSleep);
//        if (writeResult > 0) {
//            do {
//                msSleep = 1;
//                writeResult = backend->write(is->audio_buf, audioSize);
//                thread->usleep(msSleep);
//            } while (writeResult == 0 && !reqStop);
//        }
//    }
//    qDebug() << "Audio render stop";
//    delete backend;
//    thread->terminate();
//    emit stopped();
//}

//void BugAV::AudioRender::audioNotify()
//{
//    audioCallback(byteBuffer);
//}

//void BugAV::AudioRender::hanlerAudioFormatDone()
//{
//    start();
//}

//void BugAV::AudioRender::handleStateChanged(QAudio::State audioState)
//{
//     qDebug() << "Audio state change " << audioState;
//    if (audioState == QAudio::State::IdleState) {
//        qDebug() << "AudioRender callback";
////        audioCallback(byteBuffer);
//    }
//}

//void BugAV::AudioRender::playInitialData(AudioOpenALBackEnd *backend)
//{
//    auto format = AVSampleFormat(is->audio_tgt.fmt);
//    const char c = (format == AVSampleFormat::AV_SAMPLE_FMT_U8
//                        || format == AVSampleFormat::AV_SAMPLE_FMT_U8P)
//                ? 0x80 : 0;
//    for (quint32 i = 0; i < NUM_BUFFERS_OPENAL; ++i) {
//            uint8_t x[sizeBuffer];
//            memset(x, c, sizeBuffer);
//            backend->write(x, sizeBuffer); // fill silence byte, not always 0. AudioFormat.silenceByte
//        }
//    backend->play();
//}

//qint64 BugAV::AudioRender::audioLength(const AudioParams &audioParam, qint64 microSeconds)
//{
//    qint64 result = (audioParam.freq * audioParam.channels * (16 / 8))
//        * microSeconds / 100000;
//    result -= result % (audioParam.channels * 16);
//    return result;
//}

//void BugAV::AudioRender::timerEvent(QTimerEvent *)
//{
//}
