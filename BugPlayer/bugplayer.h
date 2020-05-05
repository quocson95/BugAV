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

    void demuxerStopped();
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

//    TaskScheduler *taskScheduler;


    // QObject interface
protected:
    void timerEvent(QTimerEvent *event);
};
} // namespace BugAV
#endif // BugPlayer_H

