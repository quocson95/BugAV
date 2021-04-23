#include "audiooutputportaudio.h"
#include <QThread>
#include <QtMath>
#include "QDebug"
extern "C" {
#include "libavutil/time.h"
}

#include "common/videostate.h"
#include "portaudiobackend.h"

namespace BugAV {
AudioOutputPortAudio::AudioOutputPortAudio(VideoState *is)
    : QObject(nullptr)
    ,is{is}
//    ,outputParameters{nullptr}
//    ,stream{nullptr}
    ,curThread{nullptr}
    ,reqStop{false}
//    ,buffer{nullptr}
{       
    curThread = new QThread{this};
    moveToThread(curThread);
    connect(curThread, &QThread::started, this, &AudioOutputPortAudio::process);
}

AudioOutputPortAudio::~AudioOutputPortAudio()
{
    AudioOutputPortAudio::close();
    delete curThread;
}

int AudioOutputPortAudio::open(int wanted_channel_layout,
                                int wanted_nb_channels,
                                int wanted_sample_rate, AudioParams *audio_hw_params)
{
//    init();
    close();
    reqStop = false;
    if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);

    if (audio_hw_params != nullptr) {
        audio_hw_params->fmt = AV_SAMPLE_FMT_S16;;
        audio_hw_params->freq = wanted_sample_rate;
        audio_hw_params->channel_layout = wanted_channel_layout;
        audio_hw_params->channels = wanted_nb_channels;
        audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
        audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
        if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
            return -1;
        }
        qDebug() << " Open audio channel: " << wanted_nb_channels << " samples " << wanted_sample_rate;
    }
    if (is->muted == false && audio_hw_params != nullptr) {
        PortAudioBackendDefault->close();
        PortAudioBackendDefault->open(*audio_hw_params);
    }
    curThread->start();
    auto size = 0;
    if (audio_hw_params != nullptr) {
        size = audio_hw_params->bytes_per_sec / 8 ;
    }
    return size;
}

int AudioOutputPortAudio::close()
{
    this->reqStop = true;
    if (this->curThread) {
        if (curThread->isRunning()) {
            curThread->quit();
            curThread->wait(500);
            this->curThread->terminate();
            curThread->wait();
        }
    }       
    return true;
}


void AudioOutputPortAudio::process()
{
    while (!reqStop) {
        if (!PortAudioBackendDefault->isInit()) {
            curThread->msleep(100);
            continue;
        }
//        if (Pa_IsStreamActive(stream) ) {
        renderAudio();
//        if (!qIsNaN(is->audio_clock)) {
//            if (is->audio_tgt.bytes_per_sec >  0) {
//                is->audclk.setAt(is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, audio_callback_time / 1000000.0);
//            }
//            is->extclk.syncToSlave(&is->audclk);
//        }
    }
    curThread->terminate();
}

void AudioOutputPortAudio::renderAudio()
{
    int audio_size = 0;
    audio_callback_time = av_gettime_relative();
//    int len = 512; // kb
//    while (len > 0) {
    if (reqStop) {
        return;
    }
//    if (static_cast<unsigned int>(is->audio_buf_index) >= is->audio_buf_size) {
       audio_size = audioDecodeFrame();
       if (audio_size <= 0) {

           if (reqStop) {
               return;
           }
           curThread->msleep(1);
            /* if error, just output silence */
           if (is->audio_tgt.frame_size <= 0) {
               return;
           }
           is->audio_buf = NULL;
           is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
       } else {
           is->audio_buf_size = audio_size;
       }
       is->audio_buf_index = 0;
//    }
//        auto len1 = is->audio_buf_size - is->audio_buf_index;
//        if (len1 > len)
//            len1 = len;
       if (!is->muted) {
           PortAudioBackendDefault->write(is->audio_buf, audio_size);
       }

//        if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
//            memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
//        else {
//            memset(stream, 0, len1);
//            if (!is->muted && is->audio_buf)
//                SDL_MixAudioFormat(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
//        }
//        len -= len1;
//        stream += len1;
//        is->audio_buf_index += len1;
//    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!qIsNaN(is->audio_clock)) {
        if (is->audio_tgt.bytes_per_sec >  0) {
            is->audclk.setAt(is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, audio_callback_time / 1000000.0);
        }
        is->extclk.syncToSlave(&is->audclk);
    }
}

