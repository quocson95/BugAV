
 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>

#include <RenderOuput/ibugavdefaultrenderer.h>

Grid::Grid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Grid)
{
    setWindowTitle("Grid");
    ui->setupUi(this);
//    file = "rtmp://61.28.233.149:1935/live/2bdd1370-e975-4eac-8bb8-12e07802e757_main_1588670960?ci=JDJiZGQxMzcwLWU5NzUtNGVhYy04YmI4LTEyZTA3ODAyZTc1NwIzOAAEbWFpbgN2Y2MbMWJUbmxBWkd4UWNYZ3JzRUFCQzQybUZyNm9mAAA=&sig=cc65438e8";
    file = "rtsp://onvif:Admin123@192.168.0.3:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
    size = 1;

    start();

}

Grid::~Grid()
{
    killTimer(kStatistic);
//    taskScheduler.stop();
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
    auto player = new BugAV::BugPlayer(this);
    auto renderer = new BugAV::BugGLWidget;
    QVariantHash avformat;
    avformat["probesize"] = 4096000;
    avformat["analyzeduration"] = 1000000;
    avformat["rtsp_flags"] = "prefer_tcp";
    player->setRenderer(renderer);
    player->setOptionsForFormat(avformat);
    player->play(file);
    ui->g->addWidget(renderer, i, j);
    players.push_back(player);
    renderers.push_back(renderer);
//        if (i == 0 && j ==0) {
//            player->setSaveRawImage(true);
//        }
}

void Grid::timerEvent(QTimerEvent *event)
{
//    if (event->timerId() == kStatistic) {
//        qDebug() << players.at(0)->statistic();
//    }
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
