
 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>
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
//    files << "/home/sondq/Downloads/video_audio";
//    files << "https://api.dev.vcloudcam.vn/rec/v2/segment/playlist-public/?expire=1603269402&id=d72fdf56dc08a0052920d454ccb1fe0b561fcb59&tk=aabe3cbbda1402b9a4c9cc11bd0cfa8aa09a3e7f&noRedirect=true";
//    files  << "https://api.dev.vcloudcam.vn/rec/v2/segment/playlist-public/?expire=1603269598&id=f0c1c0d70c88aa8540916d661e62c6e058022fea&tk=de612ae9a4c55e929c827adcfb4f45c92a2917b4&noRedirect=true";
    files << "rtsp://admin:Admin123@192.168.0.15:554/0";
//    files << "https://api.dev.vcloudcam.vn/rec/v2/segment/playlist-public/?expire=1603512841&id=162f4148bc4e7a9536ac8e54d189640aa54a75ac&tk=4ca66c973414fd09be9d8c242036e4d29a804e1a&noRedirect=true";
//      files << "rtmp://61.28.227.92:1935/live/84c99a7f-4a3c-40ed-9fe2-50369b247e3b_sub_1603074484?ci=JDg0Yzk5YTdmLTRhM2MtNDBlZC05ZmUyLTUwMzY5YjI0N2UzYgExAANzdWIDdmNjGzFqRzd2ZTloRWdZaVRyWjRiaU1DVWJqWlhDdQAA&sig=cef6d556f";

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

    BugAV::PacketQueue::mustInitOnce();
    start();

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
    auto player = new BugAV::BugPlayer(this, BugAV::ModePlayer::RealTime);
    auto renderer = new BugAV::BugGLWidget;
    player->setPixFmtRGB32(true);
    QVariantHash avformat;
    avformat["probesize"] = 4096;
//    avformat["analyzeduration"] = 1000000;
    avformat["rtsp_flags"] = "prefer_tcp";
    player->setRenderer(renderer);
    player->setOptionsForFormat(avformat);
    if (i*size + j < files.size()) {
        player->play(files.at(i*size + j));
    }    
    player->setSpeed(1);
//    player->seek(509);
//    player->setDisableAudio();
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
//        player->setRenderer(nullptr);
        player->stop();
    }
//    for(auto i = 0; i < ui->g->count(); i++) {
//        ui->g->removeWidget(ui->g->itemAt(i)->widget());
//    }
//    qDeleteAll(players);
//    qDeleteAll(renderers);
//    players.clear();
//    renderers.clear();

//    start();
}

void Grid::on_create_clicked()
{
    foreach (auto player, players) {
//        player->setRenderer(nullptr);
        player->refresh();
    }
}

void Grid::on_slider_valueChanged(int value)
{
    auto d = players[0]->getDuration();
    auto v = (value / 100.0 )* d;
    players[0]->seek(v);
}

void Grid::on_btnSw_clicked()
{
    players[0]->setSpeed(1.0);
}

void Grid::on_btnPause_clicked()
{
    players[0]->pause();
}

void Grid::on_btnFw_clicked()
{
    if (players[0]->isPlaying()) {
        players[0]->setSpeed(8);
    } else {
        players[0]->play();
    }
}

void Grid::on_btnMute_clicked()
{
    players[0]->setMute(!players[0]->isMute());
}
