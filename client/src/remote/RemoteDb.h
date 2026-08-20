#ifndef REMOTEDB_H
#define REMOTEDB_H

#include <QJsonObject>
#include <QJsonValue>
#include <QVariant>
#include <QString>

// ============================================================
// 业务命令封装（客户端侧）
//   把"命令名 + 参数"的调用收敛成语义化函数，页面只依赖本类。
//   业务命令由服务端持有 SQL 与事务逻辑；客户端不接触数据库。
// ============================================================
class RemoteDb
{
public:
    // QVariant → QJsonValue（供构造事务步骤参数使用）
    static QJsonValue v(const QVariant &val);

    // 构造一个事务步骤 {sql, params, captureAs?}
    static QJsonObject step(const QString &sql, const QJsonObject &params,
                            const QString &captureAs = QString());

    // 底层调用（供 RemoteQuery/RemoteModel 复用）
    static QJsonObject call(const QString &cmd, const QJsonObject &params);

    // ---- 认证 ----
    // 成功时 data 含 {token, userId, employeeId, name, position}
    static QJsonObject login(const QString &employeeId, const QString &password);
    static bool logout();
    static QJsonObject changePassword(const QString &oldPwd, const QString &newPwd);

    // ---- 通用读写（供页面直接使用）----
    static QJsonObject query(const QString &sql, const QJsonObject &binds = QJsonObject());
    static QJsonObject execute(const QString &op, const QString &sql,
                               const QJsonObject &binds = QJsonObject());

    // ---- 数据管理 ----
    // 服务端事务型/级联删除（data.deleteRow），带职位权限校验
    static QJsonObject deleteRow(const QString &table, int id);

    // ---- 通用事务 ----
    // 服务端原子执行一批参数化 SQL 步骤，支持 @captureAs 引用上一步 lastInsertId。
    // steps 元素: { "sql": "...", "params": {...}, "captureAs": "变量名"(可选) }
    // 返回 data: { lastInsertIds: [...], vars: {变量名: 值} }
    static QJsonObject transaction(const QJsonArray &steps);
};

#endif // REMOTEDB_H
