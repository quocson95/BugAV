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
    QImage receiveFrame(const QImage &frame) override;
    void clearBufferRender() override;

//static Config &ins() {
//    static GlobalCofig instance;
//    return  instance.cfg;
//}

};

#define BugAVRendererDefault BugAV::BugAVtRendererImpl::instance()
class BugAVtRendererImpl {
public:
    static IBugAVDefaultRenderer &instance() {
        static BugAVtRendererImpl ins;
        return ins.renderer;
    }

    BugAVtRendererImpl(const BugAVtRendererImpl &) = delete;
    void operator=(BugAVtRendererImpl const&) = delete;

private:
    BugAVtRendererImpl() {};
    IBugAVDefaultRenderer renderer;
};

}
#endif // IBUGAVDEFAULTRENDERER_H
