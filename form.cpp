#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);
    ui->videoLayout->addWidget(&bPlayer);
    // 2land
//    auto file = "rtmp://61.28.231.227:1935/live/756bd856-6774-44eb-9fa7-92b2fcfe1167_sub_1587032712?ci=JDc1NmJkODU2LTY3NzQtNDRlYi05ZmE3LTkyYjJmY2ZlMTE2NwMxMjcAA3N1YgN2Y2MbMWFUbGx4enY0dEtOV2xaZldTelZmU1lGczQ2AAA=&sig=1bdd79324";
    auto file = "rtmp://61.28.233.149:1935/live/40892928-6841-4c88-b85f-4875a7488a38_sub_1587436505?ci=JDQwODkyOTI4LTY4NDEtNGM4OC1iODVmLTQ4NzVhNzQ4OGEzOAIzOAADc3ViA3ZjYxsxYXZRblVEemNJdllGNDVESDlwaGlwMVhjWHIAAA==&sig=1fee34f9a";
//    auto file = "http://api.stg.vcloudcam.vn/rec/v1/segment/playlist/?camUuid=b90ee7b1-b097-455d-9082-5d506676baa1&expire=1587721203&from=1587608384&to=1587612019&token=ac0eb77baaa00c82583286268dd7c68346d830a3&type=hls";
    bPlayer.setFile(file);
    bPlayer.play();
}

Form::~Form()
{
    delete ui;
}

void Form::on_pushButton_clicked()
{
    bPlayer.play();
}

void Form::on_pushButton_2_clicked(bool checked)
{
    bPlayer.pause();
}

void Form::on_pushButton_3_clicked()
{
    bPlayer.stop();
}
