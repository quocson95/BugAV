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

    std::queue<QByteArray> fileQueue;
    int indexForFile;

};
}

#endif // FAKESTREAMDECODER_H
