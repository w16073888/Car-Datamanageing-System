#include "DataDelete.h"
#include "CommandDispatcher.h"
#include "../auth/AuthManager.h"
#include "../db/DbManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonValue>
#include <QDebug>

static QJsonObject err(const QString &msg)
{
    return { { "ok", false }, { "error", msg } };
}

static QJsonObject ok()
{
    return { { "ok", true }, { "data", QJsonObject() } };
}

void DataDelete::registerCommands()
{
    CommandDispatcher &d = CommandDispatcher::instance();
    d.registerHandler("data.deleteRow", [](const QJsonObject &params, const SessionInfo *s) {
        if (!s)
            return err("未登录");
        return handleDeleteRow(params, s);
    });
}

QJsonObject DataDelete::handleDeleteRow(const QJsonObject &params, const SessionInfo *s)
{
    const QString table = params.value("table").toString();
    const int id = params.value("id").toInt(-1);
    if (id <= 0)
        return err("无效的记录 ID");

    // 职位权限校验（与服务端 Session 逻辑一致：经理=全部；前台=工单/回访；库管=备件）
    if (!AuthManager::canDeleteDataTable(s->position, table))
        return err("当前职位无权删除「" + table + "」中的记录");

    // ========== 回访记录：删除 = 清除该工单回访信息并恢复未回访 ==========
    if (table == "回访记录") {
        QSqlQuery up(DbManager::instance().database());
        up.prepare("UPDATE t_workorder SET satisfaction=NULL, remark=NULL, visitor_id=NULL, "
                   "visited_at=NULL, is_visited='未回访' WHERE id=:oid");
        up.bindValue(":oid", id);
        if (!DbManager::instance().executeQuery(up))
            return err("删除回访记录失败: " + DbManager::instance().lastError());
        DbManager::writeAuditLog(s->userId, "update", "t_workorder", id,
                                 "删除回访记录：清除工单回访信息 id=" + QString::number(id));
        return ok();
    }

    // ========== 工单表：校验仅"已派工"且未绑定备件，然后级联删除 ==========
    if (table == "t_workorder") {
        if (s->position != "经理") {
            QSqlQuery q(DbManager::instance().database());
            q.prepare("SELECT status FROM t_workorder WHERE id = :id");
            q.bindValue(":id", id);
            DbManager::instance().executeQuery(q);
            QString status;
            if (q.next()) status = q.value(0).toString();
            if (status != "已派工")
                return err(QString("该工单当前状态为「%1」，仅「已派工」状态的工单允许删除。")
                               .arg(status));

            QSqlQuery c1(DbManager::instance().database());
            c1.prepare("SELECT COUNT(*) FROM t_part_instance WHERE workorder_id = :wid");
            c1.bindValue(":wid", id);
            DbManager::instance().executeQuery(c1);
            const int instCount = c1.next() ? c1.value(0).toInt() : 0;

            QSqlQuery c2(DbManager::instance().database());
            c2.prepare("SELECT COUNT(*) FROM t_workorder_item WHERE workorder_id = :wid");
            c2.bindValue(":wid", id);
            DbManager::instance().executeQuery(c2);
            const int itemCount = c2.next() ? c2.value(0).toInt() : 0;

            if (instCount > 0 || itemCount > 0)
                return err(QString("该工单已绑定备件，无法删除。\n"
                                   "• 备件实例绑定: %1 条\n• 工单备件明细: %2 条\n\n"
                                   "请先退回已领出的备件后再删除工单。")
                               .arg(instCount).arg(itemCount));
        }

        QSqlQuery del(DbManager::instance().database());
        del.prepare("DELETE FROM t_workorder WHERE id = :id");
        del.bindValue(":id", id);
        if (!DbManager::instance().executeQuery(del))
            return err("删除工单失败: " + DbManager::instance().lastError());
        DbManager::writeAuditLog(s->userId, "delete", "t_workorder", id,
                                 "删除工单（含级联校验）id=" + QString::number(id));
        return ok();
    }

    // ========== 备件表：事务内级联删除 ==========
    if (table == "t_parts") {
        DbManager::instance().beginTransaction();
        bool okAll = true;
        auto step = [&](const QString &sql, const QString &bindName, int val) {
            if (!okAll) return;
            QSqlQuery q(DbManager::instance().database());
            q.prepare(sql);
            q.bindValue(bindName, val);
            if (!DbManager::instance().executeQuery(q)) {
                qWarning() << "[DataDelete] 级联删除失败:" << q.lastError().text();
                okAll = false;
            }
        };
        step("DELETE FROM t_inventory_log WHERE part_id = :pid", ":pid", id);
        step("DELETE FROM t_part_instance WHERE part_id = :pid", ":pid", id);
        step("DELETE FROM t_part_purchase WHERE part_id = :pid", ":pid", id);
        step("UPDATE t_workorder_item SET part_id = NULL WHERE part_id = :pid", ":pid", id);
        step("DELETE FROM t_parts WHERE id = :pid", ":pid", id);

        if (okAll) {
            DbManager::instance().commitTransaction();
            DbManager::writeAuditLog(s->userId, "delete", "t_parts", id,
                                     "删除备件（级联删除流水/实例/采购记录）id=" + QString::number(id));
            return ok();
        }
        DbManager::instance().rollbackTransaction();
        return err("删除备件失败: " + DbManager::instance().lastError());
    }

    // ========== 其他表白名单内：单条 DELETE ==========
    static const QSet<QString> kDeleteTables = {
        "t_employee", "t_vehicle", "t_workorder_item",
        "t_quote_item", "t_inventory_log", "t_settlement", "t_system_log",
        "t_vehicle_transaction", "t_part_purchase", "t_part_instance"
    };
    if (!kDeleteTables.contains(table))
        return err("不支持删除该表: " + table);

    QSqlQuery del(DbManager::instance().database());
    del.prepare(QString("DELETE FROM %1 WHERE id = :id").arg(table));
    del.bindValue(":id", id);
    if (!DbManager::instance().executeQuery(del))
        return err("删除失败: " + DbManager::instance().lastError());
    DbManager::writeAuditLog(s->userId, "delete", table, id,
                             "删除记录 id=" + QString::number(id));
    return ok();
}
