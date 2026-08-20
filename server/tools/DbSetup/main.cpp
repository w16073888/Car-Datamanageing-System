#include <QCoreApplication>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

// ============================================================
// DbSetup — 一次性数据库初始化/迁移工具（只运行一次，之后不再使用）
//
//   用法:  DbSetup --init     全新安装：建库 + 执行 sql/init.sql + 记录版本
//          DbSetup --migrate  老库升级：按 sql/migrations/NNN_*.sql 顺序应用
//
//   config.ini 与 4s-server 相同格式（[mysql] host/port/db/user/password）
//   需在 exe 同目录放置 sql/init.sql 与 sql/migrations/，以及 qsqlmysql 驱动。
// ============================================================

static QTextStream gOut(stdout);
static QTextStream gErr(stderr);

// 按行拆分 SQL 语句（语句以 ';' 结尾；忽略 -- 行注释）
static QStringList splitStatements(const QString &sql)
{
    QStringList out;
    QString cur;
    const QStringList lines = sql.split('\n');
    for (QString line : lines) {
        const int ci = line.indexOf("--");
        if (ci >= 0)
            line = line.left(ci);
        cur += line + "\n";
        if (line.trimmed().endsWith(';')) {
            QString s = cur.trimmed();
            if (!s.isEmpty())
                out << s;
            cur.clear();
        }
    }
    if (!cur.trimmed().isEmpty())
        out << cur.trimmed();
    return out;
}

