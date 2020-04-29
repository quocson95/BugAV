 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>

Grid::Grid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Grid)
{
    setWindowTitle("Grid");
    ui->setupUi(this);
    file = "rtmp://61.28.233.70:1935/live/40892928-6841-4c88-b85f-4875a7488a38_sub_1587436505?ci=JDQwODkyOTI4LTY4NDEtNGM4OC1iODVmLTQ4NzVhNzQ4OGEzOAIzOAADc3ViA3ZjYxsxYjdzN01CUWo5SGhHVmtrb283RldjRVNWYjgAAA==&sig=8e51ddcfd";
//    file = "rtmp://61.28.233.149:1935/live/c3a8b94b-9e0c-444e-9e2f-b72937586cb7_sub_1587971873?ci=JGMzYThiOTRiLT";
    size = 3;

    start();

}

Grid::~Grid()
{
    killTimer(kStatistic);
//    taskScheduler.stop();
    BugAV::BugPlayer::stopTaskScheduler();
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
    player->setRenderer(renderer);
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
