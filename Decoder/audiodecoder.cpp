#include "audiodecoder.h"
#include <QDebug>

BugAV::AudioDecoder::AudioDecoder(VideoState *is):
    thread{nullptr}
    ,is{is}
    ,frame{nullptr}
{

}

BugAV::AudioDecoder::~AudioDecoder()
{
    stop();
}

void BugAV::AudioDecoder::start()
{
    reqStop = false;
    if (thread == nullptr) {
        thread = new QThread{this};
        moveToThread(thread);
        connect(thread, &QThread::started, this, &AudioDecoder::process);
    }
    if (thread->isRunning()) {
        return;
    }
    thread->start();
}

void BugAV::AudioDecoder::stop()
{
    reqStop = true;
    if (thread == nullptr) {
        return;
    }
    thread->quit();
    do {
        thread->wait(500);
    } while(thread->isRunning());
}

bool BugAV::AudioDecoder::isRunning() const
{
    if (thread == nullptr) {
        return false;
    }
    return thread->isRunning();
}

//void BugAV::AudioDecoder::audioCallback(quint8 *stream, int len)
//{
//    int audioSize, len1;
//    auto audio_callback_time = av_gettime_relative();
//    while (len > 0) {
//        if (is->audio_buf_index > int(is->audio_buf_size)) {
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
//        } else {
//            // memset
//            if (!is->muted && is->audio_buf) {
//                // SDL_MixAudioFormat
//            }
//            len -= len1;
//            stream += len1;
//            is->audio_buf_index += len1;
//        }
//        is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
//        if (!qIsNaN(is->audio_clock)) {
//            auto clockVal = is->audio_clock -
//                    double(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / double(is->audio_tgt.bytes_per_sec);
//            is->audclk.setAt(clockVal, is->audio_clock_serial, audio_callback_time / 1000000.0);
//            is->extclk.syncToSlave(&is->audclk);
//        }
//    }
//}

int BugAV::AudioDecoder::getAudioFrame(AVFrame *frame)
{
    if (reqStop) {
        return  -1;
    }
    auto got_frame = is->auddec.decodeFrame(frame);
    return got_frame;
}

//int BugAV::AudioDecoder::audioDecodeFrame()
//{
//    int data_size, resampled_data_size;
//   int64_t dec_channel_layout;
//   int wanted_nb_samples;
//   Frame *af;
//   if (is->paused) {
//       return  -1;
//   }
//   do {
//#if defined(_WIN32)
//        while (frame_queue_nb_remaining(&is->sampq) == 0) {
//            if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
//                return -1;
////            av_usleep (1000);
//            return -100;
//        }
//#endif
//        if (!(af = is->sampq->peekReadable())) {
//            return -1;
//        }
//        is->sampq->queueNext();
//   } while (af->serial != is->audioq->serial);
//   auto avFrame = af->frame;
//   data_size = av_samples_get_buffer_size(nullptr, avFrame->channels,
//                                          avFrame->nb_samples,
//                                          AVSampleFormat(avFrame->format),
//                                          1);
//   dec_channel_layout =
//           (avFrame->channel_layout && avFrame->channels == av_get_channel_layout_nb_channels(avFrame->channel_layout)) ?
//               avFrame->channel_layout : av_get_default_channel_layout(avFrame->channels);
//   // sync audio
////   wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);
//   if (avFrame->format          != is->audio_src.fmt                ||
//           dec_channel_layout   != is->audio_src.channel_layout     ||
//           avFrame->sample_rate != is->audio_src.freq               ||
//           (wanted_nb_samples   != avFrame->nb_samples && !is->swr_ctx)) {
//       swr_free(&is->swr_ctx);
//       is->swr_ctx = swr_alloc_set_opts(nullptr,
//                                        is->audio_tgt.channel_layout, AVSampleFormat(is->audio_tgt.fmt), is->audio_tgt.freq,
//                                        dec_channel_layout, AVSampleFormat(avFrame->format), avFrame->sample_rate,
//                                        0, nullptr);
//       if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
//           av_log(NULL, AV_LOG_ERROR,
//                  "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
//                   af->frame->sample_rate, av_get_sample_fmt_name(AVSampleFormat(avFrame->format)), af->frame->channels,
//                   is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
//           swr_free(&is->swr_ctx);
//           return -1;
//       }
//       is->audio_src.channel_layout = dec_channel_layout;
//       is->audio_src.channels = avFrame->channels;
//       is->audio_src.freq = avFrame->sample_rate;
//       is->audio_src.fmt = AVSampleFormat(avFrame->format);
//   }
//    if (is->swr_ctx) {
//        const auto& in = (const quint8 **)(af->frame->extended_data);
//        auto out = &is->audio_buf1;
//        int out_count = qint64(wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256);
//        int out_size = av_samples_get_buffer_size(nullptr, is->audio_tgt.channels, out_count,
//                                                 AVSampleFormat(is->audio_tgt.fmt), 0);
//        if (out_size < 0) {
//           av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
//            return -1;
//        }
//        if (wanted_nb_samples != af->frame->nb_samples) {
//           if (swr_set_compensation(is->swr_ctx,
//                                    (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
//                                    wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0 ) {
//               av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
//                               return -1;
//           }
//        }
//        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
//        if (!is->audio_buf1) {
//           av_log(NULL, AV_LOG_ERROR, "av_fast_malloc(audio_buf1) failed\n");
//                           return -1;
//           return AVERROR(ENOMEM);
//        }
//        auto len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
//        if (len2 < 0) {
//            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
//            return -1;
//        }
//        if (len2 == out_count) {
//            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
//            if (swr_init(is->swr_ctx) < 0) {
//                swr_free(&is->swr_ctx);
//            }
//        }
//        is->audio_buf = is->audio_buf1;
//        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(AVSampleFormat(is->audio_tgt.fmt));
//    } else {
//        is->audio_buf = af->frame->data[0];
//        resampled_data_size = data_size;
//    }
//    auto audio_clock0 = is->audio_clock;
//    /* update the audio clock with the pts */
//    if (!qIsNaN(af->getPts())) {
//        is->audio_clock = af->getPts() + double(af->frame->nb_samples) / double(af->frame->sample_rate);
//    } else {
//        is->audio_clock = NAN;
//    }
//    is->audio_clock_serial = af->serial;
//    if (is->debug) {
//        static double last_clock;
//        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
//               is->audio_clock - last_clock,
//               is->audio_clock, audio_clock0);
//        last_clock = is->audio_clock;
//    }
//    return  resampled_data_size;
//}

