#include "ibugavdefaultrenderer.h"

#include <QDebug>
namespace BugAV {

IBugAVDefaultRenderer::IBugAVDefaultRenderer()
    :IBugAVRenderer()
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

}
