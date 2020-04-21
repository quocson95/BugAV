#ifndef RENDERER_H
#define RENDERER_H

#include <QGLWidget>
#include <QMutex>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class Renderer : public QGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit Renderer(QWidget *parent = nullptr);
   ~Renderer();
    void setCurrentImage(QImage value);

signals:
    void needReDraw();
public slots:
    void draw();
private:
    void loadTexture();
private:
    QImage currentImage;
    QMutex mutex;

    // QOpenGLWidget interface
protected:
    void paintGL();
    void resizeGL(int w, int h);
    void initializeGL();
};

#endif // RENDERER_H
