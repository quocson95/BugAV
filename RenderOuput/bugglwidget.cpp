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

#ifdef Q_OS_LINUX
#include <x86intrin.h>
#endif

#ifdef Q_OS_WINDOWS
#pragma comment(lib, "opengl32.lib")
#endif
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
    "rgb = mat3(1.164,1.164,1.164,\n"
    "0,-0.392,2.017,\n"
    "1.596, -0.813,  0) * yuv;\n"
    "gl_FragColor = vec4(rgb, 1);\n"
    "}\n" };
const GLfloat m_verticesA[20] = { 1, 1, 0, 1, 0, -1, 1, 0, 0, 0, -1, -1, 0, 0,
                            1, 1, -1, 0, 1, 1, };  


#define glsl(x) #x
static const char g_fragmentShaderRGB[] = glsl(
        uniform sampler2D Ytex;
        uniform sampler2D u_Texture0;
        varying vec2 vTextureCoord;
        void main() {
            vec4 c = texture2D(Ytex, vTextureCoord);
            gl_FragColor = c;
        });
#undef glsl
//------------------------------------------

BugGLWidget::BugGLWidget(QWidget *parent)
    :QOpenGLWidget{parent}
    ,noNeedRender{false}
    ,m_vshaderA{nullptr}
    ,m_fshaderA{nullptr}
    ,m_programA{nullptr}
    ,m_texture(nullptr)
    ,m_transparent(false)
    ,m_background(Qt::white)

    ,hasInitYUV{false}
    ,hasInitRGB{false}
    ,denyPaint{false}
{
//    moveToThread(qApp->thread());
    frameW=0;
    frameH=0;
    lineSize = 0;
    frameWxH = 0;
    FrameWxH_4 = 0;

    renderH = 0;
    renderW = 0;
    renderWxH = 0;
    renderWxH_4 = 0;
    offsetX = offsetY = 0;

    bufIndex=0;
    bufImgTransformIndex = 0 ;
    memset(m_textureIds, 0, sizeof(GLuint) * 3);
    for(auto i = 0; i < 3; i++) {
        originFrame.data[i] = nullptr;
        rawRgbData.data[0] = nullptr;
    }
    yuvBuffer[0].y = yuvBuffer[0].u = yuvBuffer[0].v = nullptr;
    yuvBuffer[1].y = yuvBuffer[1].u = yuvBuffer[1].v = nullptr;

    for(auto i = 0; i < BUFF_SIZE; i++) {
        rgbBuffer[i].y = rgbBuffer[i].u = rgbBuffer[i].v = nullptr;
    }

//    rgbBuffer[1].y = rgbBuffer[1].u = rgbBuffer[1].v = nullptr;
    isShaderInited=false;

    pixFmt = AVPixelFormat::AV_PIX_FMT_NONE;

//    dataRGB = nullptr;
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
    freeBufferRGB();

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
    for(int i=0;i<BUFF_SIZE;i++) {
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
    this->lineSize = linesize[0];
    for(auto i = 0; i < 3; i++) {
        originFrame.linesize[i] = linesize[i];
        originFrame.data[i] = static_cast<unsigned char*>(malloc(size_t((linesize[i]) * (h))));
    }
    for(int i=0;i<BUFF_SIZE;i++)
    {
       yuvBuffer[i].y= static_cast<unsigned char*>(malloc(linesize[0] * h));
       yuvBuffer[i].u= static_cast<unsigned char*>(malloc(linesize[1] * h/2));
       yuvBuffer[i].v= static_cast<unsigned char*>(malloc(linesize[2] * h/2));
    }
}

void BugGLWidget::initBufferRGB(int w, int h)
{   
    hasInitRGB = true;
    if (rawRgbData.data[0] != nullptr) {
        free(rawRgbData.data[0]);
    }

    for(auto i = 0; i < BUFF_SIZE; i++) {
        if (rgbBuffer[i].y != nullptr) {
            free(rgbBuffer[i].y);

        }
    }

    auto size = w * h * 4;
    for(auto i = 0; i < BUFF_SIZE; i++) {
        rgbBuffer[i].y = static_cast<unsigned char*>(malloc(size));
    }
    rawRgbData.data[0] = static_cast<unsigned char*>(malloc(size));
    rawRgbData.height = h;
    rawRgbData.cap = size;

    rawRgbFilter.data[0] = static_cast<unsigned char*>(malloc(size));
    rawRgbFilter.cap = size;
    for(auto i = 0; i < 3; i++) {
        rawRgbData.linesize[i] = w;       
//        rawRgbData.data[i] = static_cast<unsigned char*>(malloc(size_t((w) * (h))));
    }
}

void BugGLWidget::freeBufferRGB()
{

//    images.clear();
//    imagesTransform.clear();
//    if (dataRGB != nullptr) {
//        _mm_free(dataRGB);
//        dataRGB = nullptr;
//    }
}

void BugGLWidget::initializeYUVGL()
{
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
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
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);




    if (m_programA != nullptr) {
        m_programA->release();
        delete m_programA;
    }

    if (m_fshaderA != nullptr) {
        m_fshaderA->deleteLater();
        m_fshaderA = nullptr;
    }
    //---------------------
    if (m_vshaderA == nullptr) {
        m_vshaderA = new QOpenGLShader(QOpenGLShader::Vertex);
        m_vshaderA->compileSourceCode(g_vertextShader);
    }


    m_fshaderA = new QOpenGLShader(QOpenGLShader::Fragment);
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        m_fshaderA->compileSourceCode(g_fragmentShader);
    } else {
        m_fshaderA->compileSourceCode(g_fragmentShaderRGB);
    }



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

