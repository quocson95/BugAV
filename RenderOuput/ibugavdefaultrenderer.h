#ifndef IBUGAVDEFAULTRENDERER_H
#define IBUGAVDEFAULTRENDERER_H

#include "IBugAVRenderer.h"

namespace BugAV {
class IBugAVDefaultRenderer: public IBugAVRenderer
{
public:
    IBugAVDefaultRenderer();
    ~IBugAVDefaultRenderer() override;

    void updateData(AVFrame *frame) override;

    void setRegionOfInterest(int x, int y, int w, int h) override;

    QSize videoFrameSize() const override;
};
}
#endif // IBUGAVDEFAULTRENDERER_H
