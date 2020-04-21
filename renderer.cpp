#include "renderer.h"

#include <QGLWidget>
#include "QApplication"
static GLuint textureID(10);

Renderer::Renderer(QWidget *parent) : QGLWidget(parent)
{    
//    context()->moveToThread(qApp->thread());
    connect(this, &Renderer::needReDraw, this, &Renderer::draw);
}

Renderer::~Renderer()
{
    makeCurrent();
    doneCurrent();
}

void Renderer::setCurrentImage(QImage value)
{
    mutex.lock();
    currentImage = value;
    mutex.unlock();
    emit needReDraw();
}

void Renderer::draw()
{
    loadTexture();
    updateGL();
}

void Renderer::loadTexture()
{
    makeCurrent();
    glEnable(GL_TEXTURE_2D); // Enable texturing

    glGenTextures(1, &textureID); // Obtain an id for the texture
    glBindTexture(GL_TEXTURE_2D, textureID); // Set as the current texture

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    mutex.lock();
    QImage tex = QGLWidget::convertToGLFormat(currentImage);
    mutex.unlock();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width(), tex.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.bits());

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glDisable(GL_TEXTURE_2D);
}

void Renderer::paintGL()
{
    if (!textureID)
        return;
    glClearColor(0.4f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-1, -1, -1);
        glTexCoord2f(1,0); glVertex3f( 1, -1, -1);
        glTexCoord2f(1,1); glVertex3f( 1,  1, -1);
        glTexCoord2f(0,1); glVertex3f(-1,  1, -1);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void Renderer::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, 0.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    glShadeModel(GL_FLAT);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    static GLfloat lightAmbient[4] = { 1.0, 1.0, 1.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
}
