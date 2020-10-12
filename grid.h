#ifndef GRID_H
#define GRID_H

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
    QVector<QString> files;
    QVector<BugAV::BugPlayer *> players;
    QVector<BugAV::IBugAVRenderer *> renderers;
    int size;
private slots:
    void on_reCreateGrid_clicked();
    void on_create_clicked();    
    void on_slider_valueChanged(int value);
    void on_btnSw_clicked();
    void on_btnPause_clicked();
    void on_btnFw_clicked();
};

#endif // GRID_H
