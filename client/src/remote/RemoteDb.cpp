#include "RemoteDb.h"
#include "RemoteClient.h"

#include <QJsonValue>
#include <QJsonArray>

QJsonObject RemoteDb::call(const QString &cmd, const QJsonObject &params)
{
    return RemoteClient::instance().call(cmd, params);
}

QJsonValue RemoteDb::v(const QVariant &val)
{
    if (!val.isValid() || val.isNull())
        return QJsonValue(QJsonValue::Null);
    switch (val.type()) {
    case QVariant::Bool:       return val.toBool();
    case QVariant::Int:        return val.toInt();
    case QVariant::UInt:       return static_cast<qint64>(val.toUInt());
    case QVariant::LongLong:   return static_cast<qint64>(val.toLongLong());
    case QVariant::ULongLong:  return static_cast<qint64>(val.toULongLong());
    case QVariant::Double:     return val.toDouble();
    case QVariant::DateTime:   return val.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    case QVariant::Date:       return val.toDate().toString("yyyy-MM-dd");
    case QVariant::Time:       return val.toTime().toString("HH:mm:ss");
    default:                   return val.toString();
    }
}

QJsonObject RemoteDb::login(const QString &employeeId, const QString &password)
{
    QJsonObject params;
    params["employeeId"] = employeeId;
    params["password"] = password;
    return call("auth.login", params);
}

bool RemoteDb::logout()
{
    return call("auth.logout", QJsonObject()).value("ok").toBool();
}

QJsonObject RemoteDb::changePassword(const QString &oldPwd, const QString &newPwd)
{
    QJsonObject params;
    params["oldPwd"] = oldPwd;
    params["newPwd"] = newPwd;
    return call("auth.changePassword", params);
}

QJsonObject RemoteDb::query(const QString &sql, const QJsonObject &binds)
{
    QJsonObject params;
    params["sql"] = sql;
    params["params"] = binds;
    return call("query", params);
}

QJsonObject RemoteDb::execute(const QString &op, const QString &sql, const QJsonObject &binds)
{
    QJsonObject params;
    params["op"] = op;
    params["sql"] = sql;
    params["params"] = binds;
    return call("execute", params);
}

QJsonObject RemoteDb::deleteRow(const QString &table, int id)
{
    QJsonObject params;
    params["table"] = table;
    params["id"] = id;
    return call("data.deleteRow", params);
}

QJsonObject RemoteDb::transaction(const QJsonArray &steps)
{
    QJsonObject params;
    params["steps"] = steps;
    return call("transaction", params);
}

QJsonObject RemoteDb::step(const QString &sql, const QJsonObject &params,
                           const QString &captureAs)
{
    QJsonObject s;
    s["sql"] = sql;
    s["params"] = params;
    if (!captureAs.isEmpty())
        s["captureAs"] = captureAs;
    return s;
}
