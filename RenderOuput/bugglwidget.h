#ifndef BugGLWidget_H
#define BugGLWidget_H

#include "IBugAVRenderer.h"
#include "marco.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTime>
#include <QVector>
#include <QOpenGLTexture>
#include <QMutex>
#include <QReadWriteLock>

namespace BugAV {
constexpr int BUFF_SIZE = 2;

class LIB_EXPORT BugGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, public IBugAVRenderer
{
    Q_OBJECT
public:
    explicit BugGLWidget(QWidget *parent = nullptr);
    ~BugGLWidget() override;

    void updateData(AVFrame *frame) override;
    void updateData(Frame *frame) override;

    void setRegionOfInterest(int x, int y, int w, int h) override;
    void setRegionOfInterest(int x, int y, int w, int h, bool emitUpdate);

    QSize videoFrameSize() const override;
    void initShader(int w,int h);
//    QObject *widget() override;

    void setQuality(int quality) override;
    void setOutAspectRatioMode(int ratioMode) override;
    QImage receiveFrame(const QImage& frame) override;

    QImage getLastFrame() const;
    Frame getLastDisplayFrame();

    void clearBufferRender() override;

public slots:

    void setTransparent(bool transparent);

protected:
//    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void initializeGL() Q_DECL_OVERRIDE;

private: signals:
    void reqUpdate();
private slots:
    void callUpdate();
private:
     void initializeTexture( GLuint id, int frameW, int frameH);
     void freeBufferYUV();
     void initBufferYUV(int *linesize, int h);

     void initBufferRGB(int w, int h );
     void freeBufferRGB();

     void initializeYUVGL();

     int incIndex(int index, int maxIndex);
     int fastUp16(int x);

     void cropYUVFrame();
     void cropRGBFrame();

     void prepareYUV(AVFrame *frame);
     void prepareRGB(AVFrame *frame);
     void prepareRGB(Frame *frame);

     void drawYUV();
     void drawRGB();
     void reDraw();

protected:
     // somtime need trigger receiveFrame but no need render
     bool noNeedRender;
private:
//    QTimer *timer;
    bool init;
    //--------------------
    GLuint          m_textureIds[3];
    QOpenGLShader *m_vshaderA;
    QOpenGLShader *m_fshaderA;
    QOpenGLShaderProgram *m_programA;
    QOpenGLBuffer m_vboA;
    //-------------------
    QOpenGLTexture *m_texture;

    bool m_transparent;

    QColor m_background;

    bool isShaderInited;
    int frameW;
    int frameH;
    int lineSize;
    size_t frameWxH;
    size_t FrameWxH_4;

    int renderW, renderH;
    size_t renderWxH;
    size_t renderWxH_4;

    QSize picSize;

    int offsetX, offsetY;

    struct YUVBuff {
        unsigned char *y;
        unsigned char *u;
        unsigned char *v;
    };

    Frame originFrame;

    Frame yuvBuffer[BUFF_SIZE];

    Frame rgbBuffer[BUFF_SIZE]; // buffer data for painter
    Frame rawRgbData; // raw frame data from ffmpeg
    Frame rawRgbFilter; // raw frame data after filter by client


    int bufIndex;

    QVector<QImage> imagesTransform;
    int bufImgTransformIndex;

    QMutex mutex, mutexBufRGB;

    int kUpdate;

    QRect realRoi;

    bool hasInitYUV;
    bool hasInitRGB;

    int pixFmt;

    // prevent access buffer render
    // when re alloc in case with or height of frame change
    //
    QReadWriteLock rwMutex;
    bool denyPaint;

    // IBugAVRenderer interface
public:
    void newFrame(const Frame &frame) override;
    void updateFrameBuffer(const Frame &frame) override;

    // QOpenGLWidget interface
protected:
    void resizeGL(int w, int h) override;
};
}
#endif // BugGLWidget_H
