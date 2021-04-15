#ifndef DEMUXER_H
#define DEMUXER_H
extern "C" {
#include <libavformat/avformat.h>
}
#include <QMutex>
#include <QThread>
#include <QVariantHash>
#include <QWaitCondition>

class QElapsedTimer;

namespace BugAV {

class PacketQueue;
class Clock;
class StreamInfo;
class VideoState;
class HandlerInterupt;
class Define;
class IAudioBackEnd;

class Demuxer: public QObject
{
    Q_OBJECT
public:
    Demuxer();
    Demuxer(VideoState *is, Define *def);
    ~Demuxer();

    void setAudioRender(IAudioBackEnd *audioRender);

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

    qint64 getStartTime() const;
    void setStartTime(const qint64 &value);

    void doSeek(const double& position);

    bool getSkipNonKeyFrame() const;
    void enableSkipNonKeyFrame(bool value = true);

    void reOpenAudioSt();

signals:
    void started();
    void stopped();
    void loadDone();
    void loadFailed();
    void readFrameError();

//    void positionChanged(qint64 time);

    void seekFinished(qint64 position);
private:    
    static int findBestStream(AVFormatContext *ic, AVMediaType type, int index);
    static int streamHasEnoughPackets(AVStream *st, int streamID, PacketQueue *queue);

    void freeAvctx();
    void parseAvFormatOpts();
    int streamOpenCompnent(int streamIndex);
    void seek();
    int doQueueAttachReq();
    void stepToNextFrame();
//    void streamTogglePause();
    void streamSeek(int64_t pos, int64_t rel, int seek_by_bytes);

    void reFindStream(AVFormatContext *ic, AVMediaType type, int index);

private slots:
    void process();

//    void onUpdatePositionChanged();
private:
    VideoState *is;
    Define *def;

    QThread *curThread;
    QMutex mutex;
    QWaitCondition cond;

    AVDictionary *formatOpts;
    QVariantHash avformat;
    qint64 startTime; // using for seek begin
    int stIndex[AVMEDIA_TYPE_NB];
    int infinityBuff = -1;
    int loop;

    bool isRun;
    int seek_by_bytes = -1;

    bool reqStop;

    AVPacket pkt1, *pkt;
    HandlerInterupt *handlerInterupt;
    friend HandlerInterupt;

//    AVCodecContext *avctx;

    IAudioBackEnd *audioRender;

    bool skipNonKeyFrame;

    QElapsedTimer *elLastRetryOpenAudioSt;
    bool denyRetryOpenAudioSt;

//    QElapsedTimer *elLastEmptyRead;
//    QElapsedTimer *elTimer;
//    bool isAllowUpdatePosition;
//    qint64 currentPos;
//    qint64 lastUpdatePos;
};
}
#endif // DEMUXER_H

