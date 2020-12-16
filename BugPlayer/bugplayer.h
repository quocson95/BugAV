#ifndef BUGPLAYER_H
#define BUGPLAYER_H
#include "marco.h"
#include <QObject>
#include "common/global.h"

namespace BugAV {

class IBugAVRenderer;
class BugPlayerPrivate;

class LIB_EXPORT BugPlayer: public QObject
{
    Q_OBJECT
public:

    explicit BugPlayer(QObject *parent = nullptr, ModePlayer mode = ModePlayer::RealTime);
    ~BugPlayer();

    void setFile(const QString &file);
    QString getFile() const;

    void play();

    void play(const QString &file);

    void pause();

    void togglePause();

    void stop();

    void refresh();

    // refresh and seek to current position
    void refreshAtCurrent();

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

    void setPixFmtRGB32(bool value = true);

    // set speed, can set anytime
    void setSpeed(const double & speed);

    void setStartPosition(const qint64 & time);
    qint64 getStartPosition() const;

    qint64 getDuration() const;

    void seek(const double& position);

    void setDisableAudio(bool value = true);

    void setMute(bool value = true);

    bool isMute() const;

    QMap<QString, QString> getMetadata() const;

    void enableHIKSDK();
    void setWindowForHIKSDK(QWidget *w);
    void setWindowFishEyeForHIKSDK(QString id, QWidget *w);

Q_SIGNALS:
    void stateChanged(BugAV::AVState state);
    void mediaStatusChanged(BugAV::MediaStatus state);
    void error(QString err);

    void seekFinished(qint64 timestamp);

    void positionChanged(qint64);

    void durationChanged(qint64 duration);

private slots:
    void positionChangedSlot(qint64);
protected:
    explicit BugPlayer(BugPlayerPrivate &d, QObject *parent = nullptr);
private:

    void streamLoaded();
    void streamLoadedFailed();

    void demuxerStopped();

    void readFrameError();

    void firstFrameComming();

    void noRenderNewFrameLongTime();

    void noMoreFrame();

private:
    //Q_DECLARE_PRIVATE(BugPlayer);
    QScopedPointer<BugPlayerPrivate> d_ptr;
    bool needSeekCurrent;
    bool useHIKSDK;
};
}
Q_DECLARE_METATYPE(BugAV::AVState);
Q_DECLARE_METATYPE(BugAV::MediaStatus);
#endif // BUGPLAYER_H
