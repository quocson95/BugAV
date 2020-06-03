#include "bugglrgb.h"
#include <QDebug>

extern "C" {
    #include <libavutil/frame.h>
}
#include <QDateTime>
#include <QPainter>

namespace BugAV {
BugGlRgbOutput::BugGlRgbOutput(QWidget *parent)
    :QOpenGLWidget{parent}
    ,bufIndex{0}
    ,frameW{0}
    ,frameH{0}    
//    ,buffer{nullptr}
{
    buffer[0] = nullptr;
    buffer[1] = nullptr;
}

BugGlRgbOutput::~BugGlRgbOutput()
{

}

void BugGlRgbOutput::updateData(AVFrame *frame)
{
    if (frame->linesize[0] < 0) {
        qDebug() << "Current buggl render not support frame linesize < 0";
        return;
    }
    if (frameW == 0 || frameH == 0 ||
            frameW != frame->width || frameH != frame->height) {
        initShader(frame->width, frame->height, frame->linesize);
    }
    auto index = bufIndex + 1;
    if (index >= BUF_SIZE) {
        index = 0;
    }
    auto img = QImage(frame->data[0], frame->width, frame->height, QImage::Format_RGB32);
    images.replace(index, view180Degree(img));
    bufIndex = index;    
    update();
}

void BugGlRgbOutput::setRegionOfInterest(int x, int y, int w, int h)
{

}

QSize BugGlRgbOutput::videoFrameSize() const
{
    return QSize(0, 0);
}

void BugGlRgbOutput::setQuality(int quality)
{

}

void BugGlRgbOutput::setOutAspectRatioMode(int ratioMode)
{

}

void BugGlRgbOutput::initializeGL()
{
    initializeOpenGLFunctions();
}

void BugGlRgbOutput::paintGL()
{
    if (buffer[bufIndex] == nullptr) {
        return;
    }
//    QImage tmpImg((uchar *)buffer[bufIndex],frameW, frameH,QImage::Format_RGB32);
//    QImage img = tmpImg.scaled(this->size(),Qt::IgnoreAspectRatio);

    if (images.count() == 0) {
        return;
    }
    QPainter painter{this};
    painter.beginNativePainting();
    painter.drawImage(rect(), images.at(bufIndex));
    painter.endNativePainting();
}

void BugGlRgbOutput::initBuffer()
{

}

void BugGlRgbOutput::freeBuffer()
{
    if (images.count() > 0) {
//        qDeleteAll(images);
        images.clear();
    }
    for (auto i = 0; i < BUF_SIZE; i++) {
        if (buffer[i] != nullptr) {
            free(buffer[i]);
        }
    }
}

void BugGlRgbOutput::initShader(int w, int h, int *linesize)
{
    freeBuffer();
    frameW = w;
    frameH = h;
    for (auto i = 0; i < BUF_SIZE; i++) {
//        auto img = new QImage(w, h,  QImage::Format_RGB888);
        images.push_back(QImage(w, h,  QImage::Format_RGB32));
    }
    buffer[0] = static_cast<unsigned char*>(malloc(size_t(linesize[0] * h *2)));
    buffer[1] = static_cast<unsigned char*>(malloc(size_t(linesize[0] * h * 2)));
}

QPoint BugGlRgbOutput::findPoint(int Xe, int Ye, double R, double Cfx, double Cfy, double He, double We, double angle)
{
    double r= Ye / He * R;
    double theta = Xe / We * 2.0 * M_PI + angle;

    double xF = Cfx + r * sin(theta);
    double yF = Cfy + r * cos(theta);

    return QPoint(static_cast<int>(xF), static_cast<int>(yF));
}

QImage BugGlRgbOutput::process(const QImage &src, int ratio, double angle)
{
    int Hf = src.height();
    int Wf = src.width();


    double R = Hf / 2.0;
    double Cfx = Wf / 2.0;
    double Cfy = Hf / 2.0;
    double hF = R;
    double wF = ratio * M_PI * R;

    QImage dst(static_cast<int>(wF), static_cast<int>(hF), src.format());

    for (int Xe = 0; Xe < dst.width(); ++Xe) {
       for (int Ye = 0; Ye < dst.height(); ++Ye) {
           QPoint point = findPoint(Xe, Ye, R, Cfx, Cfy, hF, wF, angle);
           dst.setPixelColor(dst.width() - 1 - Xe, dst.height() - 1 - Ye, src.pixelColor(point));
       }
    }

    return dst.copy({0, 0, dst.width(), 4*dst.height()/5});
}

QImage BugGlRgbOutput::view360Degree(const QImage &image)
{
    return process(image, 1, 0);
}

QImage BugGlRgbOutput::view180Degree(const QImage &image)
{
    QImage src = view360Degree(image);
    int w = src.width();
    int h = src.height();
    QImage dst(static_cast<int>(ceil(w/2.0)), h*2, src.format());

    QPainter painer(&dst);
    painer.drawImage(QPoint{0, 0}, src, {QPoint{0, 0}, QPoint{w/2, h}});
    painer.drawImage(QPoint{0, h}, src, {QPoint{w/2, 0}, QPoint{w, h}});
    return dst;
}
}
