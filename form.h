#ifndef FORM_H
#define FORM_H

#include <QWidget>

#include <BugPlayer/bugplayer.h>
using namespace BugAV;

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked(bool checked);

    void on_pushButton_3_clicked();

private:
    Ui::Form *ui;
    BugPlayer bPlayer{nullptr};
    IBugAVRenderer *renderer;
};

#endif // FORM_H
