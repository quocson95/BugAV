#ifndef BugGLWidget_H
#define BugGLWidget_H

#include "IRenderer.h"

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

#define BUF_SIZE 2



class BugGLWidget : public QOpenGLWidget, protected QOpenGLFunctions, public IRenderer
{
    Q_OBJECT
public:
    BugGLWidget();
    ~BugGLWidget() override;

    void updateData(unsigned char*);
    void updateData(unsigned char**) override;

    void initShader(int w,int h) override;
public slots:

    void setTransparent(bool transparent);


protected:
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void initializeGL() Q_DECL_OVERRIDE;
//    void loadBackGroundImage();

private:
     void initializeTexture( GLuint id, int width, int height);
     void freeBuffer();
     void initBuffer(int size);
private:
//    QTimer *timer;

    //--------------------
    QOpenGLShader *m_vshaderA;
    QOpenGLShader *m_fshaderA;
    QOpenGLShaderProgram *m_programA;
    QOpenGLBuffer m_vboA;
    //-------------------
    QOpenGLTexture *m_texture;

    bool m_transparent;

    QColor m_background;

    bool isShaderInited;
    int width;
    int height;
    quint8 *buffer[BUF_SIZE];
    int bufIndex;   
    QMutex mutex;

};

#endif // BugGLWidget_H
