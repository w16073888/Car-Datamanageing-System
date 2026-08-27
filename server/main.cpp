#include <QCoreApplication>
#include <QSettings>
#include <QThread>
#include <QSqlQuery>
#include <QJsonObject>
#include <QDebug>

#include "src/db/DbManager.h"
#include "src/auth/AuthManager.h"
#include "src/cmd/CommandDispatcher.h"
#include "src/cmd/QueryCommands.h"
#include "src/cmd/DataDelete.h"
#include "src/net/ServerCore.h"

// ============================================================
// 4s-server 入口
//   1. 读取 config.ini（监听端口 + MySQL 连接参数）
//   2. 连接 MySQL（失败每 30 秒重试，兼容开机时 MySQL 未就绪）
//   3. 注册命令 → 监听 TCP 端口 → 进入事件循环
// ============================================================
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("4s-server");

    const QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cfg.setIniCodec("UTF-8");
#endif

    const quint16 port = static_cast<quint16>(cfg.value("server/port", 9456).toUInt());
    const QString dbHost = cfg.value("mysql/host", "127.0.0.1").toString();
    const int dbPort = cfg.value("mysql/port", 3306).toInt();
    const QString dbName = cfg.value("mysql/db", "garagedb").toString();
    const QString dbUser = cfg.value("mysql/user", "test").toString();
    const QString dbPwd = cfg.value("mysql/password", "test").toString();

    qInfo() << "[server] 使用配置文件:" << iniPath;

    // ---------- 连接 MySQL ----------
    while (!DbManager::instance().connectToDatabase(dbHost, dbPort, dbName, dbUser, dbPwd)) {
        qWarning() << "[server] MySQL 连接失败，30 秒后重试...";
        QThread::msleep(30000);
    }
    qInfo() << "[server] MySQL 连接成功:" << dbName << "@" << dbHost << ":" << dbPort;

    // ---------- 注册命令 ----------
    QueryCommands::registerCommands();
    DataDelete::registerCommands();
    CommandDispatcher &d = CommandDispatcher::instance();

    d.registerHandler("auth.login", [](const QJsonObject &params, const SessionInfo *) {
        return AuthManager::instance().login(
            params.value("employeeId").toString(),
            params.value("password").toString());
    }, false);   // 登录无需已登录 token

    d.registerHandler("auth.changePassword", [](const QJsonObject &params, const SessionInfo *s) {
        if (!s)
            return QJsonObject{ { "ok", false }, { "error", "未登录" } };
        const QString oldPwd = params.value("oldPwd").toString();
        const QString newPwd = params.value("newPwd").toString();
        if (newPwd.isEmpty())
            return QJsonObject{ { "ok", false }, { "error", "新密码不能为空" } };

        QSqlQuery q(DbManager::instance().database());
        q.prepare("SELECT password FROM t_employee WHERE id = :id");
        q.bindValue(":id", s->userId);
        if (!DbManager::instance().executeQuery(q))
            return QJsonObject{ { "ok", false }, { "error", "查询失败: " + DbManager::instance().lastError() } };
        if (!q.next())
            return QJsonObject{ { "ok", false }, { "error", "用户不存在" } };

        const QString stored = q.value(0).toString();
        if (stored != oldPwd && stored != AuthManager::sha256(oldPwd))
            return QJsonObject{ { "ok", false }, { "error", "原密码错误" } };

        QSqlQuery upd(DbManager::instance().database());
        upd.prepare("UPDATE t_employee SET password = :pwd WHERE id = :id");
        upd.bindValue(":pwd", newPwd);
        upd.bindValue(":id", s->userId);
        if (!DbManager::instance().executeQuery(upd))
            return QJsonObject{ { "ok", false }, { "error", "更新失败: " + DbManager::instance().lastError() } };
        return QJsonObject{ { "ok", true }, { "data", QJsonObject() } };
    });

    // ---------- 启动监听 ----------
    ServerCore core;
    QString err;
    if (!core.listen(port, &err)) {
        qCritical() << "[server] 监听端口失败:" << port << err;
        return 1;
    }
    qInfo() << "[server] 4s-server 启动成功，监听端口" << port;

    return app.exec();
}
