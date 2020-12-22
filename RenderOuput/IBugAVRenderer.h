#ifndef IRENDERER_H
#define IRENDERER_H
#include "marco.h"

#include <QImage>
#include <QSize>

struct AVFrame;

struct Frame {
    unsigned char* data[3];
    int linesize[3];
    int height;
    int sizeInBytes;
    int cap;
    int type;

    Frame() {
        for (auto i = 0; i < 3; i++) {
           this->linesize[i] = 0;
        }
        this->height = 0;
        this->sizeInBytes = 0;
        this->cap = 0;
        this->data[0] = nullptr;
        this->data[1] = nullptr;
        this->data[2] = nullptr;

        this->type = 0;
    }

    void resize(int size) {
        if (cap <= size) {
            this->cap = size;
            this->data[0] = (unsigned char *)realloc(this->data[0], this->cap);
        }
    }

    Frame clone() const {
        Frame f;
        f.cap = cap;
        f.height = height;
        f.sizeInBytes = sizeInBytes;
        for (auto i = 0; i < 3; i++) {
            f.linesize[i] = linesize[i];
        }        
        f.data[0] = static_cast<unsigned char *>(malloc(cap));
        memcpy(f.data[0], this->data[0], sizeInBytes);
        f.type = this->type;
        return f;
    }

    void mallocData(int size = 0) {
        freeMem();
        if (size == 0) {
            size = linesize[0] * height * 4;
        }
        cap = size;
        data[0] = static_cast<unsigned char *>(malloc(cap));
    }

    bool isNull() const {
        return this->data[0] == nullptr || linesize[0] < 1 || height < 1;
    }

    void freeMem() {
        if (this->data[0] != nullptr) {
            free(this->data[0]);
        }
    }
} ;


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