int BugGLWidget::incIndex(int index, int maxIndex)
{
    auto i = index + 1;
    if (i >= maxIndex) {
        i=0;
    }
    return i;
}

int BugGLWidget::fastUp16(int x)
{
    return ((1 + ((x - 1) / 16)) << 4);
}

void BugGLWidget::cropYUVFrame()
{
    mutex.lock();   
    auto index = bufIndex + 1;
    if(index>=BUFF_SIZE)
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

void BugGLWidget::cropRGBFrame()
{
    auto index = incIndex(bufIndex, BUFF_SIZE);
    for (int i = 0; i < renderH ; i++) {
        memcpy(rgbBuffer[index].y + renderW * i * 4,
            rawRgbFilter.data[0] + rawRgbFilter.linesize[0] * (offsetY + i) * 4  + offsetX *4,
            static_cast<size_t>(renderW*4));
    }
    bufIndex = index;
    emit reqUpdate();
}

void BugGLWidget::prepareYUV(AVFrame *frame)
{
    if (    lineSize != frame->linesize[0]
            || frame->width != frameW
            || frame->height != frameH
            || !hasInitYUV)
    {
        initBufferYUV(frame->linesize, frame->height);
        initShader(frame->width, frame->height);        
    }
    mutex.lock();
    memcpy(originFrame.data[0], frame->data[0], frame->linesize[0] * frame->height);
    memcpy(originFrame.data[1], frame->data[1], frame->linesize[1] * frame->height/2);
    memcpy(originFrame.data[2], frame->data[2], frame->linesize[2] * frame->height/2);
    mutex.unlock();
    cropYUVFrame();
}

void BugGLWidget::prepareRGB(AVFrame *frame)
{
    if (frame->width != frameW
            || frame->height != frameH || !hasInitRGB)
    {        
        rwMutex.lockForWrite();
        denyPaint = true;
        rwMutex.unlock();

        initBufferRGB(frame->width, frame->height);
        initShader(frame->width, frame->height);        

        rwMutex.lockForWrite();
        denyPaint = false;
        rwMutex.unlock();
    }
//    auto index = bufIndex + 1;
//    if (index >= BUFF_SIZE) {
//        index = 0;
//    }
//    auto img = QImage(frame->data[0], frame->width, frame->height, QImage::Format_RGB32);
//    img.save("/home/sondq/Downloads/aaa/img.pn");    
    auto size = frame->width * frame->height * 4;
    memcpy(rawRgbData.data[0], frame->data[0], size);
    rawRgbData.sizeInBytes = size;
    newFrame(rawRgbData);
//    cropRGBFrame();
//    bufIndex = index;
//    emit reqUpdate();
}

void BugGLWidget::drawYUV()
{    
    m_programA->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

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

    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);
    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

//    m_programA->release();
}

void BugGLWidget::drawRGB()
{
    if (rgbBuffer[bufIndex].y == nullptr) {
        return;
    }
    m_programA->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, renderW, renderH, 0, GL_BGRA, GL_UNSIGNED_BYTE, rgbBuffer[bufIndex].y);

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);

     glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, g_indices);

//    if (images.count() == 0) {
//        return;
//    }
//    auto imgInBuf = images.at(bufIndex);
//    receiveFrame(imgInBuf);
//    auto imgPainter = imagesTransform.at(bufImgTransformIndex);
//    if (imgPainter.width() == 0 || noNeedRender) {
//        return;
//    }
//    // update picture size, update roi
//    if (picSize.width() != imgPainter.size().width() || picSize.height() != imgPainter.size().height()) {
//        picSize = imgPainter.size();
//        setRegionOfInterest(0, 0, picSize.width(), picSize.height());
//    }
//    if (renderW != picSize.width() || renderH != picSize.height()) {
//        imgPainter = imgPainter.copy(QRect(offsetX, offsetY, renderW, renderH));
//    }
//    QPainter painter{this};
//    painter.beginNativePainting();
//    // Maybe necessary
//    glPushAttrib(GL_ALL_ATTRIB_BITS);
//    glMatrixMode(GL_PROJECTION);
//    glPushMatrix();
//    glMatrixMode(GL_MODELVIEW);
//    glPushMatrix();

