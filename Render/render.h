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
class QElapsedTimer;

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

    void positionChanged(qint64 time);
private:
    bool initPriv();

    bool isAvailFirstFrame();

    void freeSwsBuff();

private:
    static double vp_duration(double maxFrameDuration, Frame *vp, Frame *nextvp);
    static double compute_target_delay (VideoState *is, double delay);
    static void getSdlPixFmtAndBlendmode(int format, uint32_t *pix_fmt);    
    static AVPixelFormat fixDeprecatedPixelFormat(AVPixelFormat fmt);
    void updateVideoPts(double pts, int64_t pos, int serial);

    void uploadTexture(Frame *f, SwsContext **img_convert_ctx);

private slots:
    void process();
    void videoRefresh();
    void videoDisplay();

    void updatePositionChanged(Frame *vp);
private:
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

    QElapsedTimer *elPrevFrame;

    QElapsedTimer *elTimer;

    qint64 currentFramePts;

public:
    void setSaveRawImage(bool value);
    void setPreferPixFmt(const AVPixelFormat &value);
    qint64 getCurrentPosi() const;
    AVPixelFormat getPreferPixFmt() const;
};
}
#endif // RENDER_H

