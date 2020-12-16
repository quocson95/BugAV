#include "ibugavdefaultrenderer.h"

#include <QDebug>
#include <QWindow>

namespace BugAV {

IBugAVDefaultRenderer::IBugAVDefaultRenderer(QWidget *parent)
    :QFrame(parent)
    ,IBugAVRenderer()
{
//    setAttribute(Qt::WA_NativeWindow);
//        setAttribute(Qt::WA_NoSystemBackground);
}

IBugAVDefaultRenderer::~IBugAVDefaultRenderer()
{

}

void IBugAVDefaultRenderer::updateData(AVFrame *frame)
{
    Q_UNUSED(frame)
//    qDebug() << "default render";
}

void IBugAVDefaultRenderer::setRegionOfInterest(int x, int y, int w, int h)
{
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(w)
    Q_UNUSED(h)
}

QSize IBugAVDefaultRenderer::videoFrameSize() const
{
    return QSize(0, 0);
}

void IBugAVDefaultRenderer::setQuality(int quality)
{
    Q_UNUSED(quality)
}

void IBugAVDefaultRenderer::setOutAspectRatioMode(int ratioMode)
{
    Q_UNUSED(ratioMode)
}

QImage IBugAVDefaultRenderer::receiveFrame(const QImage &frame)
{
    return frame;
}

void IBugAVDefaultRenderer::clearBufferRender()
{

}

void IBugAVDefaultRenderer::registerWinIDChangedCB(const IBugAVRenderer::CallbackWinIDChanged &cbFunc, QString id, void *opaque)
{
    this->winIDChangedCB = cbFunc;
    this->opaque = opaque;
    this->id = id;
    if (this->winIDChangedCB != nullptr) {
        unsigned int wnID = winId();
        this->winIDChangedCB(wnID, id, opaque);
    }
//    showEvent(nullptr);
}

//void IBugAVDefaultRenderer::showEvent(QShowEvent *event)
//{
//    QWidget::winId();
//    if (this->winIDChangedCB != nullptr && windowHandle() != nullptr) {
//       unsigned int wnID = windowHandle()->winId();
//       this->winIDChangedCB(wnID, id, opaque);
//    }
//}

//bool IBugAVDefaultRenderer::event(QEvent *event)
//{
//    auto t = event->type();
//    if (windowTitle() != nullptr) {
//        qDebug() << windowHandle()->winId();
//    }
//    if (event->type() == QEvent::WinIdChange) {
//        if (this->winIDChangedCB != nullptr) {
//            unsigned int wnID = windowHandle()->winId();
//            this->winIDChangedCB(wnID, id, opaque);
//        }
//    }
//    return QWidget::event(event);
//}

}
