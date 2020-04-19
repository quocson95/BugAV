#ifndef DEMUXER_H
#define DEMUXER_H
extern "C" {
#include <libavformat/avformat.h>
}
#include <QMutex>
#include <QVariantHash>
#include <QWaitCondition>

class PacketQueue;
class Clock;
class StreamInfo;
class Demuxer: public QObject
{
    Q_OBJECT
public:
    Demuxer();
    ~Demuxer();

    void fakeInit();

    void setPktq(PacketQueue *value);
    void setAvformat(QVariantMap avformat);

    bool load();
    void unload();
    void readFrame();

    QString getFileUrl() const;
    void setFileUrl(const QString &value);

private:
    static bool isRealtime(const QString &fileUrl);
    static int findBestStream(AVFormatContext *ic, AVMediaType type, int index);
    static int streamHasEnoughPackets(AVStream *st, int streamID, PacketQueue *queue);
    int streamOpenCompnent(int streamIndex);
    void seek();
    int doQueueAttachReq();
    void stepToNextFrame();
    void streamTogglePause();
    void streamSeek(int64_t pos, int64_t rel, int seek_by_bytes);
private slots:
    void process();
private:
    QThread *thread;
    QMutex mutex;
    QWaitCondition cond;
    PacketQueue *videoq;
    AVDictionary *formatOpts;
    AVInputFormat *input_format;
    QVariantMap avformat;

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
    QWaitCondition continueReadThread; // must set to decoder
    AVPacket pkt1, *pkt;

};

#endif // DEMUXER_H
