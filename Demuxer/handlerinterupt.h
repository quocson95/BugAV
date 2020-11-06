#ifndef HANDLERINTERUPT_H
#define HANDLERINTERUPT_H

extern "C" {
#include <libavformat/avio.h>
}
#include <QElapsedTimer>

namespace BugAV {

class Demuxer;

class HandlerInterupt: public AVIOInterruptCB
{
public:
    enum Action {
        Unknown = -1,
        Open,
        FindStreamInfo,
        Read
    };
    HandlerInterupt(Demuxer *demuxer, qint64 timeout = 10000);
    ~HandlerInterupt();

    void begin(Action action);
    void end();

    static int handleTimeout(void *opaque);
    int getStatus() const;
    void setStatus(int value);

    void setReqStop(bool value);

private:
    int status;
    qint64 timeout;
    QElapsedTimer timer;
    Demuxer *demuxer;
    Action action;
    bool emitError;
    bool timeoutAbort;

    bool reqStop;
};
}
#endif // HANDLERINTERUPT_H

