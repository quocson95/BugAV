#include "vopengl.h"

extern "C" {
    #include <libavutil/frame.h>
}

#include <QOpenGLShaderProgram>

namespace BugAV {

const char gl_indices[] = { 0, 3, 2, 0, 2, 1 };
const char gl_vertextShader[] = { "attribute vec4 aPosition;\n"
                                 "attribute vec2 aTextureCoord;\n"
                                 "varying vec2 vTextureCoord;\n"
                                 "void main() {\n"
                                 "  gl_Position = aPosition;\n"
                                 "  vTextureCoord = aTextureCoord;\n"
                                 "}\n" };

const char gl_fragmentShader[] = {
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

const GLfloat ml_verticesA[20] = { 1, 1, 0, 1, 0, -1, 1, 0, 0, 0, -1, -1, 0, 0,
                            1, 1, -1, 0, 1, 1, };


#define glslv(x) #x
static const char gl_fragmentShaderRGB[] = glslv(
        uniform sampler2D Ytex;
        uniform sampler2D u_Texture0;
        varying vec2 vTextureCoord;
        void main() {
            vec4 c = texture2D(Ytex, vTextureCoord);
            gl_FragColor = c;
        });

static const char gl_fragmentShaderRGB_FEC[] = glslv(
        uniform sampler2D Ytex;
        uniform sampler2D u_Texture0;
        varying vec2 vTextureCoord;
        void main() {
//            float p = 90.0;
//            float t = 4.0;
//            float z = 1.0;
//            float r = 0.5;
            vec2 coord = vTextureCoord;
//            vec2 xyzr = vec2(coord.x * coord.x + coord.y * coord.y + z *z * r * r,
//                             coord.x * coord.x + coord.y * coord.y + z *z * r * r);
//            vec2 xy = vec2((r*(coord.x * cos(p) - coord.y*sin(p)*cos(t) + z*r*sin(t)*sin(p)))/sqrt(xyzr.x),
//                          (r*(coord.x * sin(p) - coord.y*cos(p)*cos(t) + z*r*sin(t)*cos(p)))/sqrt(xyzr.x));
//            vec2 xy = vec2(1, r*(coord.x * cos(p) - coord.y*sin(p)cos(t) ));
            vec4 c = texture2D(Ytex, coord);
            gl_FragColor = c;
        });
#undef glslv

VOpenGL::VOpenGL(QWidget *parent):
    QOpenGLWidget(parent)
  ,m_vshaderA{nullptr}
  ,m_fshaderA{nullptr}
  ,m_programA{nullptr}
  ,m_texture(nullptr)
  ,pixFmt{-1}
  ,allowPaint{true}
{

    renderH = 0;
    renderW = 0;
    renderWxH = 0;
    renderWxH_4 = 0;
    for (auto i = 0; i < BUFF_FRAME_SIZE; i++) {
        yuvBuffer[i].format = YUV_FORMAT;
        rgbBuffer[i].format = RGB_FORMAT;
    }
    connect(this, &VOpenGL::reqUpdate, [this]() {
        update();
    });
}

void VOpenGL::initializeTexture(GLuint id, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, nullptr);
}

void VOpenGL::initYUVBuffer(int *linesize, int h)
{
    hasInitYUV = true;
    freeYUVBuffer();
    for (auto i = 0; i < BUFF_FRAME_SIZE; i++) {
        yuvBuffer[i].mallocData(linesize, h);
    }

}

void VOpenGL::freeYUVBuffer()
{
    for (auto i = 0; i < BUFF_FRAME_SIZE; i++) {
        yuvBuffer[i].freeMem();
    }
}

void VOpenGL::initRGBuffer(int *linesize, int h)
{
    hasInitRGB = true;
    freeRGBBuffer();
    for (auto i = 0; i < BUFF_FRAME_SIZE; i++) {
        rgbBuffer[i].mallocData(linesize, h);
    }
}

void VOpenGL::freeRGBBuffer()
{
    for (auto i = 0; i < BUFF_FRAME_SIZE; i++) {
        rgbBuffer[i].freeMem();
    }
}

void VOpenGL::initializeGLTexture()
{
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
        m_vshaderA->compileSourceCode(gl_vertextShader);
    }


    m_fshaderA = new QOpenGLShader(QOpenGLShader::Fragment);
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        m_fshaderA->compileSourceCode(gl_fragmentShader);
    } else {
        m_fshaderA->compileSourceCode(gl_fragmentShaderRGB);
    }



    m_programA = new QOpenGLShaderProgram;
    m_programA->addShader(m_vshaderA);
    m_programA->addShader(m_fshaderA);
    m_programA->link();

    auto  positionHandle=m_programA->attributeLocation("aPosition");
    auto textureHandle=m_programA->attributeLocation("aTextureCoord");

    glVertexAttribPointer(GLuint(positionHandle), 3, GL_FLOAT, false,
                          5 * sizeof(GLfloat), ml_verticesA);

    glEnableVertexAttribArray(GLuint(positionHandle));

    glVertexAttribPointer(GLuint(textureHandle), 2, GL_FLOAT, false,
                          5 * sizeof(GLfloat), &ml_verticesA[3]);

    glEnableVertexAttribArray(GLuint(textureHandle));

    int i=m_programA->uniformLocation("Ytex");
    glUniform1i(i, 0);
    i=m_programA->uniformLocation("Utex");
    glUniform1i(i, 1);
    i=m_programA->uniformLocation("Vtex");
    glUniform1i(i, 2);
}

void VOpenGL::prepareBuffer(int *linesize, int h)
{
    if (pixFmt == AVPixelFormat::AV_PIX_FMT_YUV420P) {
        // prepare for yuv
        hasInitRGB = false;
        initYUVBuffer(linesize, h);
    } else if (pixFmt == AVPixelFormat::AV_PIX_FMT_RGB32) {
        // prepare for rgb
        hasInitYUV  = false;
        initRGBuffer(linesize, h);
    }
    initShader(linesize[0], h);
}

void VOpenGL::initShader(int w, int h)
{
    if (frameW != w || frameH != h) {
        this->frameW = w;
        this->frameH = h;

        picSize = QSize(w, h);
        frameWxH = w * h;
        frameWxH_4 = frameWxH / 4;
        isShaderInited = false;
        setRegionOfInterest(0, 0, frameW, frameH, false);
    }
}

int VOpenGL::fastUp16(int x) const
{
    return ((1 + ((x - 1) / 16)) << 4);
}

void VOpenGL::setRegionOfInterest(int x, int y, int w, int h, bool emitUpdate)
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

    renderWxH = size_t(renderW) * size_t(renderH);
    renderWxH_4 = renderWxH / 4;
    isShaderInited=false;
    if (emitUpdate) {
        reDraw();
    }
}

void VOpenGL::reDraw()
{
    emit reqUpdate();
}

void VOpenGL::initializeGL()
{
    initializeOpenGLFunctions();
}

void VOpenGL::paintGL()
{

}

}
