#include "bugglwidget.h"

#include <QPainter>
#include <QPaintEngine>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QCoreApplication>
#include <math.h>
#include "QTimer"


GLuint          m_textureIds[3];



//-------------------------------------
const char g_indices[] = { 0, 3, 2, 0, 2, 1 };
const char g_vertextShader[] = { "attribute vec4 aPosition;\n"
                                 "attribute vec2 aTextureCoord;\n"
                                 "varying vec2 vTextureCoord;\n"
                                 "void main() {\n"
                                 "  gl_Position = aPosition;\n"
                                 "  vTextureCoord = aTextureCoord;\n"
                                 "}\n" };
const char g_fragmentShader[] = {
    "uniform sampler2D Ytex,Utex,Vtex;\n"
    "varying vec2 vTextureCoord;\n"
    "void main(void) {\n"
    "vec3 yuv;\n"
    "vec3 rgb;\n"
    "yuv.x = texture2D(Ytex, vTextureCoord).r;\n"
    "yuv.y = texture2D(Utex, vTextureCoord).r - 0.5;\n"
    "yuv.z = texture2D(Vtex, vTextureCoord).r - 0.5;\n"
    "rgb = mat3(1,1,1,\n"
    "0,-0.39465,2.03211,\n"
    "1.13983, -0.58060,  0) * yuv;\n"
    "gl_FragColor = vec4(rgb, 1);\n"
    "}\n" };
GLfloat m_verticesA[20] = { 1, 1, 0, 1, 0, -1, 1, 0, 0, 0, -1, -1, 0, 0,
                            1, 1, -1, 0, 1, 1, };
//------------------------------------------


void BugGLWidget::initializeTexture( GLuint  id, int width, int height) {


    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, NULL);
}

void BugGLWidget::freeBuffer()
{
    for(int i=0;i<BUF_SIZE;i++)
    {
       if (buffer[i] != nullptr) {
           free(buffer[i]);
       }
    }
}

void BugGLWidget::initBuffer(int size)
{
    freeBuffer();
    for(int i=0;i<BUF_SIZE;i++)
    {
       buffer[i]=(unsigned char*)malloc(size);
    }
}



BugGLWidget::BugGLWidget()
    :m_texture(0),
      m_transparent(false),
      m_background(Qt::white)
{
    width=0;
    height=0;
    bufIndex=0;
    memset(buffer, 0, BUF_SIZE);
    buffer[0] = nullptr;
    buffer[1] = nullptr;
    memset(m_textureIds,0,3);
    isShaderInited=false;
}

BugGLWidget::~BugGLWidget()
{
    // And now release all OpenGL resources.
    makeCurrent();

    doneCurrent();
    freeBuffer();   
}

void BugGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glEnable(GL_TEXTURE_2D);

    /* The following three commented lines if uncommented generate following errors:
     * undefined reference to `_imp__glShadeModel@4'
     * undefined reference to `_imp__glClearDepth@8'
     * bad reloc address 0x20 in section `.eh_frame'
     */
   // glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
   // glClearDepth(1.0f);
   // glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);



    //---------------------
    m_vshaderA = new QOpenGLShader(QOpenGLShader::Vertex);
    m_vshaderA->compileSourceCode(g_vertextShader);
    m_fshaderA = new QOpenGLShader(QOpenGLShader::Fragment);
    m_fshaderA->compileSourceCode(g_fragmentShader);
    m_programA = new QOpenGLShaderProgram;
    m_programA->addShader(m_vshaderA);
    m_programA->addShader(m_fshaderA);
    m_programA->link();

    int positionHandle=m_programA->attributeLocation("aPosition");
    int textureHandle=m_programA->attributeLocation("aTextureCoord");

    glVertexAttribPointer(positionHandle, 3, GL_FLOAT, false,
                          5 * sizeof(GLfloat), m_verticesA);

    glEnableVertexAttribArray(positionHandle);

    glVertexAttribPointer(textureHandle, 2, GL_FLOAT, false,
                          5 * sizeof(GLfloat), &m_verticesA[3]);

    glEnableVertexAttribArray(textureHandle);

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);
    i=m_programA->uniformLocation("Utex");
    glUniform1i(i, 1);
    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);


    // glViewport(0, 0, width, height);


    //timer->start(30);
}

