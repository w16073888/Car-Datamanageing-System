#include "mainwindow.h"
#include "basedataapi.h"
#include <QApplication>
#include<QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    basedataapi::getInstance().init();

    MainWindow window;
    window.show();
    /*---------------------------------test Field---------------------------------------*/
       /* basedataapi::getInstance().save("肖筱天", "技术部", "114", basedataapi::getInstance().getRootPath()+"/cathe/DSC_1746.jpg", basedataapi::getInstance().getRootPath()+"/cathe/tt1.jpg");
        basedataapi::getInstance().save("吴滨涵", "技术部", "514", basedataapi::getInstance().getRootPath()+"/cathe/C5783B0849A021614D62FC991B6C741D.jpg", basedataapi::getInstance().getRootPath()+"/cathe/tt2.jpg");
        QVector<QStringList> *list1=basedataapi::getInstance().inquireContent();
        basedataapi::getInstance().update("514","滨涵", "部", "14", basedataapi::getInstance().getRootPath()+"/cathe/C5783B0849A021614D62FC991B6C741D.jpg", basedataapi::getInstance().getRootPath()+"/cathe/tt2.jpg");
        qDebug() << "---------查询开始------------";
        for (QStringList employee : *list1) {
            qDebug() << "员工信息：";
            qDebug() << "  姓名：" << employee[0];
            qDebug() << "  部门：" << employee[1];
            qDebug() << "  工号：" << employee[2];
            qDebug() << "  照片：" << employee[3];
            qDebug() << "  特征值：" << employee[4];
        }
        basedataapi::getInstance().writeInfo("do what?");

*/
        return a.exec();
}
