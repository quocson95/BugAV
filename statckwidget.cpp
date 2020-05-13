#include "statckwidget.h"
#include "ui_statckwidget.h"

StatckWidget::StatckWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StatckWidget)
{
    ui->setupUi(this);
}

StatckWidget::~StatckWidget()
{
    delete ui;
}
