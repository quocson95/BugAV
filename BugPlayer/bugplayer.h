#ifndef BugPlayer_H
#define BugPlayer_H

#include <QString>

#include <RenderOuput/bugglwidget.h>
#include <RenderOuput/opengldisplay.h>

class VideoState;
class Demuxer;
class VideoDecoder;
class Render;

class BugPlayer: public BugGLWidget
{
public:
    BugPlayer();
    ~BugPlayer();
    static void setLog();

    void setFile(const QString &file);

    void play();

    void pause();

    void stop();

    void reset();

    bool isPlaying() const;

    bool isSourceChange() const;

private:
    void initPriv();
    void playPriv();

private slots:
    void workerStarted();
    void workerStopped();

    void streamLoaded();
private:
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    Render *render;

    QString curFile;

    bool demuxerRunning;
    bool vDecoderRunning;
    bool renderRunning;    
};

#endif // BugPlayer_H
