#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "basedataapi.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    QPushButton *Fromcamera;
    QPushButton *Fromfile;
    QPushButton *Manage;
    QPushButton *Savem;
    QPushButton *Fileselectpo;
    QPushButton *Fileselectre;

    QPushButton *Cut;
    QPushButton *Start;


    QPushButton *Insure;

    QPushButton *Update;
    QPushButton *Delete;

    QPushButton *Back[6];

    void setState(int n);
    ~MainWindow();



private:
    int state;
            /*控制当前窗口是什么：
              0：选择模式
              1：摄像头模式
              2：(管理员)保存员工
              3：本地文件模式
              4：管理员模式
              5：员工编辑
              */
    Ui::MainWindow *ui;
    QStackedWidget *book;
    QPropertyAnimation *fadeAnimation;
    QGraphicsOpacityEffect *opacotyEffect;
};

#endif // MAINWINDOW_H
