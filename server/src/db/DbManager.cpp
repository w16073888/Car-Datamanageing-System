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
    m_db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qWarning() << "[DbManager] 数据库连接失败:" << m_lastError;
        return false;
    }

    // 设置连接编码。
    // 注意：只用 SET NAMES 'utf8mb4'（同时设置 client/connection/results 三处字符集）。
    // 不要追加 SET CHARACTER SET utf8mb4 —— 它会按 MySQL 文档把 character_set_connection
    // 重置为数据库默认字符集（character_set_database）。若数据库默认是 latin1，连接层的
    // character_set_connection 会退回 latin1，中文（含 ENUM 值）被转成 '?' 导致写入报错
    // "Data truncated for column ..."。DbSetup 建库时应显式指定 DEFAULT CHARACTER SET utf8mb4。
    QSqlQuery query(m_db);
    // 必须同时固定 collation_connection：
    // Qt QMYSQL 驱动绑定字符串参数时固定使用 utf8mb4 的默认 collation
    // (utf8mb4_general_ci)，而 DATE_FORMAT() 等函数结果的 collation 跟随
    // collation_connection。若不显式固定，当服务端把 utf8mb4 默认 collation 配成
    // utf8mb4_unicode_ci（如 my.ini 设 collation-server=utf8mb4_unicode_ci）时，
    // "DATE_FORMAT(...) = :param" 这类比较会报
    // "Illegal mix of collations (utf8mb4_unicode_ci,COERCIBLE) and (utf8mb4_general_ci,COERCIBLE)"。
    query.exec("SET NAMES 'utf8mb4' COLLATE 'utf8mb4_general_ci'");

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

void DbManager::writeAuditLog(int operatorId, const QString &actionType,
                              const QString &table, const QVariant &recordId,
                              const QString &detail)
{
    QSqlQuery q(DbManager::instance().database());
    q.prepare("INSERT INTO t_system_log (operator_id, action_type, table_name, record_id, detail) "
              "VALUES (:op, :act, :tbl, :rid, :detail)");
    q.bindValue(":op", operatorId);
    q.bindValue(":act", actionType);
    q.bindValue(":tbl", table);
    q.bindValue(":rid", recordId);
    q.bindValue(":detail", detail);
    if (!DbManager::instance().executeQuery(q))
        qWarning() << "[AuditLog] 写入失败:" << q.lastError().text();
}

QString DbManager::lastError() const
{
    return m_lastError;
}
