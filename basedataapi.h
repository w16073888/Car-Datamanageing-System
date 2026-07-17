#ifndef BASEDATAAPI_H
#define BASEDATAAPI_H

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QString>
#include <QVector>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDateTime>
#include <QStringList>
#include <QTextStream>
#include <QSqlDatabase>

//#include <QCoreApplication>



/*
 * 引入该库之后需要调用void setPermission(bool sta);设置权限，默认客户端，
 * 还需要在Appication对象创建之后手动init()
 * */


class basedataapi
{
private:
    QVector<QVector<QStringList>*> *inquire_list;
    const QString m_rootPath="D:/Sysdata";
    static basedataapi instance;
    basedataapi();
    ~basedataapi();
    static bool created;
    int Per;
public:
/*-------------------以下为功能函数，拿来给你调用的，前面的别动，也用不到-----------------*/
    int getPermission();
    void init();
              //在主函数创建application之后运行
    static basedataapi& getInstance();
              //懒汉单例

    QString getRootPath();
              //返回"D:/Sysdata"

    void setPermission(int sta);
              //设置权限，0为defaul，啥都不能干
              //1能查询数据
              //2还能保存数据
              //3还能更新和删除数据

    bool save(const QString& name, const QString& depart,
              const QString& id, const QString& photo ,const QString& chara);
              //保存信息没什么好说的，photo传绝对路径，要带文件名

    bool deleteContent(const QString& id);
              //考虑到其他要素都有可能重复，所以只提供id删除工具，图片和数据库一并删除

    QVector<QStringList>* inquireContent(const int model,const QString& cloum);
              //model:1->name,2->depart,3->id，使用完毕不需要手动delete了嘿嘿，没有符合要求会返回空vector
    QVector<QStringList>* inquireContent();
              //重载，返回所有参数

    bool update(const QString& preid,const QString& name, const QString& depart,
                const QString& id, const QString& photoo ,const QString& chara);
              //更新功能，参数列表和save一模一样，功能是将preid的人信息全面改为当前信息，也需要保证cathe里面有相应的新图片

    bool writeInfo(QString di);
              //日志，记录修改,包括修改时间（自动读取）和修改内容（参数传递），写入成功返回1否则返回0

};

#endif // BASEDATAAPI_H
