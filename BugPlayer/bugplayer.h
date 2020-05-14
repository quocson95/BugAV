#ifndef BugPlayer_H
#define BugPlayer_H

#include "marco.h"
#include <QObject>
#include <QString>
#include <RenderOuput/IBugAVRenderer.h>

namespace BugAV {
class VideoState;
class Demuxer;
class VideoDecoder;
class Render;

class LIB_EXPORT BugPlayer: public QObject
{
    Q_OBJECT
public:
    enum class AVState {
        StoppedState,
        LoadingState,
        PlayingState,
        PausedState,
        PlayingNoRenderState,

    };

    enum class MediaStatus {
        FirstFrameComing,
    };

    explicit BugPlayer(QObject *parent = nullptr);
    ~BugPlayer();
//    static void setLog();

    void setFile(const QString &file);
    QString getFile() const;

    void play();

    void play(const QString &file);

    void pause();

    void togglePause();

    void stop();

    void refresh();

    bool isPlaying() const;

    bool isSourceChange() const;

    qint64 buffered() const;

    qreal bufferPercent() const;

    void setRenderer(IBugAVRenderer *renderer);

    IBugAVRenderer *getRenderer();

    void setOptionsForFormat(QVariantHash avFormat);


    QString statistic();

    bool setSaveRawImage(bool save = false);


    void setEnableFramedrop(bool value = true);

Q_SIGNALS:
    void stateChanged(BugAV::BugPlayer::AVState state);
    void mediaStatusChanged(BugAV::BugPlayer::MediaStatus state);
    void error(QString err);


private:
    void initPriv();
    void playPriv();

private Q_SLOTS:
    void workerStarted();
    void workerStopped();

    void streamLoaded();
    void streamLoadedFailed();

    void demuxerStopped();

    void readFrameError();

    void firstFrameComming();
private:
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    Render *render;

    QString curFile;

    bool demuxerRunning;
    bool vDecoderRunning;
    bool renderRunning;

    int kUpdateStatistic;

    bool enableFramedrop;

//    TaskScheduler *taskScheduler;


    // QObject interface
protected:
    void timerEvent(QTimerEvent *event);
};
} // namespace BugAV

Q_DECLARE_METATYPE(BugAV::BugPlayer::AVState);
Q_DECLARE_METATYPE(BugAV::BugPlayer::MediaStatus);
#endif // BugPlayer_H

