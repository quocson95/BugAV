#ifndef STATCKWIDGET_H
#define STATCKWIDGET_H

#include <QMainWindow>

namespace Ui {
class StatckWidget;
}

class StatckWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit StatckWidget(QWidget *parent = nullptr);
    ~StatckWidget();

private:
    Ui::StatckWidget *ui;
};

#endif // STATCKWIDGET_H
