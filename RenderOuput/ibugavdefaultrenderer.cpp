#include "ibugavdefaultrenderer.h"

#include <QDebug>
namespace BugAV {

IBugAVDefaultRenderer::IBugAVDefaultRenderer(QWidget *parent)
    :QWidget(parent)
    ,IBugAVRenderer()
{

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

}
