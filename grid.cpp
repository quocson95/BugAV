#include "bugrenderer.h"
 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>

Grid::Grid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Grid)
{
    setWindowTitle("Grid");
    ui->setupUi(this);
    file = "rtmp://61.28.233.149:1935/live/40892928-6841-4c88-b85f-4875a7488a38_sub_1587436505?ci=JDQwODkyOTI4LTY4NDEtNGM4OC1iODVmLTQ4NzVhNzQ4OGEzOAIzOAADc3ViA3ZjYxsxYkdKUkVNdzhVMFdxOGxnOW5QcVR2eTRDYjkAAA==&sig=5bf1d32f1";
//    file = "rtmp://61.28.231.227:1935/live/756bd856-6774-44eb-9fa7-92b2fcfe1167_sub_1588315651?ci=JDc1NmJkODU2LTY3NzQtNDRlYi05ZmE3LTkyYjJmY2ZlMTE2NwMxMjcAA3N1YgN2Y2MbMWFUbGx4enY0dEtOV2xaZldTelZmU1lGczQ2AAA=&sig=1bdd79324";
    size = 4;

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
    BugAV::BugPlayer::setLog();
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
    auto renderer = new BugAV::BugGLWidget{this};
    QVariantHash avformat;
    avformat["probesize"] = 4096000;
    avformat["analyzeduration"] = 1000000;
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