//int BugAV::AudioDecoder::synchronizeAudio(int nb_samples)
//{
//    auto wanted_nb_samples = nb_samples;
//    if (is->isAudioClock()) {
//        auto diff = is->audclk.get() - is->getMasterClock();
//        if (!qIsNaN(diff) && qFabs(diff) < AV_NOSYNC_THRESHOLD) {
//            is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
//            if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
//                /* not enough measures to have a correct estimate */
//                is->audio_diff_avg_count++;
//            } else {
//                /* estimate the A-V difference */
//                auto avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

//                if (qFabs(avg_diff) >- is->audio_diff_threshold) {
//                    wanted_nb_samples = nb_samples + int(diff * double(is->audio_src.freq));
//                    auto min_min_nb_samples = nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
//                    auto max_nb_samples = nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
//                    wanted_nb_samples = av_clip(wanted_nb_samples, min_min_nb_samples, max_nb_samples);
//                }
//                if (is->debug) {
//                    av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
//                                            diff, avg_diff, wanted_nb_samples - nb_samples,
//                                            is->audio_clock, is->audio_diff_threshold);
//                }
//            }
//        } else {
//            /* too big difference : may be initial PTS errors, so
//                           reset A-V filter */
//            is->audio_diff_avg_count = 0;
//            is->audio_diff_cum = 0;
//            if (is->debug) {
//                av_log(NULL, AV_LOG_TRACE, "Audio too big difference : may be initial PTS errors, so reset A-V filter ");
//            }
//        }
//    }
//    return wanted_nb_samples;
//}

void BugAV::AudioDecoder::process()
{
    emit started();
    qDebug() << "AudioDecoder started";
    if (frame == nullptr) {
        frame = av_frame_alloc();
    }
    if (!frame) {
        // error when alloc mem,
        // todo fix here
        emit stopped();
        thread->quit();
        qDebug() << "AudioDecoder can't alloc frame";
        return;
    }
    while ((is->audioq->size == 0 || is->audio_st == nullptr) && !is->noAudioStFound) {
//        qDebug() << "wait video stream input";
        if (reqStop) {
            av_frame_free(&frame);
            frame = nullptr;
            break;
        }
        thread->msleep(100);
    }
    if (is->noAudioStFound) {
        av_frame_free(&frame);
        emit stopped();
        qDebug() << "!!!AudioDecoder Thread exit becase no audio stream found";
        thread->terminate();
    }
    forever {
        if (reqStop) {
            break;
        }
        auto got_frame = getAudioFrame(frame);
        if (got_frame < 0) {
            qDebug() << "get audio frame error";
            break;
            // break; // todo handle this, continue or exit?
        }
//        qDebug() << "AudioDecoder get audio frame";
        if (got_frame) {
            tb = AVRational{1, frame->sample_rate};
        }
        Frame *f;
        if (!(f = is->sampq->peekWriteable())) {
            qDebug() << "peekWriteable audio queue error";
            break;
        }
        auto pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        f->setPts(pts);
        f->pos = frame->pkt_pos;
        f->serial = is->auddec.getPkt_serial();
        auto av = AVRational{frame->nb_samples, frame->sample_rate};
        f->duration = av_q2d(av);
        av_frame_move_ref(f->frame, frame);
        is->sampq->queuePush();
    }
    is->audioq->flush();
    av_frame_unref(frame);
    av_frame_free(&frame);
    emit stopped();
    qDebug() << "!!!AudioDecoder Thread exit";
    thread->terminate();

}
