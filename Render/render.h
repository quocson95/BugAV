#ifndef RENDER_H
#define RENDER_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QRunnable>
extern "C" {
#include <libavutil/frame.h>
}

class QThread;
struct SwsContext;

namespace BugAV {

class VideoState;
class Frame;
class IBugAVRenderer;
class BugFilter;

class Render:public QObject //, public QRunnable
{
    Q_OBJECT
public:
    Render();
    Render(VideoState *is, IBugAVRenderer *renderer = nullptr);
    ~Render();

    void start();
    void stop();
    void setIs(VideoState *value);

    void setRenderer(IBugAVRenderer *value);
    IBugAVRenderer *getRenderer();

    bool getRequestStop() const;
    void setRequestStop(bool value);

    bool isRunning() const;

    QString statistic();
public: signals:
    void firstFrameComming();
    void noRenderNewFrameLongTime();
private:
    bool initPriv();

    bool isAvailFirstFrame();

    void freeSwsBuff();

    bool handlerFrameState1();
    bool handlerFrameState2();
    int handlerFrameState3();

private:
    static double vp_duration(double maxFrameDuration, Frame *vp, Frame *nextvp);
    static double compute_target_delay (VideoState *is, double delay);
    static void getSdlPixFmtAndBlendmode(int format, uint32_t *pix_fmt);
    static QImage::Format avFormatToImageFormat(int format);
    static AVPixelFormat fixDeprecatedPixelFormat(AVPixelFormat fmt);
//    int getSDLPixFmt(int format);
    void updateVideoPts(double pts, int64_t pos, int serial);

    void uploadTexture(Frame *f, SwsContext **img_convert_ctx);

//    AVFrame *cropImage(AVFrame *frame, int left, int top, int right, int bottom);
private slots:
    void process();
    void videoRefresh();
    void videoDisplay();
private:

    enum class PrivState {
        Stop = -1,
        Init = 0,
        WaitingFirstFrame,
        HandleFrameState1,
        HandleFrameState2,
        HandleFrameState3,        
    };
//    AVFrame *frameYUV;
    uint8_t * buffer;
//    uint8_t *out_buffer;
    bool hasInit;
    QThread *thread;
    IBugAVRenderer *renderer;
    IBugAVRenderer *defaultRenderer;
    VideoState *is;
//    BugFilter *filter;
    double rdftspeed = 0.02;
    double remaining_time = 0;
    bool requestStop;
    QMutex mutex;
    bool isRun;

    PrivState privState;

    qint64 lastUpdateFrame;

    QImage *img;

    bool saveRawImage = false;


    qint64 lastVideoRefresh;

    QTimer *timerCheckNoFrameRender;

    uint8_t *dst_data[4];
    bool initSwsBuff = false;
    int dst_linesize[4];

    // prefer pixel format for render
    // default will using AV_PIX_FMT_YUV420P for render
    // else force to rgb AV_PIX_FMT_RGB32
    // todo allow another format
    AVPixelFormat preferPixFmt;

    // QRunnable interface
public:
    void run();
    void setSaveRawImage(bool value);
    void setPreferPixFmt(const AVPixelFormat &value);
};
}
#endif // RENDER_H

