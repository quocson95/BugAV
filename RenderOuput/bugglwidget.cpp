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
    "yuv.x = texture2D(Ytex, vTextureCoord).r - 0.0625;\n" // -16
    "yuv.y = texture2D(Utex, vTextureCoord).r - 0.5;\n"   // -128
    "yuv.z = texture2D(Vtex, vTextureCoord).r - 0.5;\n"  // -128
//    "rgb = mat3(1,1,1,\n"
//    "0,-0.39465,2.03211,\n"
//    "1.13983, -0.58060,  0) * yuv;\n"
    "rgb = mat3(1.164,1.164,1.164,\n"
    "0,-0.392,2.017,\n"
    "1.596, -0.813,  0) * yuv;\n"
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

void BugGLWidget::freeBufferYUV()
{
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

void BugGLWidget::initBufferYUV(int *linesize, int h)
{
    hasInitYUV = true;
    freeBufferYUV();

    for(auto i = 0; i < 3; i++) {
        originFrame.linesize[i] = linesize[i];
        originFrame.data[i] = static_cast<unsigned char*>(malloc(size_t((linesize[i]) * (h))));
    }
    for(int i=0;i<BUF_SIZE;i++)
    {
       yuvBuffer[i].y= static_cast<unsigned char*>(malloc(linesize[0] * h));
       yuvBuffer[i].u= static_cast<unsigned char*>(malloc(linesize[1] * h/2));
       yuvBuffer[i].v= static_cast<unsigned char*>(malloc(linesize[2] * h/2));
    }
}

void BugGLWidget::initBufferRGB(int w, int h)
{
    hasInitRGB = true;
    freeBufferRGB();
    images.reserve(BUF_SIZE);
    for(auto i = 0; i < BUF_SIZE; i++) {
        images.push_back(QImage(w, h, QImage::Format_RGB32));
    }
    const size_t rgb_stride = w*3 +(16-(3*w)%16)%16;
    dataRGB = static_cast<unsigned char*>(_mm_malloc(rgb_stride * h, 16));
}

void BugGLWidget::freeBufferRGB()
{
    images.clear();
    if (dataRGB != nullptr) {
        _mm_free(dataRGB);
        dataRGB = nullptr;
    }
}

void BugGLWidget::initializeYUVGL()
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

void BugGLWidget::prepareYUV(AVFrame *frame)
{
    if (frame->width != frameW
            || frame->height != frameH || !hasInitYUV)
    {
        initBufferYUV(frame->linesize, frame->height);
        initShader(frame->width, frame->height);        
    }
    mutex.lock();
    memcpy(originFrame.data[0], frame->data[0], frame->linesize[0] * frame->height);
    memcpy(originFrame.data[1], frame->data[1], frame->linesize[1] * frame->height/2);
    memcpy(originFrame.data[2], frame->data[2], frame->linesize[2] * frame->height/2);
    mutex.unlock();
    copyData();
}

void BugGLWidget::prepareRGB(AVFrame *frame)
{
    if (frame->width != frameW
            || frame->height != frameH || !hasInitRGB)
    {
        initShader(frame->width, frame->height);
        initBufferRGB(frame->width, frame->height);
    }
    auto index = bufIndex + 1;
    if (index >= BUF_SIZE) {
        index = 0;
    }
    auto img = QImage(frame->data[0], frame->width, frame->height, QImage::Format_RGB32);
//    const size_t rgb_stride = frame->width*3 +(16-(3*frame->width)%16)%16;
//    yuv420_rgb24_sse(frame->width, frame->height,
//                     frame->data[0], frame->data[1], frame->data[2],
//                     frame->linesize[0], frame->linesize[1],
//                     dataRGB, rgb_stride, YCBCR_JPEG);
//    yuv420_2_rgb8888(dataRGB,
//                       frame->data[0], frame->data[1], frame->data[2],
//                       frame->width, frame->height,
//                        frame->linesize[0], frame->linesize[1],
//                        frame->width <<2,
//                        yuv2bgr565_table,
//                        0);

//    auto img = QImage(dataRGB, frame->width, frame->height, QImage::Format_RGB888);
    images.replace(index, img);
    bufIndex = index;
    emit reqUpdate();
}

void BugGLWidget::drawYUV()
{
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
}

void BugGLWidget::drawRGB()
{
    if (images.count() == 0) {
        return;
    }    
    auto img = images.at(bufIndex);
    img = receiveFrame(img);
    // update picture size, update roi
    if (picSize.width() != img.size().width() || picSize.height() != img.size().height()) {
        picSize = img.size();
        setRegionOfInterest(0, 0, picSize.width(), picSize.height());
    }
    if (renderW != picSize.width() || renderH != picSize.height()) {
        img = img.copy(QRect(offsetX, offsetY, renderW, renderH));
    }
    QPainter painter{this};
    painter.beginNativePainting();
    painter.setRenderHints(QPainter::SmoothPixmapTransform);
    painter.drawImage(rect(), img);
    painter.endNativePainting();
}

void BugGLWidget::reDraw()
{
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_NONE) {
        return;
    }
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        copyData();
    } else {
        emit reqUpdate();
    }
}

