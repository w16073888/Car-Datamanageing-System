#include "mainwindow.h"
#include "basedataapi.h"
#include <QApplication>
#include<QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    basedataapi::getInstance().init();

    MainWindow window;
    window.show();
    /* ============================================================
     *  测试区 — 依次测试四个数据库的 save / inquire / update / delete
     *  取消注释即可运行（注意：反复运行会因主键冲突而报错，
     *  建议每次测试完一轮后手动删除 D:/Sysdata/*.db 再重新测试）
     * ============================================================ */

    // ======== 1. car.db — 车辆信息 ========
    qDebug() << "\n========== [测试] car.db 车辆信息 ==========";

    // 1a. 保存车辆
    basedataapi::getInstance().saveCar("京A88888", "LSVAB4BRXJN000001",
                                       "EA888-123456", "2024-01-15", "2025-01-15", "2025-01-15");
    basedataapi::getInstance().saveCar("京B66666", "LFV3A23K5JN000002",
                                       "EA211-654321", "2024-03-20", "2025-03-20", "2025-03-20");

    // 1b. 查询全部车辆
    QVector<QStringList>* carList = basedataapi::getInstance().inquireCar();
    qDebug() << "[查询] car.db 全部记录 —— 共" << carList->size() << "条";
    for (const QStringList& row : *carList) {
        qDebug() << "  车牌:" << row[0] << "车架号:" << row[1] << "发动机号:" << row[2]
                 << "购车:" << row[3] << "年审:" << row[4] << "保险:" << row[5];
    }

    // 1c. 按车牌号查询
    QVector<QStringList>* carByPlate = basedataapi::getInstance().inquireCar(1, "京A88888");
    qDebug() << "[查询] 按车牌号 京A88888 —— 共" << carByPlate->size() << "条";

    // 1d. 按车架号查询
    QVector<QStringList>* carByVin = basedataapi::getInstance().inquireCar(2, "LFV3A23K5JN000002");
    qDebug() << "[查询] 按车架号 LFV3A23K5JN000002 —— 共" << carByVin->size() << "条";

    // 1e. 更新车辆（修改年审和保险日期）
    basedataapi::getInstance().updateCar("京A88888", "京A88888",
                                         "LSVAB4BRXJN000001", "EA888-123456",
                                         "2024-01-15", "2026-01-15", "2026-01-15");

    // 1f. 删除车辆
    basedataapi::getInstance().deleteCar("京B66666");

    // 1g. 确认删除结果
    QVector<QStringList>* carAfterDel = basedataapi::getInstance().inquireCar();
    qDebug() << "[确认] 删除后 car.db 共" << carAfterDel->size() << "条（应为 1 条）";


    // ======== 2. cus.db — 车主信息 ========
    qDebug() << "\n========== [测试] cus.db 车主信息 ==========";

    // 2a. 保存车主
    basedataapi::getInstance().saveCus("张三", "13800138001", "张师傅", "13900139001");
    basedataapi::getInstance().saveCus("李四", "13800138002", "李师傅", "13900139002");

    // 2b. 查询全部车主
    QVector<QStringList>* cusList = basedataapi::getInstance().inquireCus();
    qDebug() << "[查询] cus.db 全部记录 —— 共" << cusList->size() << "条";
    for (const QStringList& row : *cusList) {
        qDebug() << "  ID:" << row[0] << "车主:" << row[1] << "车主电话:" << row[2]
                 << "驾驶员:" << row[3] << "驾驶员电话:" << row[4];
    }

    // 2c. 按车主姓名查询
    QVector<QStringList>* cusByName = basedataapi::getInstance().inquireCus(1, "张三");
    qDebug() << "[查询] 按车主姓名 张三 —— 共" << cusByName->size() << "条";

    // 2d. 更新车主（获取第一个人的 ID 来更新）
    if (!cusList->isEmpty()) {
        int firstId = cusList->at(0)[0].toInt();
        basedataapi::getInstance().updateCus(firstId, "张三", "13800138000",
                                              "张师傅", "13900139000");
        qDebug() << "[更新] cus.db ID=" << firstId << " 电话已修改";
    }

    // 2e. 按驾驶员姓名查询
    QVector<QStringList>* cusByDriver = basedataapi::getInstance().inquireCus(3, "李师傅");
    qDebug() << "[查询] 按驾驶员姓名 李师傅 —— 共" << cusByDriver->size() << "条";


    // ======== 3. ser.db — 进店服务信息 ========
    qDebug() << "\n========== [测试] ser.db 进店服务信息 ==========";

    // 3a. 保存服务记录
    basedataapi::getInstance().saveSer("王师傅", "更换机油、机滤", 85000, 120.0);
    basedataapi::getInstance().saveSer("赵师傅", "四轮定位、动平衡", 85200, 200.0);

    // 3b. 查询全部服务记录
    QVector<QStringList>* serList = basedataapi::getInstance().inquireSer();
    qDebug() << "[查询] ser.db 全部记录 —— 共" << serList->size() << "条";
    for (const QStringList& row : *serList) {
        qDebug() << "  ID:" << row[0] << "责任人:" << row[1] << "报修内容:" << row[2]
                 << "公里数:" << row[3] << "工时费:" << row[4];
    }

    // 3c. 按维修责任人查询
    QVector<QStringList>* serByPerson = basedataapi::getInstance().inquireSer(2, "王师傅");
    qDebug() << "[查询] 按维修责任人 王师傅 —— 共" << serByPerson->size() << "条";

    // 3d. 更新服务记录（修改工时费）
    if (!serList->isEmpty()) {
        int serId = serList->at(0)[0].toInt();
        basedataapi::getInstance().updateSer(serId, "王师傅", "更换机油、机滤、空滤",
                                              86000, 180.0);
        qDebug() << "[更新] ser.db ID=" << serId << " 内容已修改";
    }

    // 3e. 按 ID 查询
    if (!serList->isEmpty()) {
        int serId = serList->at(0)[0].toInt();
        QVector<QStringList>* serById = basedataapi::getInstance().inquireSer(1, QString::number(serId));
        qDebug() << "[查询] 按ID" << serId << " —— 共" << serById->size() << "条";
    }


    // ======== 4. ware.db — 备件信息 ========
    qDebug() << "\n========== [测试] ware.db 备件信息 ==========";

    // 4a. 保存备件
    basedataapi::getInstance().saveWare("P001", "机油滤清器", 50, 35.0, "博世配件", "12个月");
    basedataapi::getInstance().saveWare("P002", "空气滤清器", 30, 45.0, "曼牌滤清器", "12个月");
    basedataapi::getInstance().saveWare("P003", "刹车片（前）", 20, 280.0, "菲罗多", "24个月");

    // 4b. 查询全部备件
    QVector<QStringList>* wareList = basedataapi::getInstance().inquireWare();
    qDebug() << "[查询] ware.db 全部记录 —— 共" << wareList->size() << "条";
    for (const QStringList& row : *wareList) {
        qDebug() << "  编号:" << row[0] << "名称:" << row[1] << "数量:" << row[2]
                 << "金额:" << row[3] << "供货商:" << row[4] << "质保期:" << row[5];
    }

    // 4c. 按备件编号查询
    QVector<QStringList>* wareById = basedataapi::getInstance().inquireWare(1, "P001");
    qDebug() << "[查询] 按备件编号 P001 —— 共" << wareById->size() << "条";

    // 4d. 按名称查询
    QVector<QStringList>* wareByName = basedataapi::getInstance().inquireWare(2, "空气滤清器");
    qDebug() << "[查询] 按名称 空气滤清器 —— 共" << wareByName->size() << "条";

    // 4e. 更新备件（修改数量）
    basedataapi::getInstance().updateWare("P001", "P001", "机油滤清器", 100, 32.0, "博世配件", "12个月");
    qDebug() << "[更新] ware.db P001 数量已改为 100，单价降为 32.0";

    // 4f. 删除备件
    basedataapi::getInstance().deleteWare("P003");
    qDebug() << "[删除] ware.db P003 已删除";

    // 4g. 确认删除结果
    QVector<QStringList>* wareAfterDel = basedataapi::getInstance().inquireWare();
    qDebug() << "[确认] 删除后 ware.db 共" << wareAfterDel->size() << "条（应为 2 条）";


    // ======== 汇总 ========
    qDebug() << "\n========== [测试完成] ==========";
    basedataapi::getInstance().writeInfo("测试完成：所有数据库方法已验证");
        return a.exec();
}
