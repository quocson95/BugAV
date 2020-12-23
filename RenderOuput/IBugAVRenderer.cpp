#include "IBugAVRenderer.h"

namespace BugAV {

}

Frame::Frame()
{
    for (auto i = 0; i < 3; i++) {
       this->linesize[i] = 0;
    }
    this->height = 0;
    this->sizeInBytes = 0;
    this->cap = 0;
    this->data[0] = nullptr;
    this->data[1] = nullptr;
    this->data[2] = nullptr;
    this->format = RGB_FORMAT;
}

unsigned char *Frame::y() const
{
    return data[0];
}

unsigned char *Frame::u() const
{
    return data[1];
}

unsigned char *Frame::v() const
{
    return data[2];
}

unsigned char *Frame::rgb()
{
    return data[0];
}

void Frame::setLinesize(int *linesize, int size)
{
    for(auto i = 0; i < size; i++) {
        this->linesize[i] = linesize[i];
    }
}

void Frame::resize(int size)
{
    if (cap <= size) {
        this->cap = size;
        this->data[0] = (unsigned char *)realloc(this->data[0], this->cap);
    }
}

bool Frame::isRGB() const
{
    return (format == -1 || format == RGB_FORMAT);
}

bool Frame::isYUV() const
{
    return (format == YUV_FORMAT);
}

void Frame::resize(int *linesize, int h)
{
    if (isRGB()) {
        auto size = linesize[0] * h * 4;
        resize(size);
        return;
    }
    if (linesize[0] != this->linesize[0] ||
            linesize[1] != this->linesize[1] ||
            linesize[2] != this->linesize[2] ||
            h != this->height)
    {
        setLinesize(linesize, 3);
        this->data[0] = (unsigned char *)realloc(this->data[0], linesize[0] * h);
        this->data[1] = (unsigned char *)realloc(this->data[1], linesize[1] / 2 * h);
        this->data[2] = (unsigned char *)realloc(this->data[2], linesize[2] / 2 * h);
        this->cap =  (linesize[0] + linesize[1] + linesize[2]) * h;
    }
}

Frame Frame::clone() const
{    
    Frame f;
    f.cap = cap;
    f.height = height;
    f.sizeInBytes = sizeInBytes;
    for (auto i = 0; i < 3; i++) {
        f.linesize[i] = linesize[i];
    }
    f.format = this->format;
    if (isYUV()) {
        f.data[0] = static_cast<unsigned char *>(malloc(this->linesize[0] * this->height));
        memcpy(f.data[0], this->data[0], this->cap);
        for (auto i = 1; i < 3; i++) {
            auto size = this->linesize[1] / 2 * this->height;
            f.data[i] = static_cast<unsigned char *>(malloc(size));
            memcpy(f.data[i], this->data[i], size);
        }
    }
    if (isRGB()) {
        f.data[0] = static_cast<unsigned char *>(malloc(f.cap));
        memcpy(f.data[0], this->data[0], this->cap);
    }   
    return f;
}

void Frame::crop(Frame frame, int w, int h, int offsetX, int offsetY)
{
    if (isRGB()) {
        return cropRGB(frame, w, h, offsetX, offsetY);
    }
    return cropYUV(frame, w, h, offsetX, offsetY);
}

void Frame::cropRGB(Frame frame, int w, int h, int offsetX, int offsetY)
{  
    auto wx4 = w * 4;
    auto hx4 = h * 4;
    auto offsetXx4 = offsetX *4;
    for (int i = 0; i < h ; i++) {
        memcpy(data[0] + wx4 * i,
            frame.data[0] + frame.linesize[0] * (offsetY + i) * 4  + offsetXx4,
            static_cast<size_t>(hx4));
    }
    sizeInBytes = hx4 * w;
}

void Frame::cropYUV(Frame frame, int w, int h, int offsetX, int offsetY)
{    
    for (int i = 0; i < h ; i++) {
        memcpy(y() + w * i,
            frame.y() + frame.linesize[0] * (offsetY + i) + offsetX,
            static_cast<size_t>(w));
    }

    auto offsetX_2 = offsetX / 2;
    auto offsetY_2 = offsetY / 2;
    auto halfH = h / 2;
    auto halfW = w / 2;
    for (auto i = 0; i < halfH; i++) {
        // u
        memcpy(u() + halfW * i,
            frame.u() + frame.linesize[1] * (i + offsetY_2) + offsetX_2 ,
             static_cast<size_t>(w / 2));
        // v
        memcpy(v() + halfW * i,
               frame.v() + frame.linesize[2] * (i + offsetY_2) + offsetX_2,
                static_cast<size_t>(halfW));
    }    
    sizeInBytes = w * h * 1.5;
}

void Frame::mallocData(int size)
{
    freeMem();

    if (isRGB()) {
        if (size == 0) {
            size = linesize[0] * height * 4;
        }
        cap = size;
        data[0] = static_cast<unsigned char *>(malloc(cap));
        return;
    }
    data[0] = static_cast<unsigned char *>(malloc(linesize[0]  * height));
    cap = linesize[0]  * height;
    for (auto i = 1; i < 3; i++) {
        data[i] = static_cast<unsigned char *>(malloc(linesize[i] / 2 * height));
        cap += linesize[i] / 2 * height;
    }
}

void Frame::mallocData(int *linesize, int h)
{
    setLinesize(linesize, 3);
    this->height = h;
    mallocData();

}

bool Frame::isNull() const
{
    return this->data[0] == nullptr || linesize[0] < 1 || height < 1;
}

void Frame::freeMem()
{
    for (auto i = 0; i < 3; i++) {
        if (this->data[i] != nullptr) {
            free(this->data[i]);
            this->data[i] = nullptr;
        }
    }
}
