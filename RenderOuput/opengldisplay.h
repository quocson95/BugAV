#ifndef OPENGLDISPLAY_H
#define OPENGLDISPLAY_H

#include "IRenderer.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QScopedPointer>
#include <QException>

class OpenGLDisplay : public QOpenGLWidget, public QOpenGLFunctions, public IRenderer
{
public:
    explicit OpenGLDisplay(QWidget* parent = nullptr);
    ~OpenGLDisplay();

//    void InitDrawBuffer(unsigned bsize);
    void DisplayVideoFrame(unsigned char *data, int frameWidth, int frameHeight);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    struct OpenGLDisplayImpl;
    QScopedPointer<OpenGLDisplayImpl> impl;

    // IRenderer interface
public:
    void updateData(unsigned char **data);
    void initShader(int w, int h);
};

/***********************************************************************/

class OpenGlException: public QException
{
public:
     void raise() const { throw *this; }
     OpenGlException *clone() const { return new OpenGlException(*this); }
};

#endif // OPENGLDISPLAY_H