// 读取 sql/migrations/ 中的最大版本号（供 --init 记录）
static int maxMigrationVersion(const QString &migDir)
{
    int maxVer = 0;
    const QStringList files = QDir(migDir).entryList(QStringList() << "*.sql",
                                                     QDir::Files, QDir::Name);
    for (const QString &fn : files) {
        const QRegularExpression re("^(\\d+)_");
        const QRegularExpressionMatch m = re.match(fn);
        if (m.hasMatch())
            maxVer = qMax(maxVer, m.captured(1).toInt());
    }
    return maxVer;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    bool doInit = false, doMigrate = false;
    for (int i = 1; i < argc; i++) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a == "--init") doInit = true;
        else if (a == "--migrate") doMigrate = true;
    }
    if (!doInit && !doMigrate) {
        gErr << "用法: DbSetup --init | --migrate\n"
             << "  --init     全新安装: 建库 + 执行 sql/init.sql + 记录 schema_version\n"
             << "  --migrate  老库升级: 按 sql/migrations/ 序号执行未应用的迁移\n";
        return 1;
    }

    const QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings cfg(iniPath, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    cfg.setIniCodec("UTF-8");
#endif
    const QString host = cfg.value("mysql/host", "127.0.0.1").toString();
    const int port = cfg.value("mysql/port", 3306).toInt();
    const QString db = cfg.value("mysql/db", "garagedb").toString();
    const QString user = cfg.value("mysql/user", "test").toString();
    const QString pwd = cfg.value("mysql/password", "test").toString();

    const QString baseDir = QCoreApplication::applicationDirPath();

    QSqlDatabase conn = QSqlDatabase::addDatabase("QMYSQL", "setup");
    conn.setHostName(host);
    conn.setPort(port);
    // 先以“服务器级”连接（不指定库）：--init 时目标库可能还不存在，
    // 若这里直接选 garagedb，会先报 “Unknown database” 导致建库流程无法执行。
    conn.setDatabaseName(QString());
    conn.setUserName(user);
    conn.setPassword(pwd);
    conn.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");
    if (!conn.open()) {
        gErr << "连接 MySQL 失败: " << conn.lastError().text() << "\n";
        return 1;
    }
    conn.exec("SET NAMES 'utf8mb4'");

    // 执行一个 SQL 文件（按 ';' 拆分为多条语句）
    auto execFile = [&](const QString &path, const QString &desc) -> bool {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            gErr << "无法读取 " << path << "\n";
            return false;
        }
        const QStringList stmts = splitStatements(QString::fromUtf8(f.readAll()));
        int okCount = 0;
        for (const QString &s : stmts) {
            if (s.isEmpty())
                continue;
            QSqlQuery q(conn);
            if (!q.exec(s)) {
                gErr << desc << " 失败: " << q.lastError().text() << "\n"
                     << "  语句: " << s.left(120) << "\n";
                return false;
            }
            okCount++;
        }
        gOut << "  " << desc << " 完成 (" << okCount << " 条语句)\n";
        return true;
    };

    if (doInit) {
        gOut << "==== DbSetup --init ====\n";
        // 1. 建库
        {
            QSqlQuery q(conn);
            q.exec(QString("CREATE DATABASE IF NOT EXISTS `%1` "
                           "DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci").arg(db));
        }
        conn.close();
        conn.setDatabaseName(db);
        if (!conn.open()) {
            gErr << "连接数据库 " << db << " 失败: " << conn.lastError().text() << "\n";
            return 1;
        }
        conn.exec("SET NAMES 'utf8mb4'");

        // 2. 执行 init.sql（会 DROP 并重建全部表 + 初始账号）
        if (!execFile(baseDir + "/sql/init.sql", "init.sql"))
            return 1;

        // 3. 记录 schema_version = 最新迁移号
        const int maxVer = maxMigrationVersion(baseDir + "/sql/migrations");
        conn.exec("CREATE TABLE IF NOT EXISTS schema_version "
                  "(version INT NOT NULL, applied_at DATETIME NOT NULL) "
                  "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
        conn.exec("DELETE FROM schema_version");
        QSqlQuery vi(conn);
        vi.prepare("INSERT INTO schema_version (version, applied_at) VALUES (:v, NOW())");
        vi.bindValue(":v", maxVer);
        vi.exec();
        gOut << "初始化完成。数据库 " << db << " 已建好，schema_version = " << maxVer << "\n";
        gOut << "提示: 初始化只需运行一次；之后日常只需启动 4s-server。\n";
    }

    if (doMigrate) {
        gOut << "==== DbSetup --migrate ====\n";
        // 初始连接为服务器级，迁移前需选中目标库
        conn.close();
        conn.setDatabaseName(db);
        if (!conn.open()) {
            gErr << "连接数据库 " << db << " 失败（数据库尚未初始化？请先运行 DbSetup --init）: "
                 << conn.lastError().text() << "\n";
            return 1;
        }
        conn.exec("SET NAMES 'utf8mb4'");
        conn.exec("CREATE TABLE IF NOT EXISTS schema_version "
                  "(version INT NOT NULL, applied_at DATETIME NOT NULL) "
                  "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

        int current = 0;
        QSqlQuery sv(conn);
        sv.exec("SELECT MAX(version) FROM schema_version");
        if (sv.next())
            current = sv.value(0).toInt();
        gOut << "  当前 schema_version = " << current << "\n";

        const QDir migDir(baseDir + "/sql/migrations");
        const QStringList files = migDir.entryList(QStringList() << "*.sql",
                                                   QDir::Files, QDir::Name);
        int applied = 0;
        for (const QString &fn : files) {
            const QRegularExpression re("^(\\d+)_");
            const QRegularExpressionMatch m = re.match(fn);
            if (!m.hasMatch())
                continue;
            const int ver = m.captured(1).toInt();
            if (ver <= current)
                continue;
            gOut << "  应用迁移 " << fn << " (version " << ver << ")...\n";
            if (!execFile(migDir.filePath(fn), fn)) {
                gErr << "迁移 " << fn << " 失败，已停止。修正后重新运行 DbSetup --migrate。\n";
                return 1;
            }
            QSqlQuery ins(conn);
            ins.prepare("INSERT INTO schema_version (version, applied_at) VALUES (:v, NOW())");
            ins.bindValue(":v", ver);
            ins.exec();
            current = ver;
            applied++;
        }
        gOut << "迁移完成，共应用 " << applied << " 个迁移，当前 schema_version = " << current << "\n";
    }

    return 0;
}
