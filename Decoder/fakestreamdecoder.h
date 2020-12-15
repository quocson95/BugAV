#ifndef FAKESTREAMDECODER_H
#define FAKESTREAMDECODER_H

#include <QElapsedTimer>
#include <QObject>
#include "queue"
#include "common/videostate.h"


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

    int openFileOutput();

    void newSegment();
    int writeSegment();
    void endSegment();
    void freeOutput();

    void freeGOP();

    void processPacket(AVPacket *pkt);


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

    FIQ *currentFIQWrite;

    std::queue<FIQ *> fileQueue;
    int indexForFile;

    LONG g_lPort;
    QWidget *www;

    QElapsedTimer elRecordDur;

private:
     void readFile(FIQ *fiq);
     void removeFile(FIQ *fiq);
};
}

#endif // FAKESTREAMDECODER_H
