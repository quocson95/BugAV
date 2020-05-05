#include "bugglwidget.h"

extern "C" {
#include <libavutil/frame.h>
}
#include <QPainter>
#include <QPaintEngine>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QCoreApplication>
#include <math.h>
#include "QTimer"

namespace BugAV {


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
const GLfloat m_verticesA[20] = { 1, 1, 0, 1, 0, -1, 1, 0, 0, 0, -1, -1, 0, 0,
                            1, 1, -1, 0, 1, 1, };  

#define glsl(x) #x
static const char kFragmentShaderRGB[] = glsl(
            uniform sampler2D u_Texture0;
            varying mediump vec2 v_TexCoords;
            void main() {
                vec4 c = texture2D(u_Texture0, v_TexCoords);
                gl_FragColor = c.rgba;
            });

#undef glsl
//------------------------------------------


void BugGLWidget::initializeTexture( GLuint  id, int width, int height) {


    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, nullptr);
}

void BugGLWidget::freeBuffer()
{
//    for(int i=0;i<BUF_SIZE;i++)
//    {
//       if (buffer[i] != nullptr) {
//           free (buffer[i]);
//           buffer[i] = nullptr;
//       }
//    }
    for(int i=0;i<BUF_SIZE;i++) {
        if (yuvBuffer[i].y != nullptr) {
            free (yuvBuffer[i].y);
        }
        if (yuvBuffer[i].v != nullptr) {
             free (yuvBuffer[i].v);
        }
        if (yuvBuffer[i].u != nullptr)  {
             free (yuvBuffer[i].u)  ;
        }

        yuvBuffer[i].y = nullptr;
        yuvBuffer[i].u = nullptr;
        yuvBuffer[i].v = nullptr;
    }
     for(int i=0;i<3;i++) {
         if (originFrame.data[i] != nullptr) {
             free(originFrame.data[i]);
         }
         originFrame.data[i] = nullptr;
     }
}

void BugGLWidget::initBuffer(int *linesize, int h)
{
    freeBuffer();
//    for(int i=0;i<BUF_SIZE;i++)
//    {
//       buffer[i]= static_cast<unsigned char*>(malloc(size));
//    }

    for(auto i = 0; i < 3; i++) {
        originFrame.linesize[i] = linesize[i];
        originFrame.data[i] = static_cast<unsigned char*>(malloc(size_t((linesize[i]) * (h))));
    }
    for(int i=0;i<BUF_SIZE;i++)
    {
       yuvBuffer[i].y= static_cast<unsigned char*>(malloc(linesize[0] * h));
       yuvBuffer[i].u= static_cast<unsigned char*>(malloc(linesize[1] * h));
       yuvBuffer[i].v= static_cast<unsigned char*>(malloc(linesize[2] * h));
    }
}

void BugGLWidget::initYUV()
{
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

    auto  positionHandle=m_programA->attributeLocation("aPosition");
    auto textureHandle=m_programA->attributeLocation("aTextureCoord");

    glVertexAttribPointer(GLuint(positionHandle), 3, GL_FLOAT, false,
                          5 * sizeof(GLfloat), m_verticesA);

    glEnableVertexAttribArray(GLuint(positionHandle));

    glVertexAttribPointer(GLuint(textureHandle), 2, GL_FLOAT, false,
                          5 * sizeof(GLfloat), &m_verticesA[3]);

    glEnableVertexAttribArray(GLuint(textureHandle));

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);
    i=m_programA->uniformLocation("Utex");
    glUniform1i(i, 1);
    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);
}

int BugGLWidget::fastUp16(int x)
{
    return ((1 + ((x - 1) / 16)) << 4);
}

void BugGLWidget::copyData()
{
//    QMutexLocker lock(&mutex);
//    Q_UNUSED(lock)


    mutex.lock();
    auto index = bufIndex + 1;
    if(index>=BUF_SIZE)
        index=0;
    for (int i = 0; i < renderH ; i++) {
        memcpy(yuvBuffer[index].y + renderW * i,
            originFrame.data[0] + originFrame.linesize[0] * (offsetY + i) + offsetX,
            static_cast<size_t>(renderW));
    }

    auto offsetX_2 = offsetX / 2;
    auto offsetY_2 = offsetY / 2;
    for (auto i = 0; i < renderH / 2; i++) {
         memcpy(yuvBuffer[index].u + renderW / 2 * i,
                originFrame.data[1] + originFrame.linesize[1] * (i + offsetY_2) + offsetX_2 ,
                 static_cast<size_t>(renderW / 2));
     }

    // V
    for (auto i = 0; i < renderH / 2; i++) {
        memcpy(yuvBuffer[index].v + renderW / 2 * i,
               originFrame.data[2] + originFrame.linesize[2] * (i + offsetY_2) + offsetX_2,
                static_cast<size_t>(renderW / 2));
    }
    bufIndex = index;
    mutex.unlock();
    emit reqUpdate();
}