BugGLWidget::BugGLWidget(QWidget *parent)
    :QOpenGLWidget{parent}
    ,m_vshaderA{nullptr}
    ,m_fshaderA{nullptr}
    ,m_programA{nullptr}
    ,m_texture(nullptr)
    ,m_transparent(false)
    ,m_background(Qt::white)
    ,hasInitYUV{false}
    ,hasInitRGB{false}
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
    memset(m_textureIds, 0, 3);
    for(auto i = 0; i < 3; i++) {        
        originFrame.data[i] = nullptr;
    }
    yuvBuffer[0].y = yuvBuffer[0].u = yuvBuffer[0].v = nullptr;
    yuvBuffer[1].y = yuvBuffer[1].u = yuvBuffer[1].v = nullptr;
    isShaderInited=false;

    pixFmt = AVPixelFormat::AV_PIX_FMT_NONE;

    dataRGB = nullptr;
    connect(this, &BugGLWidget::reqUpdate, this, &BugGLWidget::callUpdate);
//    setROI(100, 100,100, 100);
//    setGeometry(100, 200, 200,100);
    init = false;
    setTransparent(true);
}

BugGLWidget::~BugGLWidget()
{
    qDebug() << " BugGLWidget destroy";
    disconnect(this, &BugGLWidget::reqUpdate, this, &BugGLWidget::callUpdate);
    // And now release all OpenGL resources.
    makeCurrent();

    freeBufferYUV();

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
    initializeYUVGL();
}

void BugGLWidget::callUpdate()
{
    this->update();
//    window()->update();
}

void BugGLWidget::initShader(int w, int h)
{
    if(frameW != w || frameH != h)
    {
        this->frameW = w;
        this->frameH = h;        

        picSize = QSize(w, h);

        frameWxH = size_t(frameW) * size_t(frameH);
        FrameWxH_4 = frameWxH / 4;       
        isShaderInited=false;
        setRegionOfInterest(0, 0, frameW, frameH, false);
    }
}

void BugGLWidget::setQuality(int quality)
{

}

void BugGLWidget::setOutAspectRatioMode(int ratioMode)
{

}

QImage BugGLWidget::receiveFrame(const QImage &frame)
{
    return frame;
}

QImage BugGLWidget::getLastFrame() const
{
    if (images.size() > bufIndex) {
        return images.at(bufIndex);
    }
    return QImage{};
}

void BugGLWidget::updateData(AVFrame *frame)
{
//    return;
    if (frame->linesize[0] < 0) {
        qDebug() << "Current buggl render not support frame linesize < 0";        
        return;
    }

    pixFmt = (AVPixelFormat)frame->format;
    if (frame->format == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        // prepare for yuv
        prepareYUV(frame);
    } else if (frame->format == AVPixelFormat::AV_PIX_FMT_RGB32) {
        // prepare for rgb
        prepareRGB(frame);
    }
    // no support another type
    return;
}

void BugGLWidget::setRegionOfInterest(int x, int y, int w, int h)
{
    setRegionOfInterest(x, y, w, h, true);
}

void BugGLWidget::setRegionOfInterest(int x, int y, int w, int h, bool emitUpdate)
{
    if (x <= 0) {
        x = 0;
    }
    if (y <= 0) {
        y = 0;
    }
    if (w <= 0) {
        w = picSize.width();
    }
    if (h <= 0) {
        h = picSize.height();
    }
    if (w == picSize.width() && h == picSize.height()) {
        renderW = picSize.width();
        renderH = picSize.height();
        offsetX = 0;
        offsetY = 0;
    } else {
        renderW = fastUp16(w);
        renderH = fastUp16(h);
        if (renderW > picSize.width()) {
            renderW = picSize.width();
        }
        if (renderH > picSize.height()) {
            renderH = picSize.height();
        }
        offsetX = x;
        offsetY = y;
        if (renderW + offsetX >  picSize.width() ) {
            offsetX =  picSize.width() - renderW;
        }
        if (renderH +offsetY >  picSize.height()) {
            offsetY =  picSize.height() - renderH;
        }

        if (offsetX < 0) {
            offsetX = 0;
        }
        if (offsetY < 0) {
            offsetY = 0;
        }
    }

//    qDebug() << "w " << renderW << " h " << renderH;
    renderWxH = size_t(renderW) * size_t(renderH);
    renderWxH_4 = renderWxH / 4;
    isShaderInited=false;
    if (emitUpdate) {
        reDraw();
    }
}

QSize BugGLWidget::videoFrameSize() const
{
    return picSize;
}

void BugGLWidget::paintGL()
{    
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        drawYUV();
    } else if (pixFmt == AVPixelFormat::AV_PIX_FMT_RGB32) {
        drawRGB();
    }
}



void BugGLWidget::setTransparent(bool transparent)
{
//    setAttribute(Qt::WA_TransparentForMouseEvents);
//    setAttribute(Qt::WA_AlwaysStackOnTop, transparent);
    m_transparent = transparent;
    // Call update() on the top-level window after toggling AlwayStackOnTop to make sure
    // the entire backingstore is updated accordingly.
    window()->update();
}

void BugGLWidget::resizeGL(int w, int h)
{
//    if (w > 0 || h > 0) {
//        glViewport(0, 0, w, h);
//    }
//    emit reqUpdate();
//    raise();
}
}
