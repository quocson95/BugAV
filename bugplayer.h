#ifndef BugPlayer_H
#define BugPlayer_H

#include "bugglwidget.h"

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
    void start();
private:

private:
    VideoState *is;
    Demuxer *demuxer;
    VideoDecoder *vDecoder;
    Render *render;

};

#endif // BugPlayer_H
