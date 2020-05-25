#ifndef BUGPLAYER_H
#define BUGPLAYER_H
#include "marco.h"
#include <QObject>


namespace BugAV {

class IBugAVRenderer;
class BugPlayerPrivate;

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
        NoFrameRenderTooLong,
    };
    explicit BugPlayer(QObject *parent = nullptr);
    ~BugPlayer();

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
protected:
    explicit BugPlayer(BugPlayerPrivate &d, QObject *parent = nullptr);
private:

    void streamLoaded();
    void streamLoadedFailed();

    void demuxerStopped();

    void readFrameError();

    void firstFrameComming();

    void noRenderNewFrameLongTime();

private:
    //Q_DECLARE_PRIVATE(BugPlayer);
    QScopedPointer<BugPlayerPrivate> d_ptr;
};
}
Q_DECLARE_METATYPE(BugAV::BugPlayer::AVState);
Q_DECLARE_METATYPE(BugAV::BugPlayer::MediaStatus);
#endif // BUGPLAYER_H
