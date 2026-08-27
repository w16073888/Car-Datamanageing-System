#include "mainwindow.h"
#include "src/dialogs/LoginDialog.h"
#include "src/database/Session.h"
#include "src/remote/RemoteClient.h"
#include "src/remote/RemoteDb.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFont>
#include <QSettings>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft YaHei", 10));

    // ============================================================
    // 读取配置，连接数据服务（4s-server）
    //   数据库建表/迁移已交给服务器端一次性工具 DbSetup，客户端不再执行迁移。
    // ============================================================
    const QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cfg.setIniCodec("UTF-8");
#endif
    const QString serverHost = cfg.value("server/host", "127.0.0.1").toString();
    const quint16 serverPort = static_cast<quint16>(cfg.value("server/port", 9456).toUInt());

    if (!RemoteClient::instance().connectToServer(serverHost, serverPort)) {
        QMessageBox::critical(nullptr, "无法连接数据服务",
            "无法连接到数据服务（4s-server）！\n\n"
            "请确认：\n"
            "1. 服务器电脑上的 4s-server 已启动\n"
            "2. 网络可访问服务器 IP 与端口\n"
            "3. config.ini 中 server/host、server/port 配置正确\n\n"
            "错误信息：" + RemoteClient::instance().lastError());
        return -1;
    }

    // ============================================================
    // 显示登录对话框（登录走 4s-server 的 auth.login）
    // ============================================================
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        RemoteDb::logout();
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
