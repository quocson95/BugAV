#ifndef IRENDERER_H
#define IRENDERER_H
#include "marco.h"

#include <QImage>
#include <QSize>

struct AVFrame;

constexpr int RGB_FORMAT = 28;
constexpr int YUV_FORMAT = 0;

struct Frame {
    unsigned char* data[3];
    int linesize[3];
    int height;
    int sizeInBytes;
    int cap;
    // type
    // -1 or 28 is rgb32
    // 0 yuv
    int format;

    Frame();

    unsigned char * y() const;
    unsigned char * u() const;
    unsigned char * v() const;

    unsigned char * rgb();

    void setLinesize(int *linesize, int size = 1);

    void resize(int size);

    bool isRGB() const;

    bool isYUV() const;

    void resize(int *linesize, int h);

    Frame clone() const;

    void crop(Frame frame, int w, int h, int offsetX, int offsetY);

    void cropRGB(Frame frame, int w, int h, int offsetX, int offsetY);

    void cropYUV(Frame frame, int w, int h, int offsetX, int offsetY);

    void mallocData(int size = 0);
    void mallocData(int *linesize, int h);

    bool isNull() const;

    void freeMem();

    int getWidth() const {
        return linesize[0];
    }

    int getHeight() const {
        return height;
    }
} ;


namespace BugAV {

class IBugAVRenderer  {
public:
//    IBugAVRenderer() = default;
    virtual ~IBugAVRenderer() = default;
    virtual void updateData(AVFrame *frame) = 0;
    virtual void updateData(Frame *frame) = 0;

    virtual void setRegionOfInterest(int x, int y, int w, int h) = 0;
    virtual QSize videoFrameSize() const = 0;
    virtual void setQuality(int quality) = 0;
    virtual void setOutAspectRatioMode(int ratioMode) = 0;

    virtual QImage receiveFrame(const QImage& frame) = 0;

    virtual void clearBufferRender() = 0;

    virtual void newFrame(const QImage& img) {
        Q_UNUSED(img)
    };

    virtual void newFrame(const Frame& frame) {
        Q_UNUSED(frame)
    };

    virtual void updateFrameBuffer(const QImage& img) {
        Q_UNUSED(img)
    }

    virtual void updateFrameBuffer(const Frame& frame) {
        Q_UNUSED(frame)
    }



};

}
#endif // IRENDERER_H