int AudioOutputPortAudio::audioDecodeFrame()
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

        while (is->sampq->queueNbRemain() == 0) {
            if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2){
                return -1;
            }
            if (reqStop) {
                return -1;
            }
            curThread->usleep(1000);
        }

        af = is->sampq->peekReadable();
        if (af == nullptr) {
            return -1;
        }

        is->sampq->queueNext();
   } while (af->serial != is->audioq->serial);

   auto avFrame = af->frame;
   if (af->frame->sample_rate <= 0) {
       return -1;
   }

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
           auto errStr = QString( "Cannot create sample rate converter for conversion of %1 Hz %2 %3 channels to %4 Hz %5 %6 channels!\n")
                   .arg(af->frame->sample_rate).arg(av_get_sample_fmt_name(AVSampleFormat(avFrame->format)))
                   .arg(af->frame->channels).arg(is->audio_tgt.freq)
                   .arg(av_get_sample_fmt_name(is->audio_tgt.fmt))
                   .arg(is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
           qDebug() << errStr;
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
        int out_count = int64_t(wanted_nb_samples) * is->audio_tgt.freq / af->frame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(nullptr, is->audio_tgt.channels, out_count,
                                                 AVSampleFormat(is->audio_tgt.fmt), 0);
        if (out_size < 0) {
            qDebug() << "av_samples_get_buffer_size() failed";
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
           if (swr_set_compensation(is->swr_ctx,
                                    (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                                    wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0 ) {
                qDebug() << "swr_set_compensation() failed";
                return -1;
           }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
           qDebug() << "av_fast_malloc(audio_buf1) failed";
           return AVERROR(ENOMEM);
        }
        auto len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            qDebug() << "swr_convert() failed";
            return -1;
        }
        if (len2 == out_count) {
             qDebug() << "audio buffer is probably too small";
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
    if (!qIsNaN(af->getPts())) {
            is->audio_clock = af->getPts() + double(af->frame->nb_samples) / double(af->frame->sample_rate);
    } else {
        is->audio_clock = NAN;
    }
    is->audio_clock_serial = af->serial;

//    static double last_clock;
//    auto str = QString("audio: delay=%1 clock=%2 clock0=%3")
//            .arg(is->audio_clock - last_clock)
//            .arg(is->audio_clock)
//            .arg( audio_clock0);
//    qDebug() << str;
//    last_clock = is->audio_clock;
    return  resampled_data_size;
}

int AudioOutputPortAudio::synchronizeAudio(int nb_samples)
{
    if (is->isAudioClock()) {
        return 0;
    }
    auto wanted_nb_samples = nb_samples;
//    return wanted_nb_samples;
    auto diff = is->audclk.get() - is->getMasterClock();
    if (!qIsNaN(diff) && qFabs(diff) < AV_NOSYNC_THRESHOLD) {
        is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
        if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
            /* not enough measures to have a correct estimate */
            is->audio_diff_avg_count++;
        } else {
            /* estimate the A-V difference */
            auto avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

            if (qFabs(avg_diff) >= is->audio_diff_threshold) {
                wanted_nb_samples = nb_samples + int(diff * double(is->audio_src.freq));
                auto min_nb_samples = nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                auto max_nb_samples = nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
            }
            if (is->debug) {
//                auto errStr = QString("diff=%1 avgdiff=%2 sample_diff=%3 apts=%4 %5")
//                        .arg(diff).arg(avg_diff)
//                        .arg(wanted_nb_samples - nb_samples)
//                        .arg(is->audio_clock)
//                        .arg(is->audio_diff_threshold);
//                qDebug() << errStr;
//                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
//                                        diff, avg_diff, wanted_nb_samples - nb_samples,
//                                        is->audio_clock, is->audio_diff_threshold);
            }
        }
    } else {
        /* too big difference : may be initial PTS errors, so
                       reset A-V filter */
        is->audio_diff_avg_count = 0;
        is->audio_diff_cum = 0;
        if (is->debug) {
            qDebug() << "Audio too big difference : may be initial PTS errors, so reset A-V filter ";
        }
    }
    return wanted_nb_samples;
}

}
