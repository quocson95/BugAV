#ifndef IBUGAVDEFAULTRENDERER_H
#define IBUGAVDEFAULTRENDERER_H

#include "IBugAVRenderer.h"

#include <QWidget>

namespace BugAV {
class IBugAVDefaultRenderer: public QWidget, public IBugAVRenderer
{
public:
    IBugAVDefaultRenderer(QWidget *parent = nullptr);
    ~IBugAVDefaultRenderer() override;

    void updateData(AVFrame *frame) override;

    void setRegionOfInterest(int x, int y, int w, int h) override;

    QSize videoFrameSize() const override;

    void setQuality(int quality) override;
    void setOutAspectRatioMode(int ratioMode) override;
};
}
#endif // IBUGAVDEFAULTRENDERER_H
