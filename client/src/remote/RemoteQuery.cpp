#include "RemoteQuery.h"
#include "RemoteClient.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSqlError>

RemoteQuery::RemoteQuery()
{
}

void RemoteQuery::prepare(const QString &query)
{
    m_sql = query;
    m_binds.clear();   // 与 QSqlQuery::prepare 一致：重置绑定参数
}

void RemoteQuery::bindValue(const QString &placeholder, const QVariant &val)
{
    m_binds.insert(placeholder, val);
}

static QJsonValue toJsonValue(const QVariant &v)
{
    if (!v.isValid() || v.isNull())
        return QJsonValue(QJsonValue::Null);
    switch (v.type()) {
    case QVariant::Bool:       return v.toBool();
    case QVariant::Int:        return v.toInt();
    case QVariant::UInt:       return static_cast<qint64>(v.toUInt());
    case QVariant::LongLong:   return static_cast<qint64>(v.toLongLong());
    case QVariant::ULongLong:  return static_cast<qint64>(v.toULongLong());
    case QVariant::Double:     return v.toDouble();
    case QVariant::DateTime:   return v.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    case QVariant::Date:       return v.toDate().toString("yyyy-MM-dd");
    case QVariant::Time:       return v.toTime().toString("HH:mm:ss");
    default:                   return v.toString();
    }
}

static QVariant toVariant(const QJsonValue &v)
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
            list.append(toVariant(e));
        return list;
    }
    default: return v.toVariant();
    }
}

bool RemoteQuery::exec(const QString &query)
{
    prepare(query);
    return exec();
}

bool RemoteQuery::exec()
{
    m_pos = -1;
    m_columns.clear();
    m_rows.clear();
    m_lastError = QSqlError();

    if (m_sql.trimmed().isEmpty()) {
        m_lastError = QSqlError("空 SQL", QString(), QSqlError::StatementError);
        return false;
    }

    const QString upper = m_sql.trimmed().toUpper();
    const QString op = upper.startsWith("SELECT") ? "query"
                      : upper.startsWith("INSERT") ? "execute"
                      : upper.startsWith("UPDATE") ? "execute"
                      : upper.startsWith("DELETE") ? "execute"
                      : QString();

    if (op.isEmpty()) {
        m_lastError = QSqlError("不支持的 SQL 语句", m_sql, QSqlError::StatementError);
        return false;
    }

    // 组装绑定参数（键可能带 ":" 前缀，服务端接受两种）
    QJsonObject binds;
    for (auto it = m_binds.constBegin(); it != m_binds.constEnd(); ++it)
        binds.insert(it.key(), toJsonValue(it.value()));

    QJsonObject params;
    params["sql"] = m_sql;
    params["params"] = binds;
    if (op == "execute")
        params["op"] = upper.startsWith("INSERT") ? "insert"
                        : upper.startsWith("UPDATE") ? "update" : "delete";

    const QJsonObject resp = RemoteClient::instance().call(op, params);
    if (!resp.value("ok").toBool()) {
        m_lastError = QSqlError(resp.value("error").toString(), m_sql, QSqlError::StatementError);
        return false;
    }

    const QJsonObject data = resp.value("data").toObject();
    if (op == "query") {
        storeQueryResult(data);
    } else {
        m_lastInsertId = data.value("lastInsertId").toVariant();
        m_numRowsAffected = data.value("numRowsAffected").toInt(-1);
    }
    return true;
}

void RemoteQuery::storeQueryResult(const QJsonObject &data)
{
    const QJsonArray cols = data.value("columns").toArray();
    for (const QJsonValue &c : cols)
        m_columns.append(c.toString());

    const QJsonArray rows = data.value("rows").toArray();
    for (const QJsonValue &rv : rows) {
        const QJsonArray arr = rv.toArray();
        QVariantList row;
        for (const QJsonValue &e : arr)
            row.append(toVariant(e));
        m_rows.append(row);
    }
}

bool RemoteQuery::next()
{
    if (m_pos + 1 < m_rows.size()) {
        ++m_pos;
        return true;
    }
    m_pos = m_rows.size();
    return false;
}

bool RemoteQuery::first()
{
    if (m_rows.isEmpty())
        return false;
    m_pos = 0;
    return true;
}

bool RemoteQuery::seek(int row)
{
    // 允许 row == -1：与 QSqlQuery::seek 一致，表示定位到第一条记录之前，
    // 用于 "q.next() 判空后再回卷重读全部行" 的写法
    if (row < -1 || row >= m_rows.size())
        return false;
    m_pos = row;
    return true;
}

bool RemoteQuery::isNull(int index) const
{
    return index < 0 || index >= m_rows.size() || m_pos < 0 || m_pos >= m_rows.size()
        || !m_rows[m_pos][index].isValid() || m_rows[m_pos][index].isNull();
}

QVariant RemoteQuery::value(int index) const
{
    if (m_pos < 0 || m_pos >= m_rows.size() || index < 0 || index >= m_rows[m_pos].size())
        return QVariant();
    return m_rows[m_pos][index];
}

QVariant RemoteQuery::value(const QString &name) const
{
    const int idx = m_columns.indexOf(name);
    return value(idx);
}

int RemoteQuery::rowCount() const
{
    return m_rows.size();
}

QStringList RemoteQuery::columns() const
{
    return m_columns;
}

QVariant RemoteQuery::lastInsertId() const
{
    return m_lastInsertId;
}

int RemoteQuery::numRowsAffected() const
{
    return m_numRowsAffected;
}

QString RemoteQuery::lastQuery() const
{
    return m_sql;
}

QSqlError RemoteQuery::lastError() const
{
    return m_lastError;
}
