#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QMutex>

class DbManager : public QObject
{
    Q_OBJECT

public:
    static DbManager& instance();
    bool connectToDatabase(const QString &host, int port,
                           const QString &dbName,
                           const QString &user,
                           const QString &password);
    void disconnect();
    QSqlDatabase& database();
    bool isConnected() const;
    bool executeQuery(QSqlQuery &query);

    // 事务封装
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // 获取最近一次错误的描述
    QString lastError() const;

private:
    explicit DbManager(QObject *parent = nullptr);
    ~DbManager();
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    QSqlDatabase m_db;
    QString m_lastError;
    static QMutex s_mutex;
};

#endif // DBMANAGER_H
