#ifndef REMOTEQUERY_H
#define REMOTEQUERY_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QSqlError>

// ============================================================
// 仿 QSqlQuery 的远程查询缓冲类
//   页面原写法:
//     QSqlQuery q(DbManager::instance().database());
//     q.prepare(sql); q.bindValue(":x", v);
//     DbManager::instance().executeQuery(q);
//     while (q.next()) { q.value(i); }
//   迁移后写法:
//     RemoteQuery q;
//     q.prepare(sql); q.bindValue(":x", v);
//     q.exec();
//     while (q.next()) { q.value(i); }
//   内部按 SQL 首关键字自动分派到服务端 query / execute 命令。
// ============================================================
class RemoteQuery
{
public:
    RemoteQuery();

    void prepare(const QString &query);
    void bindValue(const QString &placeholder, const QVariant &val);

    // 执行：自动分派 query/execute；成功返回 true
    bool exec();
    bool exec(const QString &query);   // 便捷：等价 prepare(query) + exec()

    // 结果遍历
    bool next();
    bool first();
    bool seek(int row);
    bool isNull(int index) const;
    QVariant value(int index) const;
    QVariant value(const QString &name) const;
    int rowCount() const;
    QStringList columns() const;

    QVariant lastInsertId() const;
    int numRowsAffected() const;
    QString lastQuery() const;
    QSqlError lastError() const;

private:
    void storeQueryResult(const QJsonObject &data);

    QString m_sql;
    QMap<QString, QVariant> m_binds;
    QStringList m_columns;
    QList<QVariantList> m_rows;
    int m_pos = -1;
    QVariant m_lastInsertId;
    int m_numRowsAffected = -1;
    QSqlError m_lastError;
};

#endif // REMOTEQUERY_H
