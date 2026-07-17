#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "所有子控件：" << this->findChildren<QWidget*>();
/*-----------------基础页面信息-----------------------*/
    this->setMinimumSize(1050, 600);
    state=0;
    book = new QStackedWidget(this);
    this->setWindowIcon(QIcon(basedataapi::getInstance().getRootPath()+"/logo_256x256.ico"));//图标
    qDebug() << "文件是否存在：" << QFile::exists(basedataapi::getInstance().getRootPath()+"/logo_256x256.ico");
    qDebug() << "是否为空：" << QIcon(basedataapi::getInstance().getRootPath()+"/logo_256x256.ico").isNull();
    qDebug() << "可用尺寸：" << QIcon(basedataapi::getInstance().getRootPath()+"/logo_256x256.ico").availableSizes();
    this->setWindowTitle("Manager System");
    this->setStyleSheet(
        "QMainWindow {"
        "    background-image: url("+basedataapi::getInstance().getRootPath()+"/BackGround.png);"
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "    background-attachment: fixed;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #5fa88f;"
        "    border: 2px solid #5fa88f;"
        "    border-radius: 2px;"
        "    padding: 14px 28px;;"
        "    font-size: 14px;"
        "    letter-spacing: 2px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #a8e6cf;"
        "    color: #0a0a1a;"
        "}"
        );


    QWidget *page[6];
    QGridLayout *layout[6];
    QLabel *title[6];
    for(int i=0;i<6;i++){
        page[i]=new QWidget;
        layout[i]=new QGridLayout(page[i]);
        book->addWidget(page[i]);


        title[i]=new QLabel(page[i]);

        QPixmap pixmap(basedataapi::getInstance().getRootPath() + "/SysTitle.png");
        if (!pixmap.isNull()) {
            // 将图片缩放到期望大小（比如宽度填满，高度自适应）
            int targetWidth = 1200;  // 或者 page[i] 的宽度
            QPixmap scaledPixmap = pixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation);
            title[i]->setPixmap(scaledPixmap);
            title[i]->setScaledContents(1);  // 缩放
            layout[i]->setRowStretch(0, 1);
        } else {
            title[i]->setText("图片加载失败");
        }
        layout[i]->addWidget(title[i],0,0,1,7);
        layout[i]->setRowStretch(0, 1);         //标题
        layout[i]->setRowStretch(1, 3);         //空
        layout[i]->setRowStretch(2, 2);         //按钮
        layout[i]->setRowStretch(3, 3);         //按钮
    }

    this->opacotyEffect=new QGraphicsOpacityEffect(this);
    this->opacotyEffect->setOpacity(1.0);
    this->fadeAnimation=new QPropertyAnimation(this->opacotyEffect,"opacity");
    this->fadeAnimation->setDuration(600);
    this->fadeAnimation->setStartValue(1.0);
    this->fadeAnimation->setEndValue(0);
    book->setGraphicsEffect(this->opacotyEffect);

/*----------------孩子们这一坨是按钮配置------------------------*/
    Fromcamera = new QPushButton("摄像头对比", page[0]);
    Fromfile = new QPushButton("本地模式", page[0]);
    Manage = new QPushButton("管理员模式", page[0]);
    layout[0]->addWidget(Fromcamera,2,1);
    layout[0]->addWidget(Fromfile,2,3);
    layout[0]->addWidget(Manage,2,5);

    Cut = new QPushButton("截图", page[1]);
    Start = new QPushButton("开始", page[1]);
    Back[1] = new QPushButton("返回", page[1]);
    layout[1]->addWidget(Cut,4,4);
    layout[1]->addWidget(Start,4,5);
    layout[1]->addWidget(Back[1],4,6);

    Savem = new QPushButton("保存员工", page[2]);
    Back[2] = new QPushButton("返回", page[2]);
    layout[2]->addWidget(Savem,4,5);
    layout[2]->addWidget(Back[2],4,6);

    Fileselectpo = new QPushButton("选择图片", page[3]);
    Fileselectre = new QPushButton("选择视频", page[3]);
    Back[3] = new QPushButton("返回", page[3]);
    layout[3]->addWidget(Fileselectpo,2,1);
    layout[3]->addWidget(Fileselectre,2,3);
    layout[3]->addWidget(Back[3],4,6);

    Insure = new QPushButton("确认", page[4]);
    Back[4] = new QPushButton("返回", page[4]);
    layout[4]->addWidget(Insure,4,5);
    layout[4]->addWidget(Back[4],4,6);

    Update = new QPushButton("修改", page[5]);
    Delete = new QPushButton("删除", page[5]);
    Back[5] = new QPushButton("返回", page[5]);
    layout[5]->addWidget(Update,4,4);
    layout[5]->addWidget(Delete,4,5);
    layout[5]->addWidget(Back[5],4,6);

    connect(Insure, &QPushButton::clicked, this,[=]() {
        //附带信息
        setState(5);
    });
    connect(Update, &QPushButton::clicked, this,[=]() {
        //附带信息
        setState(2);
    });
    connect(Cut, &QPushButton::clicked, this,[=]() {
        //附带信息
        setState(2);
    });
    connect(Delete, &QPushButton::clicked, this,[=]() {
        //
    });
    connect(Manage, &QPushButton::clicked, this,[=]() {
        setState(4);
    });
    connect(Fromcamera, &QPushButton::clicked, this,[=]() {
        setState(1);
    });
    connect(Savem, &QPushButton::clicked, this,[=]() {
        setState(0);
    });
    for(int i=1;i<=5;i++){
        connect(Back[i], &QPushButton::clicked, this,[=]() {
            setState(0);
        });
    }


    connect(Fromfile, &QPushButton::clicked, this,[=]() {
        setState(3);
    });


    connect(Fileselectpo, &QPushButton::clicked, this,[=]() {
        //
    });
    connect(Fileselectre, &QPushButton::clicked, this,[=]() {
        //
    });



/*----------------孩子们这一坨每一页编辑更是重量级------------------------*/
    setCentralWidget(book);
    book->setCurrentIndex(0);

}

void MainWindow::setState(int n){
    // 连接动画完成信号：切换页面后再淡入

    connect(fadeAnimation, &QPropertyAnimation::finished, this, [=]() {

        book->setCurrentIndex(n);

        fadeAnimation->setDirection(QAbstractAnimation::Backward);

        fadeAnimation->start();
        disconnect(fadeAnimation, &QPropertyAnimation::finished, nullptr, nullptr);
    });

    fadeAnimation->setDirection(QAbstractAnimation::Forward);  // 正向（淡入）
    fadeAnimation->start();


}

MainWindow::~MainWindow()
{
    delete ui;
}
