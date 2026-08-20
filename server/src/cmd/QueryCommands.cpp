#include "QueryCommands.h"
#include "CommandDispatcher.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QDateTime>
#include <QMap>
#include <QDebug>

#include "../db/DbManager.h"
#include "../auth/AuthManager.h"

// 允许通过通用 execute 写操作的表白名单（16 张业务表；v_parts_stock 是视图，只读）
static const QSet<QString> kWriteTables = {
    "t_employee", "t_vehicle", "t_parts",
    "t_part_instance", "t_part_purchase", "t_workorder",
    "t_workorder_repair_item", "t_technician_work_record",
    "t_workorder_item", "t_quote_item", "t_inventory_log",
    "t_settlement", "t_maintenance_history", "t_vehicle_transaction",
    "t_system_log"
};

// ---------- 审计日志辅助 ----------

// 将绑定参数的值转为可读文本（数字/布尔/对象/数组也能序列化）
static QString jsonValueToText(const QJsonValue &v)
{
    if (v.isString()) return v.toString();
    if (v.isDouble()) return QString::number(v.toDouble());
    if (v.isBool()) return v.toBool() ? "true" : "false";
    if (v.isNull()) return "null";
    if (v.isArray()) return QString::fromUtf8(QJsonDocument(v.toArray()).toJson(QJsonDocument::Compact));
    if (v.isObject()) return QString::fromUtf8(QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
    return QString();
}

// 拼接 SQL 与绑定参数摘要（截断 500 字符）
static QString buildAuditDetail(const QString &sql, const QJsonObject &params)
{
    QString detail = sql;
    if (!params.isEmpty()) {
        QStringList pairs;
        pairs.reserve(params.size());
        for (auto it = params.begin(); it != params.end(); ++it)
            pairs << it.key() + "=" + jsonValueToText(it.value());
        detail += " | 参数: " + pairs.join(", ");
    }
    if (detail.size() > 500)
        detail = detail.left(500) + "…";
    return detail;
}

void QueryCommands::registerCommands()
{
    CommandDispatcher &d = CommandDispatcher::instance();
    // ping：无需登录
    d.registerHandler("ping", [](const QJsonObject &, const SessionInfo *) {
        return QJsonObject{ { "ok", true }, { "data", QJsonObject{ { "pong", true } } } };
    }, false);
    d.registerHandler("query", [](const QJsonObject &params, const SessionInfo *s) {
        return handleQuery(params, s);
    });
    d.registerHandler("execute", [](const QJsonObject &params, const SessionInfo *s) {
        return handleExecute(params, s);
    });
    d.registerHandler("transaction", [](const QJsonObject &params, const SessionInfo *s) {
        return handleTransaction(params, s);
    });
}

// ---------- 值转换 ----------

QVariant QueryCommands::jsonToVariant(const QJsonValue &v)
{
    if (v.isNull() || v.isUndefined())
        return QVariant();
    switch (v.type()) {
    case QJsonValue::Bool:   return v.toBool();
    case QJsonValue::Double: return v.toDouble();
    case QJsonValue::String: return v.toString();
    case QJsonValue::Array: {
        QVariantList list;
        const QJsonArray arr = v.toArray();
        for (const QJsonValue &e : arr)
            list.append(jsonToVariant(e));
        return list;
    }
    default: return v.toVariant();
    }
}

QJsonValue QueryCommands::variantToJson(const QVariant &v)
{
    if (!v.isValid() || v.isNull())
        return QJsonValue(QJsonValue::Null);
    switch (v.type()) {
    case QVariant::Int:        return v.toInt();
    case QVariant::UInt:       return v.toInt();                 // 值域小，转为有符号
    case QVariant::LongLong:   return static_cast<qint64>(v.toLongLong());
    case QVariant::ULongLong:  return static_cast<qint64>(v.toULongLong());
    case QVariant::Double:     return v.toDouble();
    case QVariant::Bool:       return v.toBool();
    case QVariant::DateTime:   return v.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    case QVariant::Date:       return v.toDate().toString("yyyy-MM-dd");
    case QVariant::Time:       return v.toTime().toString("HH:mm:ss");
    default:                   return v.toString();
    }
}

void QueryCommands::bindParams(QSqlQuery &q, const QJsonObject &binds)
{
    for (auto it = binds.begin(); it != binds.end(); ++it) {
        // 统一归一化为带冒号的命名占位符（Qt QMYSQL 驱动对不带冒号的键不生效）
        QString name = it.key();
        if (!name.startsWith(':') && !name.startsWith('?'))
            name.prepend(':');
        q.bindValue(name, jsonToVariant(it.value()));
    }
}

// ---------- query（只读 SELECT） ----------

QJsonObject QueryCommands::handleQuery(const QJsonObject &params, const SessionInfo *)
{
    const QString sql = params.value("sql").toString().trimmed();
    if (sql.isEmpty())
        return { { "ok", false }, { "error", "sql 为空" } };

    const QString upper = sql.toUpper();
    if (!upper.startsWith("SELECT"))
        return { { "ok", false }, { "error", "query 只允许 SELECT 语句" } };
    if (upper.contains("OUTFILE") || upper.contains("LOAD_FILE")
        || upper.contains("INTO OUTFILE"))
        return { { "ok", false }, { "error", "禁止导出文件" } };

    QSqlQuery q(DbManager::instance().database());
    if (!q.prepare(sql))
        return { { "ok", false }, { "error", "SQL 准备失败: " + q.lastError().text() } };

    bindParams(q, params.value("params").toObject());

    if (!DbManager::instance().executeQuery(q))
        return { { "ok", false }, { "error", "查询失败: " + DbManager::instance().lastError() } };

    QJsonObject data;
    QJsonArray columns;
    const QSqlRecord rec = q.record();
    for (int i = 0; i < rec.count(); ++i)
        columns.append(rec.fieldName(i));
    data["columns"] = columns;

    QJsonArray rows;
    while (q.next()) {
        QJsonArray row;
        for (int i = 0; i < rec.count(); ++i)
            row.append(variantToJson(q.value(i)));
        rows.append(row);
    }
    data["rows"] = rows;
    data["rowCount"] = rows.size();
    return { { "ok", true }, { "data", data } };
}

// ---------- 写 SQL 校验（execute 与 transaction 复用） ----------
static bool validateWriteSql(const QString &sql, QString *err)
{
    const QString upper = " " + sql.toUpper() + " ";
    static const QStringList kForbidden = {
        "DROP ", "ALTER ", "CREATE ", "TRUNCATE ", "GRANT ", "REVOKE ",
        "OUTFILE", "LOAD_FILE", "SLEEP(", "BENCHMARK(", "INFORMATION_SCHEMA "
    };
    for (const QString &w : kForbidden) {
        if (upper.contains(w, Qt::CaseInsensitive)) {
            if (err) *err = "SQL 包含被禁止的关键字: " + w.trimmed();
            return false;
        }
    }
    QString t = sql.trimmed();
    if (t.endsWith(";")) t.chop(1);
    if (t.contains(';')) {
        if (err) *err = "禁止多语句执行";
        return false;
    }
    const QString upper2 = sql.toUpper();
    if (upper2.startsWith("UPDATE") && !upper2.contains("WHERE")) {
        if (err) *err = "UPDATE 必须包含 WHERE 条件";
        return false;
    }
    if (upper2.startsWith("DELETE") && !upper2.contains("WHERE")) {
        if (err) *err = "DELETE 必须包含 WHERE 条件";
        return false;
    }
    return true;
}

// ---------- transaction（通用事务：原子执行一批参数化步骤） ----------
QJsonObject QueryCommands::handleTransaction(const QJsonObject &params, const SessionInfo *session)
{
    const QJsonArray steps = params.value("steps").toArray();
    if (steps.isEmpty())
        return { { "ok", false }, { "error", "steps 为空" } };

    DbManager::instance().beginTransaction();

    QMap<QString, QVariant> vars;     // captureAs 捕获的变量
    QJsonArray lastInsertIds;

    for (const QJsonValue &sv : steps) {
        const QJsonObject step = sv.toObject();
        const QString sql = step.value("sql").toString().trimmed();
        if (sql.isEmpty()) {
            DbManager::instance().rollbackTransaction();
            return { { "ok", false }, { "error", "事务步骤含空 SQL" } };
        }

        const QString upper = sql.toUpper();
        QString op;
        if (upper.startsWith("SELECT")) op = "select";
        else if (upper.startsWith("INSERT")) op = "insert";
        else if (upper.startsWith("UPDATE")) op = "update";
        else if (upper.startsWith("DELETE")) op = "delete";
        else {
            DbManager::instance().rollbackTransaction();
            return { { "ok", false }, { "error", "事务步骤只支持 SELECT/INSERT/UPDATE/DELETE" } };
        }

        if (op != "select") {
            QString verr;
            if (!validateWriteSql(sql, &verr)) {
                DbManager::instance().rollbackTransaction();
                return { { "ok", false }, { "error", verr } };
            }
            if (op != "select") {
                QRegularExpression re(
                    op == "delete" ? "DELETE\\s+FROM\\s+([A-Za-z0-9_]+)"
                    : op == "insert" ? "INSERT\\s+INTO\\s+([A-Za-z0-9_]+)"
                                     : "UPDATE\\s+([A-Za-z0-9_]+)",
                    QRegularExpression::CaseInsensitiveOption);
                const QRegularExpressionMatch m = re.match(sql);
                if (!m.hasMatch() || !kWriteTables.contains(m.captured(1))) {
                    DbManager::instance().rollbackTransaction();
                    return { { "ok", false },
                             { "error", "表不在白名单: " + (m.hasMatch() ? m.captured(1) : QString("?")) } };
                }
            }
        }

        // 绑定参数（支持 @captureAs 变量引用）
        QJsonObject binds = step.value("params").toObject();
        QJsonObject finalBinds;
        for (auto it = binds.begin(); it != binds.end(); ++it) {
            const QJsonValue v = it.value();
            if (v.isString() && v.toString().startsWith('@')) {
                const QString varName = v.toString().mid(1);
                if (vars.contains(varName))
                    finalBinds.insert(it.key(), QJsonValue::fromVariant(vars[varName]));
                else
                    finalBinds.insert(it.key(), v);
            } else {
                finalBinds.insert(it.key(), v);
            }
        }

        QSqlQuery q(DbManager::instance().database());
        if (!q.prepare(sql)) {
            DbManager::instance().rollbackTransaction();
            return { { "ok", false }, { "error", "SQL 准备失败: " + q.lastError().text() } };
        }
        bindParams(q, finalBinds);
        if (!DbManager::instance().executeQuery(q)) {
            DbManager::instance().rollbackTransaction();
            return { { "ok", false }, { "error", "事务执行失败: " + DbManager::instance().lastError() } };
        }

        lastInsertIds.append(variantToJson(q.lastInsertId()));
        const QString cap = step.value("captureAs").toString();
        if (!cap.isEmpty())
            vars[cap] = q.lastInsertId();
    }

    DbManager::instance().commitTransaction();

    // 审计日志：记录本次事务中每个写库步骤（best-effort，写失败不影响已提交事务）
    const int opId = session ? session->userId : 0;
    for (int i = 0; i < steps.size(); ++i) {
        const QJsonObject step = steps.at(i).toObject();
        const QString stepSql = step.value("sql").toString().trimmed();
        const QString upper = stepSql.toUpper();
        QString stepOp;
        if (upper.startsWith("INSERT")) stepOp = "insert";
        else if (upper.startsWith("UPDATE")) stepOp = "update";
        else if (upper.startsWith("DELETE")) stepOp = "delete";
        else continue;   // SELECT 等只读步骤不记日志

        QRegularExpression re(
            stepOp == "delete" ? "DELETE\\s+FROM\\s+([A-Za-z0-9_]+)"
            : stepOp == "insert" ? "INSERT\\s+INTO\\s+([A-Za-z0-9_]+)"
                                 : "UPDATE\\s+([A-Za-z0-9_]+)",
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch m = re.match(stepSql);
        const QString stepTable = m.hasMatch() ? m.captured(1) : QString();

        // record_id 仅对 INSERT 步骤取 lastInsertId；其余置 NULL
        QVariant stepRid;
        if (stepOp == "insert" && i < lastInsertIds.size())
            stepRid = lastInsertIds.at(i).toVariant().toLongLong();

        DbManager::writeAuditLog(opId, stepOp, stepTable, stepRid,
                                 buildAuditDetail(stepSql, step.value("params").toObject()));
    }

    QJsonObject data;
    data["lastInsertIds"] = lastInsertIds;
    QJsonObject varsObj;
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it)
        varsObj.insert(it.key(), variantToJson(it.value()));
    data["vars"] = varsObj;
    return { { "ok", true }, { "data", data } };
}

// ---------- execute（单条 DML） ----------

QJsonObject QueryCommands::handleExecute(const QJsonObject &params, const SessionInfo *session)
{
    const QString op = params.value("op").toString().toLower();
    const QString sql = params.value("sql").toString().trimmed();
    if (sql.isEmpty())
        return { { "ok", false }, { "error", "sql 为空" } };

    const QString upper = sql.toUpper();

    // op 与 SQL 首关键字匹配
    bool startOk = false;
    if (op == "insert") startOk = upper.startsWith("INSERT");
    else if (op == "update") startOk = upper.startsWith("UPDATE");
    else if (op == "delete") startOk = upper.startsWith("DELETE");
    else return { { "ok", false }, { "error", "op 必须为 insert/update/delete" } };
    if (!startOk)
        return { { "ok", false }, { "error", "SQL 与 op 不匹配" } };

    // 禁止多语句（去掉末尾分号后再检查）
    QString t = sql.trimmed();
    if (t.endsWith(";")) t.chop(1);
    if (t.contains(';'))
        return { { "ok", false }, { "error", "禁止多语句执行" } };

    // 危险关键字黑名单
    static const QStringList kForbidden = {
        "DROP ", "ALTER ", "CREATE ", "TRUNCATE ", "GRANT ", "REVOKE ",
        "OUTFILE", "LOAD_FILE", "SLEEP(", "BENCHMARK(", "INFORMATION_SCHEMA "
    };
    const QString up2 = " " + upper + " ";
    for (const QString &w : kForbidden) {
        if (up2.contains(w, Qt::CaseInsensitive))
            return { { "ok", false }, { "error", "SQL 包含被禁止的关键字: " + w.trimmed() } };
    }

    // 提取并校验目标表名（白名单）
    QRegularExpression re(
        op == "delete" ? "DELETE\\s+FROM\\s+([A-Za-z0-9_]+)"
        : op == "insert" ? "INSERT\\s+INTO\\s+([A-Za-z0-9_]+)"
                         : "UPDATE\\s+([A-Za-z0-9_]+)",
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(sql);
    if (!m.hasMatch())
        return { { "ok", false }, { "error", "无法识别目标表" } };
    const QString table = m.captured(1);
    if (!kWriteTables.contains(table))
        return { { "ok", false }, { "error", "表不在白名单: " + table } };

    // UPDATE/DELETE 必须带 WHERE，防止全表误改/误删
    if (op == "update" || op == "delete") {
        if (!upper.contains("WHERE"))
            return { { "ok", false }, { "error", "UPDATE/DELETE 必须包含 WHERE 条件" } };
    }

    QSqlQuery q(DbManager::instance().database());
    if (!q.prepare(sql))
        return { { "ok", false }, { "error", "SQL 准备失败: " + q.lastError().text() } };

    bindParams(q, params.value("params").toObject());

    if (!DbManager::instance().executeQuery(q))
        return { { "ok", false }, { "error", "执行失败: " + DbManager::instance().lastError() } };

    // 审计日志：记录本次写库操作（best-effort）
    QVariant rid;
    if (op == "insert")
        rid = q.lastInsertId().toLongLong();
    DbManager::writeAuditLog(session ? session->userId : 0, op, table, rid,
                             buildAuditDetail(sql, params.value("params").toObject()));

    QJsonObject data;
    if (op == "insert")
        data["lastInsertId"] = q.lastInsertId().toInt();
    data["numRowsAffected"] = q.numRowsAffected();
    return { { "ok", true }, { "data", data } };
}
