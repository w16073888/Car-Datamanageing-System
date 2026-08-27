#ifndef QUERYCOMMANDS_H
#define QUERYCOMMANDS_H

#include <QJsonObject>
#include <QVariant>

struct SessionInfo;   // 前向声明（auth/AuthManager.h 定义）

// ============================================================
// 通用读写命令：query / execute
//   客户端发送"参数化 SQL 模板 + 绑定参数"（而非 SQL 直连）。
//   - query   : 仅允许 SELECT，prepared statement 绑定参数防注入，返回 {columns, rows}
//   - execute : 仅允许单条 INSERT/UPDATE/DELETE，表名白名单 + 危险关键字黑名单 + 强制 WHERE
// ============================================================
class QueryCommands
{
public:
    static void registerCommands();

    static QJsonObject handleQuery(const QJsonObject &params, const SessionInfo *session);
    static QJsonObject handleExecute(const QJsonObject &params, const SessionInfo *session);
    // 通用事务：原子执行一批参数化 SQL 步骤，支持 @captureAs 引用上一步 lastInsertId
    static QJsonObject handleTransaction(const QJsonObject &params, const SessionInfo *session);

    // 辅助：绑定参数 / 值转换
    static void bindParams(class QSqlQuery &q, const QJsonObject &binds);
    static QVariant jsonToVariant(const QJsonValue &v);
    static QJsonValue variantToJson(const QVariant &v);
};

#endif // QUERYCOMMANDS_H
