#ifndef BugPlayer_H
#define BugPlayer_H

#include <QString>
#include <QVariantHash>

namespace BugAV {
class VideoState;
class Demuxer;
class VideoDecoder;
class Render;
class IBugAVRenderer;
class BugPlayer;

class BugPlayerPrivate
{
public:

    explicit BugPlayerPrivate(BugPlayer *q);
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

    bool isSourceChange() const;

    qint64 buffered() const;

    qreal bufferPercent() const;

    void setRenderer(IBugAVRenderer *renderer);

    IBugAVRenderer *getRenderer();

    void setOptionsForFormat(QVariantHash avFormat);


    QString statistic();

    bool setSaveRawImage(bool save = false);


    void setEnableFramedrop(bool value = true);


    BugPlayer *q_ptr;
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    Render *render;

    QString curFile;

    bool demuxerRunning;
    bool vDecoderRunning;
    bool renderRunning;
    bool enableFramedrop;


private:
    void initPriv();
    void playPriv();

};
} // namespace BugAV

#endif // BugPlayer_H

