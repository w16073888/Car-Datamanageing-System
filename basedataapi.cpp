#include "basedataapi.h"




basedataapi basedataapi::instance;

basedataapi& basedataapi::getInstance(){return instance;}

QString basedataapi::getRootPath(){return m_rootPath;}

int basedataapi::getPermission(){
    return this->Per;
}

bool basedataapi::writeInfo(QString di){
    QString time=QDateTime::currentDateTime().toString();
    QFile text(this->m_rootPath+"/info.txt");
    if (!text.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qDebug() << "文件打开失败：" << text.errorString();
        return 0;
    }
    
    /*QTextStream out(&text);
    out.setCodec("UTF-8");
    out << time+" "+di;*/
    text.write((time+" "+di+"\n").toUtf8());
    text.close();
    return 1;
}

/*
 * executeSqlFile() — 读取 .sql 文件并逐条执行其中的 SQL 语句
 * 参数：
 *   filePath       : SQL 文件的完整路径
 *   connectionName : 数据库连接名称
 *                    - 传入空字符串（默认值）→ 对默认连接执行
 *                    - 传入具体连接名     → 对该命名连接执行
 * 返回：true 表示全部执行成功，false 表示有语句执行失败
 */
bool executeSqlFile(const QString& filePath, const QString& connectionName = "") {
    qDebug() << "[executeSqlFile] 正在读取：" << filePath
             << "  连接：" << (connectionName.isEmpty() ? "default" : connectionName);

    // 打开 SQL 文件
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[executeSqlFile] 无法打开 SQL 文件：" << filePath;
        return false;
    }

    // 读取全部 SQL 内容
    QTextStream in(&file);
    QString sql = in.readAll();
    file.close();

    // 按分号分割为多条 SQL 语句
    QStringList statements = sql.split(';');

    // 根据 connectionName 获取对应的数据库对象，创建查询对象
    QSqlDatabase db = connectionName.isEmpty()
                      ? QSqlDatabase::database()          // 默认连接
                      : QSqlDatabase::database(connectionName); // 命名连接
    QSqlQuery query(db);

    int successCount = 0;
    int failCount = 0;

    for (const QString& stmt : statements) {
        QString trimmedSql = stmt.trimmed();

        // 跳过空语句和纯注释行（-- 开头的行）
        if (trimmedSql.isEmpty() || trimmedSql.startsWith("--")) {
            continue;
        }

        if (!query.exec(trimmedSql)) {
            qDebug() << "[executeSqlFile] 执行失败：" << query.lastError().text();
            qDebug() << "[executeSqlFile] 失败语句：" << trimmedSql;
            failCount++;
        } else {
            successCount++;
        }
    }

    qDebug() << "[executeSqlFile] 完成 —— 成功：" << successCount << "条  失败：" << failCount << "条";
    return failCount == 0;
}

void basedataapi::setPermission(int sta){
    switch(sta){
    case 1:this->Per=1;break;
    case 2:this->Per=2;break;
    case 3:this->Per=3;break;
    case 0:this->Per=0;break;
    default:qDebug()<<"请输入合法（0-3）的权限";
    }
}

basedataapi::basedataapi(){
    this->Per=0;
    inquire_list=new QVector<QVector<QStringList>*>;

}

/*
 * init() — 程序启动时的数据库初始化入口
 *
 * 功能：
 *   1. 确保根目录 D:/Sysdata 存在
 *   2. 创建并打开四个业务数据库的连接：
 *        car.db  — 车辆信息（车牌号、车架号等）
 *        cus.db  — 车主信息（姓名、电话等）
 *        ser.db  — 进店服务信息（维修记录、公里数等）
 *        ware.db — 备件信息（编号、名称、库存等）
 *   3. 对每个数据库执行对应的建表 SQL 文件（如 car.sql），
 *      若表已存在则不会重复创建（CREATE TABLE IF NOT EXISTS）
 *
 * 注意：
 *   - 每个数据库使用独立的"命名连接"（第二个参数），
 *     后续增删查改时需通过对应的连接名来操作
 *   - SQL 建表文件需放在 D:/Sysdata/ 目录下
 */
