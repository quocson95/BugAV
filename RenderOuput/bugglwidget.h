#ifndef BugGLWidget_H
#define BugGLWidget_H

#include "IBugAVRenderer.h"
#include "marco.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QTime>
#include <QVector>
#include <QOpenGLTexture>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

namespace BugAV {
#define BUF_SIZE 2

class LIB_EXPORT BugGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, public IBugAVRenderer
{
    Q_OBJECT
public:
    BugGLWidget(QWidget *parent = nullptr);
    ~BugGLWidget() override;

    void updateData(AVFrame *frame) override;

    void setRegionOfInterest(int x, int y, int w, int h) override;

    QSize videoFrameSize() const override;
    void initShader(int w,int h, int *linesize);
//    QObject *widget() override;

public slots:

    void setTransparent(bool transparent);    

protected:
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void initializeGL() Q_DECL_OVERRIDE;
//    void loadBackGroundImage();

private: signals:
    void reqUpdate();
private slots:
    void callUpdate();
private:
     void initializeTexture( GLuint id, int frameW, int frameH);
     void freeBuffer();
     void initBuffer(int *linesize, int h);

     void initYUV();

     int fastUp16(int x);

     void copyData();
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
    size_t frameWxH;
    size_t FrameWxH_4;

    int renderW, renderH;
    size_t renderWxH;
    size_t renderWxH_4;

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

    YUVBuff yuvBuffer[BUF_SIZE];

    int bufIndex;   
    QMutex mutex;

    int kUpdate;

    QRect realRoi;

};
}
#endif // BugGLWidget_H
