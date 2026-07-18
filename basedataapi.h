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

/*
 * ============================================================
 *  basedataapi — 汽修公司数据记录及管理系统 数据层核心类
 *
 *  本类管理四个 SQLite 数据库，每个数据库对应一个业务模块：
 *    car_connection → car.db  车辆信息
 *    cus_connection → cus.db  车主信息
 *    ser_connection → ser.db  进店服务信息
 *    ware_connection → ware.db 备件信息
 *
 *  使用方式：
 *    basedataapi::getInstance().init();        // 程序启动时初始化
 *    basedataapi::getInstance().saveCar(...);  // 增删查改
 * ============================================================
 */

class basedataapi
{
private:
    QVector<QVector<QStringList>*> *inquire_list;   // 查询结果指针列表（析构时自动清理）
    const QString m_rootPath = "D:/Sysdata";         // 数据根目录
    static basedataapi instance;                     // 单例对象
    basedataapi();                                   // 私有构造（单例模式）
    ~basedataapi();                                  // 析构时清理 inquire_list

public:
    /* ==================== 基础方法 ==================== */
    static basedataapi& getInstance();   // 获取单例
    QString getRootPath();               // 返回根目录路径 "D:/Sysdata"
    void init();                         // 初始化四个数据库连接并建表

    /* ============================================================
     *  车辆信息 — car.db（表名 vehicles）
     *  字段：customer_id(车主号) / license_plate(车牌号) / vin(车架号) / engine_number(发动机号)
     *        purchase_date(购车日期) / inspection_date(年审日期) / insurance_date(保险日期)
     *  主键：license_plate
     * ============================================================ */
    bool saveCar(int customer_id, const QString& license_plate, const QString& vin,
                 const QString& engine_number, const QString& purchase_date,
                 const QString& inspection_date, const QString& insurance_date);
    bool deleteCar(const QString& license_plate);
    QVector<QStringList>* inquireCar(int model, const QString& value);  // model: 1=车牌号, 2=车架号, 3=发动机号
    QVector<QStringList>* inquireCar();                                  // 查询所有车辆
    bool updateCar(const QString& old_license_plate,
                   int customer_id, const QString& license_plate, const QString& vin,
                   const QString& engine_number, const QString& purchase_date,
                   const QString& inspection_date, const QString& insurance_date);

    /* ============================================================
     *  车主信息 — cus.db（表名 customers）
     *  字段：id(自增主键) / owner_name(车主姓名) / owner_phone(车主电话)
     * ============================================================ */
    int saveCus(const QString& owner_name, const QString& owner_phone);
    bool deleteCus(int id);
    QVector<QStringList>* inquireCus(int model, const QString& value);  // model: 1=车主姓名, 2=车主电话
    QVector<QStringList>* inquireCus();                                  // 查询所有车主
    bool updateCus(int id,
                   const QString& owner_name, const QString& owner_phone);

    /* ============================================================
     *  进店服务信息 — ser.db（表名 services）
     *  字段：id(自增主键) / repair_person(维修责任人) / repair_content(报修内容)
     *        mileage(公里数) / labor_cost(工时费)
     *        driver_name(驾驶员姓名) / driver_phone(驾驶员电话)
     *        is_settled(是否结算) / report_time(报修时间)
     * ============================================================ */
    bool saveSer(const QString& repair_person, const QString& repair_content,
                 int mileage, double labor_cost,
                 const QString& driver_name, const QString& driver_phone,
                 int is_settled, const QString& report_time);
    bool deleteSer(int id);
    QVector<QStringList>* inquireSer(int model, const QString& value);  // model: 1=ID, 2=维修责任人
    QVector<QStringList>* inquireSer();                                  // 查询所有服务记录
    bool updateSer(int id,
                   const QString& repair_person, const QString& repair_content,
                   int mileage, double labor_cost,
                   const QString& driver_name, const QString& driver_phone,
                   int is_settled, const QString& report_time);

    /* ============================================================
     *  备件信息 — ware.db（表名 parts）
     *  字段：part_id(进货号) / part_code(备件编号) / name(名称) / quantity(数量)
     *        purchase_price(进货价) / sale_price(销售价) / supplier(供货商)
     *        out_date(出库日期) / warranty_period(质保期)
     *  主键：part_id
     * ============================================================ */
    bool saveWare(const QString& part_id, const QString& part_code, const QString& name,
                  int quantity, double purchase_price, double sale_price,
                  const QString& supplier, const QString& out_date,
                  const QString& warranty_period);
    bool deleteWare(const QString& part_id);
    QVector<QStringList>* inquireWare(int model, const QString& value); // model: 1=进货号, 2=名称
    QVector<QStringList>* inquireWare();                                 // 查询所有备件
    bool updateWare(const QString& old_part_id,
                    const QString& part_id, const QString& part_code, const QString& name,
                    int quantity, double purchase_price, double sale_price,
                    const QString& supplier, const QString& out_date,
                    const QString& warranty_period);

    /* ==================== 日志记录 ==================== */
    bool writeInfo(QString di);   // 记录操作日志到 info.txt
};

#endif // BASEDATAAPI_H