void basedataapi::init(){
    // 第一步：确保根目录存在
    QDir dir;
    if (!dir.exists(this->getRootPath())) {
        dir.mkpath(this->getRootPath());
        qDebug() << "[init] 已创建根目录：" << this->getRootPath();
    }

    // 第二步：初始化四个业务数据库
    // 配置表：{ 连接名, 数据库文件名, SQL建表文件 }
    const QString DB_CONFIG[4][3] = {
        {"car_connection",  "car.db",  "car.sql"},   // 车辆信息
        {"cus_connection",  "cus.db",  "cus.sql"},   // 车主信息
        {"ser_connection",  "ser.db",  "ser.sql"},   // 进店服务信息
        {"ware_connection", "ware.db", "ware.sql"}    // 备件信息
    };

    for (int i = 0; i < 4; i++) {
        QString connName   = DB_CONFIG[i][0];  // 数据库连接名称（如 "car_connection"）
        QString dbFileName = DB_CONFIG[i][1];  // 数据库文件名（如 "car.db"）
        QString sqlFileName= DB_CONFIG[i][2];  // 建表 SQL 文件名（如 "car.sql"）

        // 创建命名连接（第二个参数指定连接名，用于区分不同数据库）
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        QString fullDbPath = this->getRootPath() + "/" + dbFileName;
        db.setDatabaseName(fullDbPath);

        // 打开数据库（若 .db 文件不存在则自动创建）
        if (!db.open()) {
            qDebug() << "[init]" << dbFileName << "打开失败：" << db.lastError().text();
        } else {
            qDebug() << "[init]" << dbFileName << "连接成功（连接名：" << connName << "）";

            // 执行建表 SQL，指定连接名确保在正确的数据库上执行
            executeSqlFile(this->getRootPath() + "/" + sqlFileName, connName);
        }
    }
}

basedataapi::~basedataapi(){
    int s=0;
    for(QVector<QStringList>* list:*inquire_list){
        delete list;
        s++;
    }
    qDebug()<<s<<"条已经删除";
    delete inquire_list;
    qDebug()<<"inquire_list已经删除";
}


bool basedataapi::save(const QString& name, const QString& depart,
                       const QString& id, const QString& photo, const QString& chara)
{
    if(!(Per>=2)){
        qDebug()<<"当前权限不支持您进行操作！";
        return 0;
    }
    // 检查必填字段
    if (name.isEmpty() || depart.isEmpty() || id.isEmpty()) {
        qDebug() << "保存失败：姓名、部门、工号不能为空";
        return false;
    }


    //  处理图片
    QString savedPhotoPath;
    if (!photo.isEmpty() && QFile::exists(photo)) {
        // 生成目标文件名
        QString targetName = id;
        QString targetPath = m_rootPath + "/photos/" + targetName;

        // 拷贝文件
        if (!QFile::copy(photo,targetPath)) {
            qDebug() << "拷贝图片失败：" << photo << "->" << targetPath;
            return false;
        }

        savedPhotoPath = targetPath;
        qDebug() << "图片已保存到：" << savedPhotoPath;
    } else {
        qDebug() << "未提供图片或图片不存在，仅保存文本信息";
    }

    QString savedcharaPath;

    if (!chara.isEmpty() && QFile::exists(chara)) {

        // 生成目标文件名
        QString targetName1 = id+"_chara";
        QString targetPath1 = m_rootPath + "/charactors/" + targetName1;

        // 拷贝文件
        if (!QFile::copy(chara,targetPath1)) {
            qDebug() << "拷贝特征值失败：" << chara << "->" << targetPath1;
            return false;
        }

        savedcharaPath = targetPath1;
        qDebug() << "特征值已保存到：" << savedcharaPath;
    } else {
        qDebug() << "未提供特征值或特征值不存在，仅保存文本信息";
    }

    // 保存到数据库
    QSqlQuery query;
    query.prepare(
        "INSERT INTO persons (name, department, employee_id, photo_path, charactor_path) "
        "VALUES (:name, :department, :employee_id, :photo_path, :charactor_path)"
    );

    query.bindValue(":name", name);
    query.bindValue(":department", depart);
    query.bindValue(":employee_id", id);
    query.bindValue(":photo_path", savedPhotoPath);
    query.bindValue(":charactor_path", savedcharaPath);

    if (!query.exec()) {
        qDebug() << "插入数据库失败 " << query.lastError().text();
        return false;
    }

    qDebug() << "保存成功";
    qDebug() << "   姓名：" << name;
    qDebug() << "   部门：" << depart;
    qDebug() << "   工号：" << id;
    qDebug() << "   照片：" << (savedPhotoPath.isEmpty() ? "无" : savedPhotoPath);

    return true;
}


