#ifndef IRENDERER_H
#define IRENDERER_H

#include <QSize>


class QObject;
class AVFrame;

namespace BugAV {

class IBugAVRenderer  {

public:
//    IBugAVRenderer() = default;
    virtual ~IBugAVRenderer() = default;
    virtual void updateData(AVFrame *frame);
};

}
#endif // IRENDERER_H
