#ifndef IRENDERER_H
#define IRENDERER_H
#include "marco.h"

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

};

}
#endif // IRENDERER_H