//    // Put OpenGL code here

//    // Necessary if used glPush-es above
//    glMatrixMode(GL_PROJECTION);
//    glPopMatrix();
//    glMatrixMode(GL_MODELVIEW);
//    glPopMatrix();
//    glPopAttrib();
//    painter.endNativePainting();
//    auto r = rect();
//    painter.setRenderHints(QPainter::SmoothPixmapTransform);
//    painter.drawImage(r, imgPainter);
//    painter.end();
////    painter.endNativePainting();
}

void BugGLWidget::reDraw()
{
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_NONE) {
        cropRGBFrame();
        return;
    }
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        cropYUVFrame();
    } else {
        emit reqUpdate();
    }
}

void BugGLWidget::newFrame(const Frame &frame)
{
    updateFrameBuffer(std::forward<const Frame&>(frame));
}

void BugGLWidget::updateFrameBuffer(const Frame &frame)
{
    // todo realloc rawRgbFilter if rawRgbFilter.cap < frame.sizeInBytes

    QMutexLocker locker(&mutexBufRGB);
    rawRgbFilter.sizeInBytes = frame.sizeInBytes;
    for (auto i = 0 ; i < 3; i++) {
        rawRgbFilter.linesize[i] = frame.linesize[i];
    }
    memcpy(rawRgbFilter.data[0], frame.data[0], frame.sizeInBytes);
    if (picSize.width() != frame.linesize[0] || picSize.height() !=  frame.height) {
        picSize = QSize(frame.linesize[0], frame.height);
        setRegionOfInterest(offsetX, offsetY, renderW, renderH);
    } else {
        cropRGBFrame();
    }
    emit reqUpdate();
}

//void BugGLWidget::newImageBuffer(const QImage &img)
//{
//    updateImageBuffer(img);
//}

//void BugGLWidget::updateImageBuffer(const QImage &img)
//{
//    auto index = bufImgTransformIndex + 1;
//    if (index >= BUFF_SIZE) {
//        index = 0;
//    }
//    imagesTransform.replace(index, img.copy());
//    bufImgTransformIndex = index;
//    emit reqUpdate();
//}

void BugGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
//    initializeYUVGL();
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
    Q_UNUSED(quality)
}

void BugGLWidget::setOutAspectRatioMode(int ratioMode)
{
    Q_UNUSED(ratioMode)
}

QImage BugGLWidget::receiveFrame(const QImage &frame)
{
    return frame;
}

QImage BugGLWidget::getLastFrame() const
{
//    if (images.size() > bufIndex) {
//        return images.at(bufIndex);
//    }
    return QImage{};
}

void BugGLWidget::clearBufferRender()
{
//    if (images.size() < BUFF_SIZE) {
//        return;
//    }
//    for(auto i = 0; i < BUFF_SIZE; i++) {
//        auto index = bufIndex + 1;
//        if (index >= BUFF_SIZE) {
//            index = 0;
//        }
//    images.replace(i, QImage{});
//    }
}

void BugGLWidget::updateData(AVFrame *frame)
{
    if (frame->linesize[0] < 0) {
        qDebug() << "Current buggl render not support frame linesize < 0";        
        return;
    }

    if (pixFmt != frame->format) {
        isShaderInited = false;
    }
    pixFmt = (AVPixelFormat)frame->format;
    if (frame->format == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        // prepare for yuv        
        hasInitRGB = false;
        prepareYUV(frame);
    } else if (frame->format == AVPixelFormat::AV_PIX_FMT_RGB32) {
        // prepare for rgb        
        hasInitYUV  = false;
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

    if (noNeedRender) {
        return;
    }
// ---------------------
    rwMutex.lockForRead();
    if (denyPaint) {
        rwMutex.unlock();
        return;
    }
// ---------------------
    if(!isShaderInited)
    {
        isShaderInited=true;
        initializeYUVGL();
        glDeleteTextures(3, m_textureIds);
        memset(m_textureIds, 0, 3);

        glGenTextures(3, m_textureIds);

        glActiveTexture(GL_TEXTURE0);
        initializeTexture( m_textureIds[0], renderW, renderH);
        glActiveTexture(GL_TEXTURE1);
        initializeTexture( m_textureIds[1], renderW / 2, renderH / 2);
        glActiveTexture(GL_TEXTURE2);
        initializeTexture( m_textureIds[2], renderW / 2, renderH / 2);
    }

//    glViewport(0, 0, width, height);

    if(renderW ==0 || renderH ==0) {
        rwMutex.unlock();
        return;
    }

    if (AVPixelFormat(pixFmt) == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        drawYUV();
    } else if (AVPixelFormat(pixFmt) == AVPixelFormat::AV_PIX_FMT_RGB32) {
        drawRGB();
    }
    rwMutex.unlock();
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
}
