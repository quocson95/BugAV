#ifndef BugPlayer_H
#define BugPlayer_H

#include <QString>
#include <taskscheduler.h>

#include <RenderOuput/bugglwidget.h>

#include "marco.h"

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
    BugPlayer(QObject *parent = nullptr);
    ~BugPlayer();
    static void setLog();

    static void stopTaskScheduler();

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


public: signals:
    void stateChanged(BugAV::BugPlayer::AVState state);


private:
    void initPriv();
    void playPriv();

private slots:
    void workerStarted();
    void workerStopped();

    void streamLoaded();
    void streamLoadedFailed();
private:
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    Render *render;

    QString curFile;

    bool demuxerRunning;
    bool vDecoderRunning;
    bool renderRunning;

//    TaskScheduler *taskScheduler;

};
} // namespace BugAV
#endif // BugPlayer_H

