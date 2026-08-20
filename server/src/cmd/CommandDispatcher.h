#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <QHash>
#include <QJsonObject>
#include <functional>

struct SessionInfo;

// ============================================================
// 命令分发器（服务端单例）
//   命令注册表：cmd → 处理函数。requiresAuth=true 的命令需携带有效 token。
//   统一 try/catch，异常时回滚当前事务并返回错误。
// ============================================================
class CommandDispatcher
{
public:
    using Handler = std::function<QJsonObject(const QJsonObject &params, const SessionInfo *session)>;

    static CommandDispatcher& instance();

    void registerHandler(const QString &cmd, Handler handler, bool requiresAuth = true);

    // 返回内部响应对象 {ok, data|error}；由上层补 id 字段
    QJsonObject dispatch(const QString &cmd, const QJsonObject &params, const QString &token);

private:
    CommandDispatcher() = default;

    QHash<QString, Handler> m_handlers;
    QHash<QString, bool> m_requiresAuth;
};

#endif // COMMANDDISPATCHER_H
