#ifndef GRID_H
#define GRID_H

#include "taskscheduler.h"

#include <QWidget>

#include <BugPlayer/bugplayer.h>

#include <RenderOuput/bugglwidget.h>


namespace Ui {
class Grid;
}

class Grid : public QWidget
{
    Q_OBJECT

public:
    explicit Grid(QWidget *parent = nullptr);
    ~Grid();
    void start();
private:
    void addPlayer(int i, int j);
private:
    Ui::Grid *ui;

    QString file;
    QVector<BugAV::BugPlayer *> players;
    QVector<BugAV::IBugAVRenderer *> renderers;
//    BugAV::TaskScheduler taskS`cheduler;
    int size;
    int kStatistic;
    // QObject interface
protected:
    void timerEvent(QTimerEvent *event);
private slots:
    void on_reCreateGrid_clicked();
    void on_create_clicked();
};

#endif // GRID_H
