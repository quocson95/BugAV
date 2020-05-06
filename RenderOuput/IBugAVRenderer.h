#ifndef IRENDERER_H
#define IRENDERER_H

#include <QSize>

struct AVFrame;

class QObject;

namespace BugAV {

class IBugAVRenderer  {

public:
//    IBugAVRenderer() = default;
    virtual ~IBugAVRenderer() = default;
    virtual void updateData(AVFrame *frame) = 0;
};

}
#endif // IRENDERER_H
