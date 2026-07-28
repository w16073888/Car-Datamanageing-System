#include "mainwindow.h"
#include "src/dialogs/LoginDialog.h"
#include "src/database/DbManager.h"
#include "src/database/Session.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft YaHei", 10));

    // ============================================================
    // 初始化数据库连接
    // 请根据实际MySQL配置修改以下参数
    // ============================================================
    if (!DbManager::instance().connectToDatabase(
            "127.0.0.1",    // MySQL主机
            3306,           // 端口
            "garagedb",     // 数据库名称（需先通过sql/init.sql创建）
            "test",         // 用户名
            "test"          // 密码
        )) {
        QMessageBox::critical(nullptr, "数据库连接失败",
            "无法连接到MySQL数据库！\n\n"
            "请确保：\n"
            "1. MySQL 8.0 服务已启动\n"
            "2. 已通过 sql/init.sql 创建 garagedb 数据库\n"
            "3. 连接参数正确\n\n"
            "错误信息：" + DbManager::instance().lastError());
        return -1;
    }

    // ============================================================
    // 自动执行数据库迁移（确保 schema 与代码同步）
    // ============================================================
    {
        QSqlQuery mq(DbManager::instance().database());

        // 迁移: 更新 t_workorder.status ENUM
        //   - 移除 '已完工' 状态（流程简化: 已派工→库房直接提单→已结算）
        //   - 将已有的 '已完工' 记录迁移为 '维修中'
        mq.exec("SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS "
                "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_workorder' AND COLUMN_NAME='status'");
        if (mq.next()) {
            QString curType = mq.value(0).toString();
            qDebug() << "[main] 当前 t_workorder.status 列类型:" << curType;
            bool hasDispatched = curType.contains("已派工");
            bool hasFinished   = curType.contains("已完工");

            if (!hasDispatched || hasFinished || !curType.contains("待提单") || curType.contains("待派工")) {
                // 目标 ENUM: 已派工,待提单,维修中,已提单,已结算 (移除待派工)
                if (hasFinished) {
                    qDebug() << "[main] 将 '已完工' 记录迁移为 '维修中'...";
                    QSqlQuery upd(DbManager::instance().database());
                    upd.exec("UPDATE t_workorder SET status='维修中' WHERE status='已完工'");
                }
                qDebug() << "[main] 更新 status ENUM 定义...";
                QSqlQuery alt(DbManager::instance().database());
                if (alt.exec("ALTER TABLE t_workorder MODIFY COLUMN status "
                             "ENUM('已派工','待提单','维修中','已提单','已结算') "
                             "DEFAULT '已派工' COMMENT '工单状态'")) {
                    qDebug() << "[main] status ENUM 更新成功";
                } else {
                    qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
                }
            } else {
                qDebug() << "[main] status ENUM 已是最新, 无需迁移";
            }
        }

        // 迁移: 确保 t_customer 表有 address 列
        mq.exec("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
                "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_customer' AND COLUMN_NAME='address'");
        if (!mq.next()) {
            qDebug() << "[main] t_customer 缺少 address 列, 正在添加...";
            QSqlQuery alt(DbManager::instance().database());
            if (alt.exec("ALTER TABLE t_customer ADD COLUMN address VARCHAR(200) "
                         "COMMENT '地址' AFTER phone")) {
                qDebug() << "[main] ALTER TABLE 成功: 已添加 t_customer.address";
            } else {
                qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
            }
        } else {
            qDebug() << "[main] t_customer.address 列已存在, 无需迁移";
        }

        // 迁移: 确保 t_vehicle 表有 color / fuel_type / transmission 列
        // （某些旧库可能缺少这些列）
        QStringList vehCols = {"color", "fuel_type", "transmission"};
        for (const QString &col : vehCols) {
            mq.exec(QString("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
                            "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_vehicle' AND COLUMN_NAME='%1'")
                    .arg(col));
            if (!mq.next()) {
                QString type = (col == "color") ? "VARCHAR(20)" : "VARCHAR(20)";
                QString after = (col == "color") ? "model" : (col == "fuel_type") ? "color" : "fuel_type";
                qDebug() << "[main] t_vehicle 缺少" << col << "列, 正在添加...";
                QSqlQuery alt(DbManager::instance().database());
                if (alt.exec(QString("ALTER TABLE t_vehicle ADD COLUMN %1 %2 COMMENT '%3' AFTER %4")
                            .arg(col, type, col == "color" ? "颜色" : col == "fuel_type" ? "燃油类型" : "变速箱", after))) {
                    qDebug() << "[main] ALTER TABLE 成功: 已添加 t_vehicle." << col;
                } else {
                    qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
                }
            } else {
                qDebug() << "[main] t_vehicle." << col << " 列已存在, 无需迁移";
            }
        }

        // 迁移: 确保 t_workorder 表有 mechanic_tech_id / body_tech_id / paint_tech_id 列
        // （三主修人改造 - 前台工作台保存并派工需要这些列）
        struct TechCol { QString name; QString after; QString comment; };
        QList<TechCol> techCols = {
            {"mechanic_tech_id", "technician_id", "机电主修人(员工ID)"},
            {"body_tech_id",     "mechanic_tech_id", "钣金主修人(员工ID)"},
            {"paint_tech_id",    "body_tech_id",     "喷漆主修人(员工ID)"}
        };
        for (const auto &tc : techCols) {
            mq.exec(QString("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
                            "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_workorder' AND COLUMN_NAME='%1'")
                    .arg(tc.name));
            if (!mq.next()) {
                qDebug() << "[main] t_workorder 缺少" << tc.name << "列, 正在添加...";
                QSqlQuery alt(DbManager::instance().database());
                if (alt.exec(QString("ALTER TABLE t_workorder ADD COLUMN %1 INT NULL COMMENT '%2' AFTER %3")
                            .arg(tc.name, tc.comment, tc.after))) {
                    qDebug() << "[main] ALTER TABLE 成功: 已添加 t_workorder." << tc.name;
                } else {
                    qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
                }
            } else {
                qDebug() << "[main] t_workorder." << tc.name << " 列已存在, 无需迁移";
            }
        }

        // 迁移: t_maintenance_history 扩展字段（v2: 入出厂时间、里程、服务顾问等）
        struct MhCol { QString name; QString type; QString after; QString comment; };
        QList<MhCol> mhCols = {
            {"entry_date",      "DATETIME",       "maintenance_date", "入厂时间(派工时间)"},
            {"completion_date", "DATETIME",       "entry_date",       "出厂时间(结算时间)"},
            {"mileage",         "INT",            "completion_date",  "里程(km)"},
            {"service_advisor", "VARCHAR(50)",    "mileage",          "服务顾问姓名"},
            {"technicians",     "VARCHAR(200)",   "service_advisor",  "维修人员"},
            {"labor_fee",       "DECIMAL(10,2)",  "cumulative_amount","工时费"},
            {"material_fee",    "DECIMAL(10,2)",  "labor_fee",        "材料费"},
            {"other_fee",       "DECIMAL(10,2)",  "material_fee",     "其它费"},
            {"management_fee",  "DECIMAL(10,2)",  "other_fee",        "管理费"},
            {"deposit",         "DECIMAL(10,2)",  "management_fee",   "订金(已收)"},
            {"repair_items",    "TEXT",           "repair_summary",   "报修项目明细(JSON)"},
        };
        for (const auto &mc : mhCols) {
            mq.exec(QString("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS "
                            "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='%1'")
                    .arg(mc.name));
            if (!mq.next()) {
                qDebug() << "[main] t_maintenance_history 缺少" << mc.name << "列, 正在添加...";
                QSqlQuery alt(DbManager::instance().database());
                QString dflt = mc.type.startsWith("DECIMAL") ? " DEFAULT 0.00" : "";
                if (alt.exec(QString("ALTER TABLE t_maintenance_history ADD COLUMN %1 %2%3 COMMENT '%4' AFTER %5")
                            .arg(mc.name, mc.type, dflt, mc.comment, mc.after))) {
                    qDebug() << "[main] ALTER TABLE 成功: 已添加 t_maintenance_history." << mc.name;
                } else {
                    qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
                }
            } else {
                qDebug() << "[main] t_maintenance_history." << mc.name << " 列已存在, 无需迁移";
            }
        }
    }

    // ============================================================
    // 显示登录对话框
    // ============================================================
    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        // 用户取消登录，退出程序
        qDebug() << "用户取消登录，程序退出";
        return 0;
    }

    // ============================================================
    // 登录成功，显示主窗口
    // ============================================================
    MainWindow window;
    window.show();

    return a.exec();
}
