#ifndef VOPENGL_H
#define VOPENGL_H

#include "IBugAVRenderer.h"
#include "marco.h"

#include <QMutex>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class QOpenGLShader;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLTexture;

struct AVFrame;

namespace BugAV {

constexpr int BUFF_FRAME_SIZE = 2;
class LIB_EXPORT VOpenGL: public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    VOpenGL(QWidget *parent = nullptr);


private: signals:
    void reqUpdate();
private:   
    GLuint          m_textureIds[3];
    QOpenGLShader *m_vshaderA;
    QOpenGLShader *m_fshaderA;
    QOpenGLShaderProgram *m_programA;
    //-------------------
    QOpenGLTexture *m_texture;
    // QOpenGLWidget interface
protected:
    int renderW, renderH;
    int renderWxH;
    int renderWxH_4;

    int bufIdx;

    Frame yuvBuffer[BUFF_FRAME_SIZE];
    Frame rgbBuffer[BUFF_FRAME_SIZE];

    int pixFmt; // -1 yuv
    QMutex mtexIgnorePaint;
    bool allowPaint;

    bool hasInitYUV;
    bool hasInitRGB;

    int frameW, frameH, frameWxH, frameWxH_4;
    QSize picSize;
    bool isShaderInited;
    int offsetX, offsetY;

private:
    void initializeTexture( GLuint id, int width, int height);

    void initYUVBuffer(int *linesize, int h);
    void freeYUVBuffer();

    void initRGBuffer(int *linesize, int h);
    void freeRGBBuffer();

    void initializeGLTexture();

    void prepareBuffer(int *linesize, int h);
    void initShader(int w, int h);


protected:
    int fastUp16(int x) const;

    void setRegionOfInterest(int x, int y, int w, int h, bool emitUpdate);

    void reDraw();

protected:
    void initializeGL();
    void paintGL();


};
}
#endif // VOPENGL_H
