#include "handlerinterupt.h"
#include <QDebug>
HandlerInterupt::HandlerInterupt(Demuxer *demuxer, qint64 timeout)
    :status{0}
    ,timeout{timeout}
    ,demuxer{demuxer}
    ,action{Action::Unknown}
    ,emitError{true}
    ,timeoutAbort{true}
{
    callback = handleTimeout;
    opaque = this;
}

HandlerInterupt::~HandlerInterupt()
{
    timer.invalidate();
}

void HandlerInterupt::begin(HandlerInterupt::Action action)
{
    if (status > 0) {
        status = 0;
    }
    emitError = true;
    this->action = action;
    timer.start();
}

void HandlerInterupt::end()
{
    timer.invalidate();
    action = Action::Unknown;
}

int HandlerInterupt::handleTimeout(void *opaque)
{
//    return 0;
    HandlerInterupt* handler = static_cast<HandlerInterupt*>(opaque);
    if (handler == nullptr) {
        qWarning() << "[Interupt] handler is null";
        return -1;
    }

    if (handler->status < 0) {
        qDebug("[Interupt] Status < 0. Abort");
        return 1;
    }
    if (handler->timeout < 0) {
        return 0;
    }
    if (!handler->timer.isValid()) {
        //qDebug("timer is not valid, start it");
        handler->timer.start();
        return 0;
    }
    if (!handler->timer.hasExpired(handler->timeout)) {
        return 0;
    } else {
        // time out        
        handler->timer.invalidate();
        if (handler->status == 0) {
//            Action::ErrorCode ec(AVError::ReadTimedout);
//            if (handler->action == Open) {
//                ec = AVError::OpenTimedout;
//            } else if (handler->mAction == FindStreamInfo) {
//                ec = AVError::ParseStreamTimedOut;
//            } else if (handler->mAction == Read) {
//                ec = AVError::ReadTimedout;
//            }
            handler->status = int(handler->action);
            // maybe changed in other threads
            //handler->mStatus.testAndSetAcquire(0, ec);
        }
        if (handler->timeoutAbort) {
            qDebug("[Interupt] timeout abort");
            return 1;
        }
        qDebug("[Interupt] status: %d, Timeout expired: %lld/%lld -> no abort!", (int)handler->status, handler->timer.elapsed(), handler->timeout);
        return 0;
    }
}

int HandlerInterupt::getStatus() const
{
    return status;
}

void HandlerInterupt::setStatus(int value)
{
    status = value;
}
