
 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>
#include <RenderOuput/bugglrgb.h>
#include <RenderOuput/ibugavdefaultrenderer.h>

#include <common/packetqueue.h>

Grid::Grid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Grid)
{
    setWindowTitle("Grid");
    ui->setupUi(this);
//    file = "rtmp://61.28.233.149:1935/live/2bdd1370-e975-4eac-8bb8-12e07802e757_main_1588670960?ci=JDJiZGQxMzcwLWU5NzUtNGVhYy04YmI4LTEyZTA3ODAyZTc1NwIzOAAEbWFpbgN2Y2MbMWJUbmxBWkd4UWNYZ3JzRUFCQzQybUZyNm9mAAA=&sig=cc65438e8";
//    file = "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/301";
    size = 1;
    // camera fish eye
    files << "https://api.vcloudcam.vn/rec/v2/segment/playlist-public/?expire=1593672812&id=80e64d63810cf26dd44a2f825649f9514dafc730&tk=239b19e65230b298f2a292d5c68bd84b26cfa2b4&noRedirect=true";

    files << "rtsp://admin:Admin123@192.168.0.9:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:Admin123@vcloudcam.ddns.net:8556/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:Admin123@192.168.0.12:554/profile3";
             files << "rtsp://admin:Admin123@192.168.0.50:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_1";
             files << "rtsp://admin:Admin123@192.168.0.52:554/cam/realmonitor?channel=1&subtype=2&unicast=true&proto=Onvif";
             files << "rtsp://admin:Admin123@192.168.1.11:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:admin123@192.168.1.12:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/602";
             files << "rtsp://admin:Admin123@192.168.0.11:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/402";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/502";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/302";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/202";
             files << "rtsp://admin2:Admin123@192.168.0.99:554/Streaming/Channels/102";
             files << "rtsp://admin:Admin123@192.168.0.7/onvif-media/media.amp?profile=profile_1_h264&sessiontimeout=60&streamtype=unicast";
             files << "rtsp://admin:Admin123@192.168.0.111:554/user=admin_password=v0CilGW3_channel=1_stream=1.sdp?real_stream";
             files << "rtsp://admin:Admin123@192.168.0.6:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:Admin123@192.168.0.10:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:Admin123@192.168.0.13:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
             files << "rtsp://admin:Admin123@192.168.0.4:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
             files << "rtsp://onvif:Admin123@192.168.0.3:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
             files << "rtsp://admin:Admin123@192.168.0.2:554/Streaming/Channels/102?transportmode=unicast&profile=Profile_2";
             files << "rtsp://admin:Admin123@192.168.0.50:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1";
             files << "rtsp://admin:Admin123@192.168.0.12:554/profile3";


    start();
    BugAV::PacketQueue::mustInitOnce();
}

Grid::~Grid()
{   
    foreach (auto player, players) {
        player->setRenderer(nullptr);
        player->stop();
    }
    delete ui;


//    players.clear();
    qDeleteAll(players);
    qDeleteAll(renderers);
    players.clear();
    renderers.clear();
}

void Grid::start()
{
//    BugAV::BugPlayer::setLog();
    for (auto i = 0; i < size; i++) {
        for (auto j = 0; j < size; j++) {
            addPlayer(i, j);
        }
    }
//    kStatistic = startTimer(2000);
//    taskScheduler.start();
}

void Grid::addPlayer(int i, int j)
{
    auto player = new BugAV::BugPlayer(this, false);
    auto renderer = new BugAV::BugGLWidget;
    player->enableSupportFisheye(true);
    QVariantHash avformat;
    avformat["probesize"] = 4096000;
    avformat["analyzeduration"] = 1000000;
    avformat["rtsp_flags"] = "prefer_tcp";
    player->setRenderer(renderer);
    player->setOptionsForFormat(avformat);
    if (i*size + j < files.size()) {
        player->play(files.at(i*size + j));
    }    
    player->setSpeed(64);
    ui->g->addWidget(renderer, i, j);
    players.push_back(player);
    renderers.push_back(renderer);
//        if (i == 0 && j ==0) {
//            player->setSaveRawImage(true);
//        }
}


void Grid::on_reCreateGrid_clicked()
{
    foreach (auto player, players) {
        player->setRenderer(nullptr);
        player->stop();
    }
    for(auto i = 0; i < ui->g->count(); i++) {
        ui->g->removeWidget(ui->g->itemAt(i)->widget());
    }
    qDeleteAll(players);
    qDeleteAll(renderers);
    players.clear();
    renderers.clear();

    start();
}

void Grid::on_create_clicked()
{
    start();
}