//void BugGLWidget::loadBackGroundImage()
//{
//    QImage img("/home/sondq/Documents/dev/BugAV/logo.png", "png");
////    auto i = QImage::Format(5);
//    uchar* out_buffer = (uchar*)av_malloc((int)ceil(img.height() * img.width() * 1.5));
//    //allocate ffmpeg frame structures
//    AVFrame* inpic = av_frame_alloc();
//    backgroundImage = av_frame_alloc();

//    avpicture_fill((AVPicture*)inpic,
//                   img.bits(),
//                   AV_PIX_FMT_RGB0,
//                   img.width(),
//                   img.height());

//    avpicture_fill((AVPicture*)backgroundImage,
//                   out_buffer,
//                   AV_PIX_FMT_YUV420P,
//                   img.width(),
//                   img.height());

//    SwsContext* ctx = sws_getContext(img.width(),
//                                     img.height(),
//                                     AV_PIX_FMT_RGB0,
//                                     img.width(),
//                                     img.height(),
//                                     AV_PIX_FMT_YUV420P,
//                                     SWS_BICUBIC,
//                                     NULL, NULL, NULL);
//    sws_scale(ctx,
//              inpic->data,
//              inpic->linesize,
//              0,
//              img.height(),
//              backgroundImage->data,
//              backgroundImage->linesize);

//    //free memory
//    av_free(inpic);
//    //free output buffer when done with it
//    av_free(out_buffer);
//}


void BugGLWidget::initShader(int w,int h)
{

    if(width!=w || height !=h)
    {
        width=w;
        height=h;
        int size=width*height*3/2;

        mutex.lock();        
        initBuffer(size);
        mutex.unlock();
        isShaderInited=false;
    }

}


void BugGLWidget::updateData(unsigned char *data)
{

   // unsigned char *tmp=buffer[bufIndex];

   // memcpy(tmp,data,width*height*3/2);


    //this->update();
}

void BugGLWidget::updateData(unsigned char**data)
{

    unsigned char *tmp=buffer[bufIndex];

    memcpy(tmp,data[0],width*height);
    tmp+=width*height;

    memcpy(tmp,data[1],width*height/4);

    tmp+=width*height/4;
    memcpy(tmp,data[2],width*height/4);

    this->update();
}



void BugGLWidget::paintGL()
{
    //return;


    if(!isShaderInited)
    {
        isShaderInited=true;
        glDeleteTextures(3, m_textureIds);

        glGenTextures(3, m_textureIds);

        glActiveTexture(GL_TEXTURE0);
        initializeTexture( m_textureIds[0], width, height);
        glActiveTexture(GL_TEXTURE1);
        initializeTexture( m_textureIds[1], width / 2, height / 2);
        glActiveTexture(GL_TEXTURE2);
        initializeTexture( m_textureIds[2], width / 2, height / 2);
    }
    //QPainter p;
    //p.begin(this);
    //p.beginNativePainting();
//    glClearColor(m_background.redF(), m_background.greenF(), m_background.blueF(), m_transparent ? 0.0f : 1.0f);
    //glViewport(0, 0, WIDTH, HEIGHT);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //  glFrontFace(GL_CW);
    // glCullFace(GL_FRONT);
    //  glEnable(GL_CULL_FACE);
    // glEnable(GL_DEPTH_TEST);
    //qDebug()<<"C===========";

    //glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    //glTexImage2D(GL_TEXTURE_2D, 0, 4, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);


    //m_texture->bind();
    //glColor3f(1.0f,0.0f,0.0f);
    /*  glBegin(GL_QUADS);

    // 前面

    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, -1.0f,  0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, -1.0f,  0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  0.0f);


    glEnd();
*/

//    QOpenGLContext::currentContext()->functions()->glViewport(100, 100, 100, 100);
    if(width ==0 || height ==0)return;
    m_programA->bind();


    mutex.lock();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, buffer[bufIndex]);

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, (char *) buffer[bufIndex] + width * height);

    i=m_programA->uniformLocation("Utex");
    glUniform1i(i, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, (char *) buffer[bufIndex] + width * height * 5 / 4);


    mutex.unlock();
    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);


    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_programA->release();

    bufIndex++;
    if(bufIndex>=BUF_SIZE)
        bufIndex=0;

    //   p.endNativePainting();
    //   p.end();

}



void BugGLWidget::setTransparent(bool transparent)
{
    setAttribute(Qt::WA_AlwaysStackOnTop, transparent);
    m_transparent = transparent;
    // Call update() on the top-level window after toggling AlwayStackOnTop to make sure
    // the entire backingstore is updated accordingly.
    window()->update();
}

void BugGLWidget::resizeGL(int w, int h)
{

}
