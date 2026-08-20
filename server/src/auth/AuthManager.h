#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QHash>
#include <QString>
#include <QJsonObject>

// 登录会话信息
struct SessionInfo
{
    int userId = 0;
    QString employeeId;
    QString name;
    QString position;
    qint64 lastActivity = 0;   // QDateTime::currentMSecsSinceEpoch()
};

// ============================================================
// 认证与会话管理（服务端单例）
//   登录成功生成随机 token，会话保存在内存；过期自动清理。
//   职位权限逻辑移植自原客户端 Session.cpp。
// ============================================================
class AuthManager
{
public:
    static AuthManager& instance();

    // 登录：校验 t_employee，成功返回 token 与用户信息，失败返回 ok=false
    QJsonObject login(const QString &employeeId, const QString &password);
    void logout(const QString &token);

    // 按 token 取会话；过期或不存在返回 nullptr
    const SessionInfo* session(const QString &token) const;
    void touch(const QString &token);

    // 职位权限（移植自 Session::hasPermission / canDeleteDataTable）
    static bool hasPermission(const QString &position, const QString &menuPath);
    static bool canDeleteDataTable(const QString &position, const QString &tableName);

    static QString sha256(const QString &text);

private:
    AuthManager() = default;
    QString generateToken() const;

    mutable QHash<QString, SessionInfo> m_sessions;   // mutable：const session() 里也要清理过期项
    static const qint64 SESSION_TIMEOUT_MS = 3600 * 1000;   // 1 小时无活动过期
};

#endif // AUTHMANAGER_H
