#ifndef DEMUXER_H
#define DEMUXER_H
extern "C" {
#include <libavformat/avformat.h>
}
#include <QMutex>
#include <QThread>
#include <QVariantHash>
#include <QWaitCondition>

namespace BugAV {

class PacketQueue;
class Clock;
class StreamInfo;
class VideoState;
class HandlerInterupt;

class Demuxer: public QObject
{
    Q_OBJECT
public:
    Demuxer();
    Demuxer(VideoState *is);
    ~Demuxer();
    void setAvformat(QVariantHash avformat);

    bool load();
    void unload();
    int readFrame();

    QString getFileUrl() const;
    void setFileUrl(const QString &value);

    void start();
    void stop();

    void setIs(VideoState *value);

    bool isRunning() const;

signals:
    void started();
    void stopped();
    void loadDone();
    void loadFailed();
    void readFrameError();
private:    
    static int findBestStream(AVFormatContext *ic, AVMediaType type, int index);
    static int streamHasEnoughPackets(AVStream *st, int streamID, PacketQueue *queue);

    void parseAvFormatOpts();
    int streamOpenCompnent(int streamIndex);
    void seek();
    int doQueueAttachReq();
    void stepToNextFrame();
//    void streamTogglePause();
    void streamSeek(int64_t pos, int64_t rel, int seek_by_bytes);

private slots:
    void process();
private:
    VideoState *is;
    QThread *curThread;
    QMutex mutex;
    QWaitCondition cond;

    AVDictionary *formatOpts;
    QVariantHash avformat;
    int64_t startTime; // using for seek begin
    int stIndex[AVMEDIA_TYPE_NB];
    int infinityBuff = -1;
    int loop;

    bool isRun;
    int seek_by_bytes = -1;

    bool reqStop;

//    AVCodecContext *avctx;
    /*AVInputFormat *input_format;
    PacketQueue *videoq;
    AVFormatContext *ic;
    Clock *audclk;
    Clock *vidclk;
    Clock *extclk;

    QString fileUrl;
    int eof;
    int maxFrameDuration;
    int64_t startTime; // using for seek begin
    bool realtime;
    int stIndex[AVMEDIA_TYPE_NB];
    int infinityBuff;
    bool abortRequest;
    bool paused;
    bool lastPaused;
    int readPausedReturn;
    bool seekReq;
    int seekPos, seekRel, seekTarget;
    int seek_flags;
    int queueAttachmentsReq;
    int step;
    double frame_timer;
    int read_pause_return;

    StreamInfo *vStreamInfo;
    int loop;
    QWaitCondition continueReadThread; */// must set to decoder
    AVPacket pkt1, *pkt;
    HandlerInterupt *handlerInterupt;
    friend HandlerInterupt;
};
}
#endif // DEMUXER_H

