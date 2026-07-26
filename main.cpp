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

        // 迁移: 将 '已派工' 加入 t_workorder.status ENUM
        mq.exec("SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS "
                "WHERE TABLE_SCHEMA='garagedb' AND TABLE_NAME='t_workorder' AND COLUMN_NAME='status'");
        if (mq.next()) {
            QString curType = mq.value(0).toString();
            qDebug() << "[main] 当前 t_workorder.status 列类型:" << curType;
            if (!curType.contains("已派工")) {
                qDebug() << "[main] 正在将 '已派工' 加入 status ENUM...";
                QSqlQuery alt(DbManager::instance().database());
                if (alt.exec("ALTER TABLE t_workorder MODIFY COLUMN status "
                             "ENUM('待派工','已派工','维修中','已完工','已提单','已结算') "
                             "DEFAULT '待派工' COMMENT '工单状态'")) {
                    qDebug() << "[main] ALTER TABLE 成功: 已添加 '已派工' 状态";
                } else {
                    qWarning() << "[main] ALTER TABLE 失败:" << alt.lastError().text();
                }
            } else {
                qDebug() << "[main] status ENUM 已包含 '已派工', 无需迁移";
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
