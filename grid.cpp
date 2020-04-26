 #include "grid.h"
#include "ui_grid.h"

#include <BugPlayer/bugplayer.h>

Grid::Grid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Grid)
{
    ui->setupUi(this);
    file = "rtmp://61.28.231.227:1935/live/756bd856-6774-44eb-9fa7-92b2fcfe1167_sub_1587032712?ci=JDc1NmJkODU2LTY3NzQtNDRlYi05ZmE3LTkyYjJmY2ZlMTE2NwMxMjcAA3N1YgN2Y2MbMWFUbGx4enY0dEtOV2xaZldTelZmU1lGczQ2AAA=&sig=1bdd79324";
//    file = "rtmp://61.28.233.149:1935/live/40892928-6841-4c88-b85f-4875a7488a38_sub_1587436505?ci=JDQwODkyOTI4LTY4NDEtNGM4OC1iODVmLTQ4NzVhNzQ4OGEzOAIzOAADc3ViA3ZjYxsxYjFlVFhRcjNwV1VvOTNrOVFWVlVpMVhBTDAAAA==&sig=9c2af4836";
    size = 7;


    start();

}

Grid::~Grid()
{
//    taskScheduler.stop();
    BugAV::BugPlayer::stopTaskScheduler();
    foreach (auto player, players) {
//        auto r = player->getRenderer();
        player->setRenderer(nullptr);
        player->stop();
//        player->deleteLater();
    }
    delete ui;


//    players.clear();
    qDeleteAll(players);
    qDeleteAll(renderers);
}

void Grid::start()
{
    BugAV::BugPlayer::setLog();
    for (auto i = 0; i < size; i++) {
        for (auto j = 0; j < size; j++) {
            addPlayer(i, j);
        }
    }
//    taskScheduler.start();
}

void Grid::addPlayer(int i, int j)
{
    auto player = new BugAV::BugPlayer(this);
    auto renderer = new BugAV::BugGLWidget{this};
    player->setRenderer(renderer);
    player->play(file);
    ui->gridLayout->addWidget(renderer, i, j);
    players.push_back(player);
    renderers.push_back(renderer);
}
