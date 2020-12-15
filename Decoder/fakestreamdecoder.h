#ifndef FAKESTREAMDECODER_H
#define FAKESTREAMDECODER_H

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
    struct FIQ{
        QByteArray path;
        int status; // 0 init, 1 writing, 3 write done, 4 read done
    };
    std::queue<FIQ *> fileQueue;
    int indexForFile;

    LONG g_lPort;
    QWidget *www;

private:
     void readFile(FIQ *fiq);
};
}

#endif // FAKESTREAMDECODER_H
