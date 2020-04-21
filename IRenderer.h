#ifndef IRENDERER_H
#define IRENDERER_H

class IRenderer {

public:
    IRenderer() = default;
    virtual ~IRenderer() = default;
    virtual void updateData(unsigned char**) = 0;
    virtual void initShader(int w,int h) = 0;
};

#endif // IRENDERER_H
