#include "AuthManager.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QSqlQuery>
#include <QDebug>

#include "../db/DbManager.h"

AuthManager& AuthManager::instance()
{
    static AuthManager inst;
    return inst;
}

QString AuthManager::sha256(const QString &text)
{
    QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

QString AuthManager::generateToken() const
{
    const quint64 r1 = QRandomGenerator::global()->generate64();
    const quint64 r2 = QRandomGenerator::global()->generate64();
    return QString::number(r1, 16) + QString::number(r2, 16);
}

QJsonObject AuthManager::login(const QString &employeeId, const QString &password)
{
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT id, employee_id, name, password, position, is_active "
              "FROM t_employee WHERE employee_id = :eid");
    q.bindValue(":eid", employeeId);
    if (!DbManager::instance().executeQuery(q)) {
        return { { "ok", false }, { "error", "数据库查询失败: " + DbManager::instance().lastError() } };
    }
    if (!q.next())
        return { { "ok", false }, { "error", "工号不存在" } };

    const int id = q.value(0).toInt();
    const QString storedPwd = q.value(3).toString();
    const QString position = q.value(4).toString();
    const int active = q.value(5).toInt();

    if (active != 1)
        return { { "ok", false }, { "error", "该账号已被禁用，请联系管理员" } };

    // 支持明文和旧版 SHA256 哈希两种密码格式；明文不匹配时兼容旧版哈希并升级为明文
    if (storedPwd != password) {
        if (storedPwd == sha256(password)) {
            QSqlQuery upd(DbManager::instance().database());
            upd.prepare("UPDATE t_employee SET password = :pwd WHERE id = :id");
            upd.bindValue(":pwd", password);
            upd.bindValue(":id", id);
            DbManager::instance().executeQuery(upd);
        } else {
            return { { "ok", false }, { "error", "密码错误" } };
        }
    }

    const QString token = generateToken();
    SessionInfo si;
    si.userId = id;
    si.employeeId = employeeId;
    si.name = q.value(2).toString();
    si.position = position;
    si.lastActivity = QDateTime::currentMSecsSinceEpoch();
    m_sessions.insert(token, si);

    QJsonObject data;
    data["token"] = token;
    data["userId"] = id;
    data["employeeId"] = employeeId;
    data["name"] = si.name;
    data["position"] = position;
    return { { "ok", true }, { "data", data } };
}

void AuthManager::logout(const QString &token)
{
    if (!token.isEmpty())
        m_sessions.remove(token);
}

const SessionInfo* AuthManager::session(const QString &token) const
{
    if (token.isEmpty())
        return nullptr;
    auto it = m_sessions.find(token);
    if (it == m_sessions.end())
        return nullptr;
    if (QDateTime::currentMSecsSinceEpoch() - it->lastActivity > SESSION_TIMEOUT_MS) {
        m_sessions.erase(it);
        return nullptr;
    }
    return &it.value();
}

void AuthManager::touch(const QString &token)
{
    auto it = m_sessions.find(token);
    if (it != m_sessions.end())
        it->lastActivity = QDateTime::currentMSecsSinceEpoch();
}

bool AuthManager::hasPermission(const QString &position, const QString &menuPath)
{
    // 经理：全开放
    if (position == "经理")
        return true;
    // 员工管理：仅经理可访问
    if (menuPath == "员工管理")
        return false;
    // 库管：关闭 前台业务 / 服务跟踪
    if (position == "库管" && (menuPath == "前台业务" || menuPath == "服务跟踪"))
        return false;
    // 前台 / 客服：关闭 库房管理
    if ((position == "前台" || position == "客服") && menuPath == "库房管理")
        return false;
    return true;
}

bool AuthManager::canDeleteDataTable(const QString &position, const QString &tableName)
{
    if (position == "经理")
        return true;
    if (tableName == "回访记录")
        return (position == "前台" || position == "客服");
    if (position == "前台")
        return tableName == "t_workorder";
    if (position == "库管")
        return tableName == "t_parts";
    return false;
}
