#include "basedataapi.h"




basedataapi basedataapi::instance;

basedataapi& basedataapi::getInstance(){return instance;}

QString basedataapi::getRootPath(){return m_rootPath;}

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
 *
 * 处理方式：逐行读取，跳过纯注释行（-- 开头）和空行，
 * 然后将剩余行拼接后按分号分割、逐条执行。
 * 这样可以避免"整块以一个注释开头导致整条语句被误跳过"的问题。
 *
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

    // 逐行读取，过滤掉纯注释行和空行
    // 注意：-- 开头的行是 SQL 注释，不应参与执行
    QString cleanSql;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("--")) {
            continue;   // 跳过空行和注释行
        }
        cleanSql += line + "\n";
    }
    file.close();

    // 按分号分割为多条 SQL 语句
    QStringList statements = cleanSql.split(';');

    // 根据 connectionName 获取对应的数据库对象，创建查询对象
    QSqlDatabase db = connectionName.isEmpty()
                      ? QSqlDatabase::database()          // 默认连接
                      : QSqlDatabase::database(connectionName); // 命名连接
    QSqlQuery query(db);

    int successCount = 0;
    int failCount = 0;

    for (const QString& stmt : statements) {
        QString trimmedSql = stmt.trimmed();

        // 跳过空语句
        if (trimmedSql.isEmpty()) {
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

basedataapi::basedataapi(){
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


/* ============================================================
 *  车辆信息 — car.db（表名 vehicles）
 *  字段：customer_id(车主号) / license_plate(车牌号) / vin(车架号) / engine_number(发动机号)
 *        purchase_date(购车日期) / inspection_date(年审日期) / insurance_date(保险日期)
 *  主键：license_plate
 * ============================================================ */

bool basedataapi::saveCar(int customer_id, const QString& license_plate, const QString& vin,
                           const QString& engine_number, const QString& purchase_date,
                           const QString& inspection_date, const QString& insurance_date)
{
    // 获取 car_connection 对应的数据库对象并创建查询
    QSqlDatabase db = QSqlDatabase::database("car_connection");
    QSqlQuery query(db);
    // 准备 INSERT 语句，绑定所有字段
    query.prepare(
        "INSERT INTO vehicles (customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date) "
        "VALUES (:cid, :lp, :vin, :en, :pd, :id, :ins)"
    );
    query.bindValue(":cid", customer_id);
    query.bindValue(":lp", license_plate);
    query.bindValue(":vin", vin);
    query.bindValue(":en", engine_number);
    query.bindValue(":pd", purchase_date);
    query.bindValue(":id", inspection_date);
    query.bindValue(":ins", insurance_date);

    if (!query.exec()) {
        qDebug() << "[saveCar] 插入失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[saveCar] 成功 —— 车牌号：" << license_plate;
    return true;
}

bool basedataapi::deleteCar(const QString& license_plate)
{
    QSqlDatabase db = QSqlDatabase::database("car_connection");
    QSqlQuery query(db);
    query.prepare("DELETE FROM vehicles WHERE license_plate = :lp");
    query.bindValue(":lp", license_plate);

    if (!query.exec()) {
        qDebug() << "[deleteCar] 删除失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[deleteCar] 成功 —— 车牌号：" << license_plate;
    return true;
}

QVector<QStringList>* basedataapi::inquireCar(int model, const QString& value)
{
    QSqlDatabase db = QSqlDatabase::database("car_connection");
    QSqlQuery query(db);

    // model: 1=车牌号, 2=车架号, 3=发动机号
    switch (model) {
    case 1:
        query.prepare("SELECT customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date FROM vehicles WHERE license_plate = :v");
        query.bindValue(":v", value);
        break;
    case 2:
        query.prepare("SELECT customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date FROM vehicles WHERE vin = :v");
        query.bindValue(":v", value);
        break;
    case 3:
        query.prepare("SELECT customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date FROM vehicles WHERE engine_number = :v");
        query.bindValue(":v", value);
        break;
    default:
        qDebug() << "[inquireCar] 无效查询模式：" << model << "（1=车牌号, 2=车架号, 3=发动机号）";
        return nullptr;
    }

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireCar] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // customer_id
            << query.value(1).toString()  // license_plate
            << query.value(2).toString()  // vin
            << query.value(3).toString()  // engine_number
            << query.value(4).toString()  // purchase_date
            << query.value(5).toString()  // inspection_date
            << query.value(6).toString(); // insurance_date
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

QVector<QStringList>* basedataapi::inquireCar()
{
    QSqlDatabase db = QSqlDatabase::database("car_connection");
    QSqlQuery query(db);
    query.prepare("SELECT customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date FROM vehicles");

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireCar] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // customer_id
            << query.value(1).toString()  // license_plate
            << query.value(2).toString()  // vin
            << query.value(3).toString()  // engine_number
            << query.value(4).toString()  // purchase_date
            << query.value(5).toString()  // inspection_date
            << query.value(6).toString(); // insurance_date
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

bool basedataapi::updateCar(const QString& old_license_plate,
                             int customer_id,
                             const QString& license_plate, const QString& vin,
                             const QString& engine_number, const QString& purchase_date,
                             const QString& inspection_date, const QString& insurance_date)
{
    // 使用 UPDATE 语句，以原车牌号为 WHERE 条件进行更新
    QSqlDatabase db = QSqlDatabase::database("car_connection");
    QSqlQuery query(db);
    query.prepare(
        "UPDATE vehicles SET customer_id=:cid, license_plate=:lp, vin=:vin, engine_number=:en, "
        "purchase_date=:pd, inspection_date=:id, insurance_date=:ins "
        "WHERE license_plate=:old_lp"
    );
    query.bindValue(":cid", customer_id);
    query.bindValue(":old_lp", old_license_plate);
    query.bindValue(":lp", license_plate);
    query.bindValue(":vin", vin);
    query.bindValue(":en", engine_number);
    query.bindValue(":pd", purchase_date);
    query.bindValue(":id", inspection_date);
    query.bindValue(":ins", insurance_date);

    if (!query.exec()) {
        qDebug() << "[updateCar] 更新失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[updateCar] 成功 —— 原车牌号：" << old_license_plate;
    return true;
}


/* ============================================================
 *  车主信息 — cus.db（表名 customers）
 *  字段：id(自增主键) / owner_name(车主姓名) / owner_phone(车主电话)
 * ============================================================ */

int basedataapi::saveCus(const QString& owner_name, const QString& owner_phone)
{
    QSqlDatabase db = QSqlDatabase::database("cus_connection");
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO customers (owner_name, owner_phone) "
        "VALUES (:on, :op)"
    );
    query.bindValue(":on", owner_name);
    query.bindValue(":op", owner_phone);

    if (!query.exec()) {
        qDebug() << "[saveCus] 插入失败：" << query.lastError().text();
        return -1;
    }
    int newId = query.lastInsertId().toInt();
    qDebug() << "[saveCus] 成功 —— ID:" << newId << "车主姓名：" << owner_name;
    return newId;
}

bool basedataapi::deleteCus(int id)
{
    QSqlDatabase db = QSqlDatabase::database("cus_connection");
    QSqlQuery query(db);
    query.prepare("DELETE FROM customers WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "[deleteCus] 删除失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[deleteCus] 成功 —— ID：" << id;
    return true;
}

QVector<QStringList>* basedataapi::inquireCus(int model, const QString& value)
{
    QSqlDatabase db = QSqlDatabase::database("cus_connection");
    QSqlQuery query(db);

    // model: 1=车主姓名, 2=车主电话
    switch (model) {
    case 1:
        query.prepare("SELECT id, owner_name, owner_phone FROM customers WHERE owner_name = :v");
        query.bindValue(":v", value);
        break;
    case 2:
        query.prepare("SELECT id, owner_name, owner_phone FROM customers WHERE owner_phone = :v");
        query.bindValue(":v", value);
        break;
    default:
        qDebug() << "[inquireCus] 无效查询模式：" << model << "（1=车主姓名, 2=车主电话）";
        return nullptr;
    }

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireCus] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // id
            << query.value(1).toString()  // owner_name
            << query.value(2).toString(); // owner_phone
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

QVector<QStringList>* basedataapi::inquireCus()
{
    QSqlDatabase db = QSqlDatabase::database("cus_connection");
    QSqlQuery query(db);
    query.prepare("SELECT id, owner_name, owner_phone FROM customers");

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireCus] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // id
            << query.value(1).toString()  // owner_name
            << query.value(2).toString(); // owner_phone
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

bool basedataapi::updateCus(int id,
                             const QString& owner_name, const QString& owner_phone)
{
    QSqlDatabase db = QSqlDatabase::database("cus_connection");
    QSqlQuery query(db);
    query.prepare(
        "UPDATE customers SET owner_name=:on, owner_phone=:op WHERE id=:id"
    );
    query.bindValue(":id", id);
    query.bindValue(":on", owner_name);
    query.bindValue(":op", owner_phone);

    if (!query.exec()) {
        qDebug() << "[updateCus] 更新失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[updateCus] 成功 —— ID：" << id;
    return true;
}


/* ============================================================
 *  进店服务信息 — ser.db（表名 services）
 *  字段：id(自增主键) / repair_person(维修责任人) / repair_content(报修内容)
 *        mileage(公里数) / labor_cost(工时费)
 *        driver_name(驾驶员姓名) / driver_phone(驾驶员电话)
 *        is_settled(是否结算) / report_time(报修时间)
 * ============================================================ */

bool basedataapi::saveSer(const QString& repair_person, const QString& repair_content,
                           int mileage, double labor_cost,
                           const QString& driver_name, const QString& driver_phone,
                           int is_settled, const QString& report_time)
{
    QSqlDatabase db = QSqlDatabase::database("ser_connection");
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO services (repair_person, repair_content, mileage, labor_cost, "
        "driver_name, driver_phone, is_settled, report_time) "
        "VALUES (:rp, :rc, :mi, :lc, :dn, :dp, :is, :rt)"
    );
    query.bindValue(":rp", repair_person);
    query.bindValue(":rc", repair_content);
    query.bindValue(":mi", mileage);
    query.bindValue(":lc", labor_cost);
    query.bindValue(":dn", driver_name);
    query.bindValue(":dp", driver_phone);
    query.bindValue(":is", is_settled);
    query.bindValue(":rt", report_time);

    if (!query.exec()) {
        qDebug() << "[saveSer] 插入失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[saveSer] 成功 —— 维修责任人：" << repair_person;
    return true;
}

bool basedataapi::deleteSer(int id)
{
    QSqlDatabase db = QSqlDatabase::database("ser_connection");
    QSqlQuery query(db);
    query.prepare("DELETE FROM services WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "[deleteSer] 删除失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[deleteSer] 成功 —— ID：" << id;
    return true;
}

QVector<QStringList>* basedataapi::inquireSer(int model, const QString& value)
{
    QSqlDatabase db = QSqlDatabase::database("ser_connection");
    QSqlQuery query(db);

    // model: 1=ID, 2=维修责任人
    switch (model) {
    case 1:
        query.prepare("SELECT id, repair_person, repair_content, mileage, labor_cost, "
                      "driver_name, driver_phone, is_settled, report_time FROM services WHERE id = :v");
        query.bindValue(":v", value.toInt());
        break;
    case 2:
        query.prepare("SELECT id, repair_person, repair_content, mileage, labor_cost, "
                      "driver_name, driver_phone, is_settled, report_time FROM services WHERE repair_person = :v");
        query.bindValue(":v", value);
        break;
    default:
        qDebug() << "[inquireSer] 无效查询模式：" << model << "（1=ID, 2=维修责任人）";
        return nullptr;
    }

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireSer] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // id
            << query.value(1).toString()  // repair_person
            << query.value(2).toString()  // repair_content
            << query.value(3).toString()  // mileage
            << query.value(4).toString()  // labor_cost
            << query.value(5).toString()  // driver_name
            << query.value(6).toString()  // driver_phone
            << query.value(7).toString()  // is_settled
            << query.value(8).toString(); // report_time
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

QVector<QStringList>* basedataapi::inquireSer()
{
    QSqlDatabase db = QSqlDatabase::database("ser_connection");
    QSqlQuery query(db);
    query.prepare("SELECT id, repair_person, repair_content, mileage, labor_cost, "
                  "driver_name, driver_phone, is_settled, report_time FROM services");

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireSer] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // id
            << query.value(1).toString()  // repair_person
            << query.value(2).toString()  // repair_content
            << query.value(3).toString()  // mileage
            << query.value(4).toString()  // labor_cost
            << query.value(5).toString()  // driver_name
            << query.value(6).toString()  // driver_phone
            << query.value(7).toString()  // is_settled
            << query.value(8).toString(); // report_time
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

bool basedataapi::updateSer(int id,
                             const QString& repair_person, const QString& repair_content,
                             int mileage, double labor_cost,
                             const QString& driver_name, const QString& driver_phone,
                             int is_settled, const QString& report_time)
{
    QSqlDatabase db = QSqlDatabase::database("ser_connection");
    QSqlQuery query(db);
    query.prepare(
        "UPDATE services SET repair_person=:rp, repair_content=:rc, "
        "mileage=:mi, labor_cost=:lc, driver_name=:dn, driver_phone=:dp, "
        "is_settled=:is, report_time=:rt WHERE id=:id"
    );
    query.bindValue(":id", id);
    query.bindValue(":rp", repair_person);
    query.bindValue(":rc", repair_content);
    query.bindValue(":mi", mileage);
    query.bindValue(":lc", labor_cost);
    query.bindValue(":dn", driver_name);
    query.bindValue(":dp", driver_phone);
    query.bindValue(":is", is_settled);
    query.bindValue(":rt", report_time);

    if (!query.exec()) {
        qDebug() << "[updateSer] 更新失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[updateSer] 成功 —— ID：" << id;
    return true;
}


/* ============================================================
 *  备件信息 — ware.db（表名 parts）
 *  字段：part_id(进货号) / part_code(备件编号) / name(名称) / quantity(数量)
 *        purchase_price(进货价) / sale_price(销售价) / supplier(供货商)
 *        out_date(出库日期) / warranty_period(质保期)
 *  主键：part_id
 * ============================================================ */

bool basedataapi::saveWare(const QString& part_id, const QString& part_code, const QString& name,
                            int quantity, double purchase_price, double sale_price,
                            const QString& supplier, const QString& out_date,
                            const QString& warranty_period)
{
    QSqlDatabase db = QSqlDatabase::database("ware_connection");
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO parts (part_id, part_code, name, quantity, purchase_price, sale_price, "
        "supplier, out_date, warranty_period) "
        "VALUES (:pid, :pc, :nm, :qt, :pp, :sp, :su, :od, :wp)"
    );
    query.bindValue(":pid", part_id);
    query.bindValue(":pc", part_code);
    query.bindValue(":nm", name);
    query.bindValue(":qt", quantity);
    query.bindValue(":pp", purchase_price);
    query.bindValue(":sp", sale_price);
    query.bindValue(":su", supplier);
    query.bindValue(":od", out_date);
    query.bindValue(":wp", warranty_period);

    if (!query.exec()) {
        qDebug() << "[saveWare] 插入失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[saveWare] 成功 —— 备件编号：" << part_id;
    return true;
}

bool basedataapi::deleteWare(const QString& part_id)
{
    QSqlDatabase db = QSqlDatabase::database("ware_connection");
    QSqlQuery query(db);
    query.prepare("DELETE FROM parts WHERE part_id = :pid");
    query.bindValue(":pid", part_id);

    if (!query.exec()) {
        qDebug() << "[deleteWare] 删除失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[deleteWare] 成功 —— 备件编号：" << part_id;
    return true;
}

QVector<QStringList>* basedataapi::inquireWare(int model, const QString& value)
{
    QSqlDatabase db = QSqlDatabase::database("ware_connection");
    QSqlQuery query(db);

    // model: 1=进货号, 2=名称
    switch (model) {
    case 1:
        query.prepare("SELECT part_id, part_code, name, quantity, purchase_price, sale_price, "
                      "supplier, out_date, warranty_period FROM parts WHERE part_id = :v");
        query.bindValue(":v", value);
        break;
    case 2:
        query.prepare("SELECT part_id, part_code, name, quantity, purchase_price, sale_price, "
                      "supplier, out_date, warranty_period FROM parts WHERE name = :v");
        query.bindValue(":v", value);
        break;
    default:
        qDebug() << "[inquireWare] 无效查询模式：" << model << "（1=进货号, 2=名称）";
        return nullptr;
    }

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireWare] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // part_id
            << query.value(1).toString()  // part_code
            << query.value(2).toString()  // name
            << query.value(3).toString()  // quantity（int 转为 QString）
            << query.value(4).toString()  // purchase_price（double 转为 QString）
            << query.value(5).toString()  // sale_price（double 转为 QString）
            << query.value(6).toString()  // supplier
            << query.value(7).toString()  // out_date
            << query.value(8).toString(); // warranty_period
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

QVector<QStringList>* basedataapi::inquireWare()
{
    QSqlDatabase db = QSqlDatabase::database("ware_connection");
    QSqlQuery query(db);
    query.prepare("SELECT part_id, part_code, name, quantity, purchase_price, sale_price, "
                  "supplier, out_date, warranty_period FROM parts");

    QVector<QStringList>* result = new QVector<QStringList>;
    if (!query.exec()) {
        qDebug() << "[inquireWare] 查询失败：" << query.lastError().text();
        inquire_list->append(result);
        return result;
    }
    while (query.next()) {
        QStringList row;
        row << query.value(0).toString()  // part_id
            << query.value(1).toString()  // part_code
            << query.value(2).toString()  // name
            << query.value(3).toString()  // quantity
            << query.value(4).toString()  // purchase_price
            << query.value(5).toString()  // sale_price
            << query.value(6).toString()  // supplier
            << query.value(7).toString()  // out_date
            << query.value(8).toString(); // warranty_period
        result->append(row);
    }
    inquire_list->append(result);
    return result;
}

bool basedataapi::updateWare(const QString& old_part_id,
                              const QString& part_id, const QString& part_code, const QString& name,
                              int quantity, double purchase_price, double sale_price,
                              const QString& supplier, const QString& out_date,
                              const QString& warranty_period)
{
    QSqlDatabase db = QSqlDatabase::database("ware_connection");
    QSqlQuery query(db);
    query.prepare(
        "UPDATE parts SET part_id=:pid, part_code=:pc, name=:nm, quantity=:qt, "
        "purchase_price=:pp, sale_price=:sp, supplier=:su, "
        "out_date=:od, warranty_period=:wp WHERE part_id=:old_pid"
    );
    query.bindValue(":old_pid", old_part_id);
    query.bindValue(":pid", part_id);
    query.bindValue(":pc", part_code);
    query.bindValue(":nm", name);
    query.bindValue(":qt", quantity);
    query.bindValue(":pp", purchase_price);
    query.bindValue(":sp", sale_price);
    query.bindValue(":su", supplier);
    query.bindValue(":od", out_date);
    query.bindValue(":wp", warranty_period);

    if (!query.exec()) {
        qDebug() << "[updateWare] 更新失败：" << query.lastError().text();
        return false;
    }
    qDebug() << "[updateWare] 成功 —— 原备件编号：" << old_part_id;
    return true;
}
