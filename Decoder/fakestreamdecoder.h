#ifndef FAKESTREAMDECODER_H
#define FAKESTREAMDECODER_H

#include <QElapsedTimer>
#include <QObject>
#include "queue"
#include "memory"
#include "common/videostate.h"
#include "PlayM4.h"

namespace BugAV {

class FakeStreamDecoder: public QObject
{
    Q_OBJECT
public:
    FakeStreamDecoder(VideoState *is);
    ~FakeStreamDecoder();
    void start();
    void stop();

    void setWindowForHIKSDK(QWidget *w);

    void setWindowFishEyeForHIKSDK(QString id, QWidget *w);

    int openFileOutput();

    void newSegment();
    int writeSegment();
    void endSegment();
    void freeOutput();

    void freeGOP();

    void processPacket(AVPacket *pkt);

    void playM4DispCB();

    void winIDFECChanged(unsigned int wnID, QString id);

signals:
    void firstFrameComming();
    void noRenderNewFrameLongTime();

private slots:
    void process();

private:
    QThread *thread;

    VideoState *is;
    bool waitForKeyFrame;
    AVStream *stOutput;
    AVFormatContext *ctxOutput;
    QByteArray nameFileOuput;
    QByteArray path;
    std::vector<AVPacket *> gop;
    qint64 lastMuxDTSRecord;
    bool reqStop;

    enum class FileStatus {
        Init = 0,
        Writing = 1,
        WriteDone = 2,
        ReadDone = 3,
    };
    struct FIQ{
        QByteArray path;
        FileStatus status; // 0 init, 1 writing, 2 write done, 3 read done
    };

    struct FishEyeSubView {
        QWidget *w;
        unsigned port;
        FISHEYEPARAM param;
    };

    FIQ *currentFIQWrite;

    std::queue<FIQ *> fileQueue;
    int indexForFile;

    LONG g_lPort;
    unsigned int s_lPort;
    QWidget *mainWn;

    QElapsedTimer elRecordDur;

    std::unique_ptr<QElapsedTimer> elSinceFirstFrame;

    int kCheckFrameDisp;

    QMap<QString, FishEyeSubView *> fishEyeView;

private:
     void readFile(FIQ *fiq);
     void removeFile(FIQ *fiq);

     void killTimer(int *kId);
     void startTimer(int *kId, int interval);

     // QObject interface
protected:
     void timerEvent(QTimerEvent *event);
};
}

#endif // FAKESTREAMDECODER_H
