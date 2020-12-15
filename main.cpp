#include "form.h"
#include "grid.h"
#include "statckwidget.h"

#include <QApplication>
#include <QWidget>
#include "QWindow"
#include "PlayM4.h"
#include "common/define.h"
#include <Decoder/fakestreamdecoder.h>

//#include <BugPlayer/bugplayer.h>
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QApplication a(argc, argv);

    auto file = "rtmp://61.28.231.227:1935/live/756bd856-6774-44eb-9fa7-92b2fcfe1167_sub_1587032712?ci=JDc1NmJkODU2LTY3NzQtNDRlYi05ZmE3LTkyYjJmY2ZlMTE2NwMxMjcAA3N1YgN2Y2MbMWFUbGx4enY0dEtOV2xaZldTelZmU1lGczQ2AAA=&sig=1bdd79324";
//    auto file = QLatin1String("http://api.stg.vcloudcam.vn/rec/v1/segment/playlist/?camUuid=45236d67-fc26-42a0-9bd5-be920da274be\u0026expire=1587545369\u0026from=1587281755\u0026to=1587285384\u0026token=88d0bb2052d228e39c107ec23f57d60bfc997e76\u0026type=hls");
//    VideoState is;
//    Demuxer demuxer;
//    demuxer.setIs(&is);
//    demuxer.setFileUrl(file);
//    demuxer.start();

//    VideoDecoder vDecoder;
//    vDecoder.setIs(&is);
//    vDecoder.start();
//    BugGLWidget s;
//    Render render;
//    render.setIs(&is);
//    render.setRender(&s);
//    render.start();

//    auto img  =QImage("/home/sondq/Documents/dev/build-BugAV-Desktop_Qt_5_12_6_GCC_64bit-Debug/492067917.png");
//    s.setCurrentImage(img);
//    s.show();
//    BugPlayer::setLog();
//    BugPlayer bplayer;
//    bplayer.setFile(file);
//    bplayer.play();
//    bplayer.show();
//    StatckWidget w;

    Grid w;
    w.show();

//    FakeStreamDecoder fs{nullptr};


//    QWidget w;
//    w.show();
//    fs.setWindowForHIKSDK(&w);

//    int g_lPort = -1;

//    if (g_lPort == -1)
//    {
//        LONG x;
//        auto bFlag = PlayM4_GetPort(&x);
//        if (bFlag == false)
//        {
//            PlayM4_GetLastError(g_lPort);
//            return 0;
//        }
//        g_lPort = x;
//    }

//    PlayM4_SetStreamOpenMode(g_lPort, STREAME_FILE);
////    checkError("PlayM4_SetStreamOpenMode", g_lPort);

////   PlayM4_OpenStream(g_lPort, (PBYTE)x, 40, 1024 * 1024);
////   PlayM4_OpenStream(g_lPort, nullptr, 0, 1024 * 1024);
////    char name[] = "/home/sondq/Documents/dev/build-BugAV-Desktop_Qt_5_12_9_GCC_64bit-Debug/abc/fba57199-d08c-4cca-a937-d5d6fade9946/0.ts";
//    char name[] = "/home/sondq/Downloads/abc.mp4";
//    PlayM4_OpenFile(g_lPort, name);
////    checkError("PlayM4_OpenFile", g_lPort);

//    auto hWnd = PLAYM4_HWND(w.windowHandle()->winId());
//    PlayM4_Play(g_lPort, hWnd);
////    checkError("PlayM4_Play", g_lPort);

    a.exec();

//    demuxer.stop();
//    vDecoder.stop();
    return 0;
}
