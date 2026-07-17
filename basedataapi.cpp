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

bool executeSqlFile(const QString& filePath) {
    qDebug()<<filePath;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开 SQL 文件：" << filePath;
        return false;
    }

    QTextStream in(&file);
    QString sql = in.readAll();
    file.close();

    QStringList statements = sql.split(';');
    QSqlQuery query;
    int successCount = 0;
    int failCount = 0;

    for (const QString& sql : statements) {
        QString trimmedSql = sql.trimmed();
        if (trimmedSql.isEmpty()) {
            continue;
        }

        if (!query.exec(trimmedSql)) {
            qDebug() << "   执行 SQL 失败：" << query.lastError().text();
            qDebug() << "   失败的语句：" << trimmedSql;
            failCount++;
        } else {
            successCount++;
        }
    }

    qDebug() << "   成功：" << successCount << "条";
    qDebug() << "   失败：" << failCount << "条";

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

void basedataapi::init(){
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = this->getRootPath() + "/employee.db";
    qDebug() << "数据库Path：" << dbPath;
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "数据库打开失败：" << db.lastError().text();
    }
    qDebug() << "连接成功";

    executeSqlFile(this->getRootPath()+"/person.sql");
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