QVector<QStringList>* basedataapi::inquireContent(const int model,const QString& cloum){
            //model:1->name,2->depart,3->id
    if(!(Per>=1)){
        qDebug()<<"当前权限不支持您进行操作！";
        return 0;
    }
    QVector<QStringList> *result=new QVector<QStringList>;
    QSqlQuery query;
    switch(model){
        case 1:query.prepare("SELECT name, department, employee_id, photo_path,charactor_path FROM persons WHERE name = :cl");query.bindValue(":cl", cloum);break;
        case 2:query.prepare("SELECT name, department, employee_id, photo_path,charactor_path FROM persons WHERE department = :cl");query.bindValue(":cl", cloum);break;
        case 3:query.prepare("SELECT name, department, employee_id, photo_path,charactor_path FROM persons WHERE employee_id = :cl");query.bindValue(":cl", cloum);break;
        default:qDebug()<<"请输入有效指令";
    }
    query.exec();
    while(query.next()) {
        QStringList temp;
        temp << query.value(0).toString()
               << query.value(1).toString()
               << query.value(2).toString()
               << query.value(3).toString()
               << query.value(4).toString();
        (*result).append(temp);
    }
    inquire_list->append(result);
    return result;
}

QVector<QStringList>* basedataapi::inquireContent(){
    if(!(Per>=1)){
        qDebug()<<"当前权限不支持您进行操作！";
        return 0;
    }

    QVector<QStringList> *result = new QVector<QStringList>;
    QSqlQuery query;
    query.prepare("SELECT name, department, employee_id, photo_path,charactor_path FROM persons");
    if (!query.exec()) {
        qDebug() << "查询失败：" << query.lastError().text();
        delete result;
        return 0;
    }

    while(query.next()) {
        QStringList temp;
        temp << query.value(0).toString()
             << query.value(1).toString()
             << query.value(2).toString()
             << query.value(3).toString()
             << query.value(4).toString();
        (*result).append(temp);
    }

    inquire_list->append(result);
    return result;
}

bool basedataapi::deleteContent(const QString& id){
    if(!(Per>=3)){
        qDebug()<<"当前权限不支持您进行操作！";
        return 0;
    }
    QSqlQuery query;
    query.prepare("DELETE FROM persons WHERE employee_id = :employee_id");
    query.bindValue(":employee_id", id);

    QVector<QStringList> *list=this->inquireContent(3,id);
    for (QStringList employee : *list) {
        qDebug()<<employee[3];
        QFile::remove(employee[3]);
        QFile::remove(employee[4]);
    }

    if (!query.exec()) {
        qDebug() << "删除失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "成功删除工号为 " << id << " 的员工";
    return true;
}

bool basedataapi::update(const QString& preid,const QString& name, const QString& depart,
                         const QString& id, const QString& photo ,const QString& chara){
    bool sta1=this->deleteContent(preid);
    bool sta2=this->save(name,depart,id,photo,chara);
    return sta1&&sta2;
}
