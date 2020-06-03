#ifndef BUGGLRGB_H
#define BUGGLRGB_H

#include "IBugAVRenderer.h"

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace BugAV {
#define BUF_SIZE 2
class BugGlRgbOutput: public QOpenGLWidget, protected QOpenGLFunctions, public IBugAVRenderer
{
    Q_OBJECT
public:
    explicit BugGlRgbOutput(QWidget *parent = nullptr);
    ~BugGlRgbOutput();
    // IBugAVRenderer interface
public:
    void updateData(AVFrame *frame);
    void setRegionOfInterest(int x, int y, int w, int h);
    QSize videoFrameSize() const;
    void setQuality(int quality);
    void setOutAspectRatioMode(int ratioMode);

    // QOpenGLWidget interface
protected:
    void initializeGL();
    void paintGL();
    void initBuffer();
    void freeBuffer();

    void initShader(int w,int h, int *linesize);
private:
    int bufIndex;
    QVector<QImage> images;    
    int frameW;
    int frameH;    
    uint8_t *buffer[BUF_SIZE];

    // for transform
    QPoint findPoint(int Xe, int Ye, double R, double Cfx, double Cfy, double He, double We, double angle);
    QImage process(const QImage &src, int ratio, double angle);
    QImage view360Degree(const QImage &image);
    QImage view180Degree(const QImage &image);

};
}
#endif // BUGGLRGB_H
