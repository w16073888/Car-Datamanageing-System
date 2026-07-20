#include "DbManager.h"
#include <QSqlDriver>

QMutex DbManager::s_mutex;

DbManager::DbManager(QObject *parent)
    : QObject(parent)
{
}

DbManager::~DbManager()
{
    disconnect();
}

DbManager& DbManager::instance()
{
    static DbManager inst;
    return inst;
}

bool DbManager::connectToDatabase(const QString &host, int port,
                                   const QString &dbName,
                                   const QString &user,
                                   const QString &password)
{
    QMutexLocker locker(&s_mutex);

    if (QSqlDatabase::contains("garage_connection")) {
        m_db = QSqlDatabase::database("garage_connection");
        if (m_db.isOpen()) {
            m_db.close();
        }
    } else {
        m_db = QSqlDatabase::addDatabase("QMYSQL", "garage_connection");
    }

    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);
    m_db.setConnectOptions("MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=5");

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qWarning() << "[DbManager] 数据库连接失败:" << m_lastError;
        return false;
    }

    // 设置连接编码
    QSqlQuery query(m_db);
    query.exec("SET NAMES 'utf8mb4'");
    query.exec("SET CHARACTER SET utf8mb4");

    qDebug() << "[DbManager] 数据库连接成功:" << dbName << "@" << host;
    return true;
}

void DbManager::disconnect()
{
    QMutexLocker locker(&s_mutex);
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (QSqlDatabase::contains("garage_connection")) {
        QSqlDatabase::removeDatabase("garage_connection");
    }
}

QSqlDatabase& DbManager::database()
{
    if (!m_db.isValid() || !m_db.isOpen()) {
        m_db = QSqlDatabase::database("garage_connection");
    }
    return m_db;
}

bool DbManager::isConnected() const
{
    return m_db.isOpen();
}

bool DbManager::executeQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qWarning() << "[DbManager] 查询执行失败:" << m_lastError;
        qWarning() << "[DbManager] SQL:" << query.lastQuery();
        return false;
    }
    return true;
}

bool DbManager::beginTransaction()
{
    QSqlQuery q(m_db);
    return q.exec("START TRANSACTION");
}

bool DbManager::commitTransaction()
{
    QSqlQuery q(m_db);
    return q.exec("COMMIT");
}

bool DbManager::rollbackTransaction()
{
    QSqlQuery q(m_db);
    return q.exec("ROLLBACK");
}

QString DbManager::lastError() const
{
    return m_lastError;
}