BugGLWidget::BugGLWidget(QWidget *parent)
    :QOpenGLWidget{parent}
    ,m_vshaderA{nullptr}
    ,m_fshaderA{nullptr}
    ,m_programA{nullptr}
    ,m_texture(nullptr)
    ,m_transparent(false)
    ,m_background(Qt::white)
{
//    moveToThread(qApp->thread());
    frameW=0;
    frameH=0;
    frameWxH = 0;
    FrameWxH_4 = 0;

    renderH = 0;
    renderW = 0;
    renderWxH = 0;
    renderWxH_4 = 0;
    offsetX = offsetY = 0;

    bufIndex=0;
//    memset(m_textureIds, 0, 3);
    for(auto i = 0; i < 3; i++) {
        m_textureIds[i] = i;
        originFrame.data[i] = nullptr;
    }
    yuvBuffer[0].y = yuvBuffer[0].u = yuvBuffer[0].v = nullptr;
    yuvBuffer[1].y = yuvBuffer[1].u = yuvBuffer[1].v = nullptr;
    isShaderInited=false;
    connect(this, &BugGLWidget::reqUpdate, this, &BugGLWidget::callUpdate);
//    setROI(100, 100,100, 100);
//    setGeometry(100, 200, 200,100);
    init = false;
}

BugGLWidget::~BugGLWidget()
{
    qDebug() << " BugGLWidget destroy";
    disconnect(this, &BugGLWidget::reqUpdate, this, &BugGLWidget::callUpdate);
    // And now release all OpenGL resources.
    makeCurrent();

    freeBuffer();

    if (m_vshaderA != nullptr) {
        delete m_vshaderA;
    }
    if (m_fshaderA != nullptr) {
        delete m_fshaderA;
    }
    if (m_texture != nullptr) {
        delete m_texture;
    }
    if (m_programA) {
        delete m_programA;
    }

    doneCurrent();    
}

void BugGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    initYUV();
}

void BugGLWidget::callUpdate()
{
    this->update();
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


void BugGLWidget::initShader(int w, int h, int *linesize)
{
    if(frameW != w || frameH != h)
    {
        this->frameW = w;
        this->frameH = h;

        frameWxH = size_t(frameW) * size_t(frameH);
        FrameWxH_4 = frameWxH / 4;
        initBuffer(linesize, h);
        isShaderInited=false;
        setRegionOfInterest(0, 0, frameW, frameH);
    }
}

void BugGLWidget::updateData(AVFrame *frame)
{
    if (frame->linesize[0] < 0) {
        qDebug() << "Current buggl render not support frame linesize < 0";        
        return;
    }


    if (frame->width != frameW
            || frame->height != frameH)
    {
        initShader(frame->width, frame->height, frame->linesize);
    }    
    mutex.lock();
    memcpy(originFrame.data[0], frame->data[0], frame->linesize[0] * frame->height);
    memcpy(originFrame.data[1], frame->data[1], frame->linesize[1] * frame->height/2);
    memcpy(originFrame.data[2], frame->data[2], frame->linesize[2] * frame->height/2);
    mutex.unlock();
    copyData();
}

void BugGLWidget::setRegionOfInterest(int x, int y, int w, int h)
{
//    QMutexLocker lock(&mutex);
//    Q_UNUSED(lock)
    if (x <= 0) {
        x = 0;
    }
    if (y <= 0) {
        y = 0;
    }
    if (w <= 0) {
        w = frameW;
    }
    if (h <= 0) {
        h = frameH;
    }
    if (w == frameW && h == frameH) {
        renderW = frameW;
        renderH = frameH;
        offsetX = 0;
        offsetY = 0;
    } else {
        renderW = fastUp16(w);
        renderH = fastUp16(h);
        offsetX = x;
        offsetY = y;
        if (renderW + offsetX > frameW ) {
            offsetX = frameW - renderW;
        }
        if (renderH +offsetY > frameH) {
            offsetY = frameH - renderH;
        }
    }

//    qDebug() << "w " << renderW << " h " << renderH;
    renderWxH = size_t(renderW) * size_t(renderH);
    renderWxH_4 = renderWxH / 4;
    isShaderInited=false;
    copyData();
}

QSize BugGLWidget::videoFrameSize() const
{
    return QSize(frameW, frameH);
}



void BugGLWidget::paintGL()
{
    //return;

//    if (!init) {
//        initYUV();
//        init = true;
//    }
    if(!isShaderInited)
    {
        isShaderInited=true;
        glDeleteTextures(3, m_textureIds);

        glGenTextures(3, m_textureIds);

        glActiveTexture(GL_TEXTURE0);
        initializeTexture( m_textureIds[0], renderW, renderH);
        glActiveTexture(GL_TEXTURE1);
        initializeTexture( m_textureIds[1], renderW / 2, renderH / 2);
        glActiveTexture(GL_TEXTURE2);
        initializeTexture( m_textureIds[2], renderW / 2, renderH / 2);
    }

//    glViewport(0, 0, width, height);

    if(renderW ==0 || renderH ==0)return;
    m_programA->bind();

//    mutex.lock();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW, renderH, GL_RED,
//                    GL_UNSIGNED_BYTE, buffer[bufIndex]);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW, renderH, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, yuvBuffer[bufIndex].y);

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW / 2, renderH / 2, GL_RED,
//                    GL_UNSIGNED_BYTE, buffer[bufIndex] + renderWxH);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW/2, renderH/2, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, yuvBuffer[bufIndex].u);

    i=m_programA->uniformLocation("Utex");
    glUniform1i(i, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
//    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW / 2, renderH / 2, GL_RED,
//                    GL_UNSIGNED_BYTE, buffer[bufIndex] + renderWxH +renderWxH_4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, renderW/2, renderH/2, GL_LUMINANCE,
                    GL_UNSIGNED_BYTE, yuvBuffer[bufIndex].v);


//    mutex.unlock();
    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);


    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_programA->release();


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
    if (w > 0 || h > 0) {
        glViewport(0, 0, w, h);
    }
}
}
