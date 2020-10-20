#ifndef BugPlayer_H
#define BugPlayer_H

#include <QString>
#include <QVariantHash>
#include "common/global.h"

namespace BugAV {
class VideoState;
class Demuxer;
class VideoDecoder;
class AudioDecoder;
class Render;
class IBugAVRenderer;
class BugPlayer;
class Define;
class AudioRender;

class BugPlayerPrivate
{
public:

    explicit BugPlayerPrivate(BugPlayer *q, ModePlayer mode);
    ~BugPlayerPrivate();
//    static void setLog();

    void setFile(const QString &file);
    QString getFile() const;

    int play();

    int play(const QString &file);

    int pause();

    void togglePause();

    void stop();

    void refresh();

    bool isPlaying() const;

    bool isPause() const;

    bool isSourceChange() const;

    qint64 buffered() const;

    qreal bufferPercent() const;

    void setRenderer(IBugAVRenderer *renderer);

    IBugAVRenderer *getRenderer();

    void setOptionsForFormat(QVariantHash avFormat);


    QString statistic();

    bool setSaveRawImage(bool save = false);


    void setEnableFramedrop(bool value = true);

    void setSpeed(const double & speed);

    void setStartPosition(const qint64 & time);
    qint64 getStartPosition() const;

    qint64 getDuration() const;

    void seek(const double& position);

    BugPlayer *q_ptr;
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    AudioDecoder *aDecoder;

    Render *render;
    AudioRender *audioRender;

    QString curFile;

    bool demuxerRunning;
    bool vDecoderRunning;
    bool renderRunning;
    bool enableFramedrop;

    float speed;

    Define *def;


private:
    void initPriv();
    int playPriv();

};
} // namespace BugAV

#endif // BugPlayer_H

