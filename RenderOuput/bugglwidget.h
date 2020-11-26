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

namespace BugAV {
constexpr int BUFF_SIZE = 2;

class LIB_EXPORT BugGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, public IBugAVRenderer
{
    Q_OBJECT
public:
    explicit BugGLWidget(QWidget *parent = nullptr);
    ~BugGLWidget() override;

    void updateData(AVFrame *frame) override;

    void setRegionOfInterest(int x, int y, int w, int h) override;
    void setRegionOfInterest(int x, int y, int w, int h, bool emitUpdate);

    QSize videoFrameSize() const override;
    void initShader(int w,int h);
//    QObject *widget() override;

    void setQuality(int quality) override;
    void setOutAspectRatioMode(int ratioMode) override;
    QImage receiveFrame(const QImage& frame) override;

    QImage getLastFrame() const;

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

     int fastUp16(int x);

     void copyData();

     void prepareYUV(AVFrame *frame);
     void prepareRGB(AVFrame *frame);

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
        uint8_t *y;
        uint8_t *u;
        uint8_t *v;
    };

    typedef struct {
        unsigned char* data[3];
        int linesize[3];
    } Frame;

    Frame originFrame;

    YUVBuff yuvBuffer[BUFF_SIZE];

    QVector<QImage> images;

    int bufIndex;   

    QVector<QImage> imagesTransform;
    int bufImgTransformIndex;

    QMutex mutex;

    int kUpdate;

    QRect realRoi;

    bool hasInitYUV;
    bool hasInitRGB;

    int pixFmt;

    unsigned char* dataRGB;   


    // IBugAVRenderer interface
public:
    void newImageBuffer(const QImage &img) override;
    void updateImageBuffer(const QImage &img) override;
};
}
#endif // BugGLWidget_H
