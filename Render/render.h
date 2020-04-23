#ifndef RENDER_H
#define RENDER_H

#include <QImage>
#include <QObject>
extern "C" {
#include <libavutil/frame.h>
}
class QThread;
class VideoState;
class Frame;
class IRenderer;
class BugFilter;
class SwsContext;

class Render: public QObject
{
    Q_OBJECT
public:
    Render();
    Render(IRenderer *renderer, VideoState *is);
    ~Render();

    void start();
    void stop();
    void setIs(VideoState *value);

    void setRenderer(IRenderer *value);

    bool getRequestStop() const;
    void setRequestStop(bool value);

    bool isRunning() const;
private:
    void initPriv();
signals:
    void started();
    void stopped();
private:
    static double vp_duration(double maxFrameDuration, Frame *vp, Frame *nextvp);
    static double compute_target_delay (VideoState *is, double delay);
    static void getSdlPixFmtAndBlendmode(int format, uint32_t *pix_fmt);
    static QImage::Format avFormatToImageFormat(int format);
    static AVPixelFormat fixDeprecatedPixelFormat(AVPixelFormat fmt);
//    int getSDLPixFmt(int format);
    void updateVideoPts(double pts, int64_t pos, int serial);

    void uploadTexture(Frame *f, SwsContext **img_convert_ctx);

    AVFrame *cropImage(AVFrame *frame, int left, int top, int right, int bottom);
private slots:
    void process();
    void videoRefresh();
    void videoDisplay();
private:
    AVFrame *frameYUV;
    uint8_t *out_buffer;
    bool hasInit;
    QThread *thread;
    IRenderer *renderer;
    VideoState *is;
    BugFilter *filter;
    double rdftspeed = 0.02;
    double remaining_time = 0;
    bool requestStop;    

};

#endif // RENDER_H
