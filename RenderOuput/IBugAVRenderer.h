#ifndef IRENDERER_H
#define IRENDERER_H
#include "marco.h"

#include <QImage>
#include <QSize>

struct AVFrame;

namespace BugAV {

class IBugAVRenderer  {
public:
//    IBugAVRenderer() = default;
    virtual ~IBugAVRenderer() = default;
    virtual void updateData(AVFrame *frame) = 0;    

    virtual void setRegionOfInterest(int x, int y, int w, int h) = 0;
    virtual QSize videoFrameSize() const = 0;
    virtual void setQuality(int quality) = 0;
    virtual void setOutAspectRatioMode(int ratioMode) = 0;

    virtual QImage receiveFrame(const QImage& frame) = 0;

    virtual void clearBufferRender() = 0;

    virtual void newImageBuffer(const QImage& img) {
        Q_UNUSED(img)
    };

    virtual void updateImageBuffer(const QImage& img) {
        Q_UNUSED(img)
    }

};

}
#endif // IRENDERER_H
