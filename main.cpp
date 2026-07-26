#include "mainwindow.h"
#include "src/dialogs/LoginDialog.h"
#include "src/database/DbManager.h"
#include "src/database/Session.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft YaHei", 10));

    // ============================================================
    // 初始化数据库连接
    // 请根据实际MySQL配置修改以下参数
    // ============================================================
    if (!DbManager::instance().connectToDatabase(
            "127.0.0.1",    // MySQL主机
            3306,           // 端口
            "garagedb",     // 数据库名称（需先通过sql/init.sql创建）
            "test",         // 用户名
            "test"          // 密码
        )) {
        QMessageBox::critical(nullptr, "数据库连接失败",
            "无法连接到MySQL数据库！\n\n"
            "请确保：\n"
            "1. MySQL 8.0 服务已启动\n"
            "2. 已通过 sql/init.sql 创建 garagedb 数据库\n"
            "3. 连接参数正确\n\n"
            "错误信息：" + DbManager::instance().lastError());
        return -1;
    }

    // ============================================================
    // 显示登录对话框
    // ============================================================
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        // 用户取消登录，退出程序
        qDebug() << "用户取消登录，程序退出";
        return 0;
    }

    // ============================================================
    // 登录成功，显示主窗口
    // ============================================================
    MainWindow window;
    window.show();

    return a.exec();
}
