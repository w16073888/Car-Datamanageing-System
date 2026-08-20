#include "CommandDispatcher.h"

#include "../auth/AuthManager.h"
#include "../db/DbManager.h"

CommandDispatcher& CommandDispatcher::instance()
{
    static CommandDispatcher inst;
    return inst;
}

void CommandDispatcher::registerHandler(const QString &cmd, Handler handler, bool requiresAuth)
{
    m_handlers.insert(cmd, std::move(handler));
    m_requiresAuth.insert(cmd, requiresAuth);
}

QJsonObject CommandDispatcher::dispatch(const QString &cmd, const QJsonObject &params, const QString &token)
{
    if (!m_handlers.contains(cmd))
        return { { "ok", false }, { "error", "未知命令: " + cmd } };

    const SessionInfo *session = nullptr;
    if (m_requiresAuth.value(cmd, true)) {
        session = AuthManager::instance().session(token);
        if (!session)
            return { { "ok", false }, { "error", "未登录或登录已过期" } };
        AuthManager::instance().touch(token);
    }

    try {
        return m_handlers[cmd](params, session);
    } catch (const std::exception &e) {
        DbManager::instance().rollbackTransaction();
        return { { "ok", false }, { "error", QString::fromUtf8("服务端异常: ") + QString::fromUtf8(e.what()) } };
    }
}
