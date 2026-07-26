#include "SettlementPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QDateTime>
#include <QDebug>

#define S_BTN1 "QPushButton{padding:6px 16px;border:none;border-radius:3px;background:#3498db;color:#fff;font-weight:bold;}QPushButton:hover{background:#2980b9;}"
#define S_BTN2 "QPushButton{padding:6px 16px;border:none;border-radius:3px;background:#e67e22;color:#fff;font-weight:bold;}QPushButton:hover{background:#d35400;}"
#define S_BTN3 "QPushButton{padding:6px 16px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-weight:bold;}QPushButton:hover{background:#219a52;}"
#define S_BTNP "QPushButton{padding:6px 16px;border:none;border-radius:3px;background:#8e44ad;color:#fff;font-weight:bold;}QPushButton:hover{background:#7d3c98;}"

SettlementPage::SettlementPage(QWidget *parent) : QWidget(parent), m_currentOrderId(0) { setupUI(); }
SettlementPage::~SettlementPage() {}

void SettlementPage::refreshData()
{
    m_editOrderNo->clear();
    m_lblOrderInfo->setText("请加载工单");
    m_model->setQuery(QSqlQuery());
    m_lblLaborFee->setText("工时费：¥0.00");
    m_lblMaterialFee->setText("材料费：¥0.00");
    m_lblOtherFee->setText("其它费：¥0.00");
    m_lblManagementFee->setText("管理费：¥0.00");
    m_lblDeposit->setText("订金：¥0.00");
    m_lblTotal->setText("总计：¥0.00");
    m_btnNotifyWH->setEnabled(false);
    m_btnSettle->setEnabled(false);
    m_currentOrderId = 0;
}

void SettlementPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("工单结算");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 工单搜索 + 操作按钮
    QHBoxLayout *topLayout = new QHBoxLayout;
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("输入工单号搜索");
    topLayout->addWidget(m_editOrderNo, 1);
    m_btnLoad = new QPushButton("加载工单");
    m_btnLoad->setStyleSheet(S_BTN1);
    topLayout->addWidget(m_btnLoad);
    m_btnNotifyWH = new QPushButton("通知库房提单");
    m_btnNotifyWH->setStyleSheet(S_BTN2);
    m_btnNotifyWH->setEnabled(false);
    topLayout->addWidget(m_btnNotifyWH);
    m_btnSettle = new QPushButton("确认结算");
    m_btnSettle->setStyleSheet(S_BTN3);
    m_btnSettle->setEnabled(false);
    topLayout->addWidget(m_btnSettle);
    mainLayout->addLayout(topLayout);

    // 工单信息
    m_lblOrderInfo = new QLabel("请加载工单");
    m_lblOrderInfo->setStyleSheet("padding:8px;background:#f8f9fa;border-radius:4px;font-size:14px;");
    mainLayout->addWidget(m_lblOrderInfo);

    // 备件明细表
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section{background-color:#34495e;color:white;padding:5px;}");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    // 费用明细
    QGroupBox *feeGroup = new QGroupBox("费用明细");
    QGridLayout *feeGrid = new QGridLayout(feeGroup);
    feeGrid->setSpacing(6);

    m_lblLaborFee = new QLabel("工时费：¥0.00");
    m_lblLaborFee->setStyleSheet("font-weight:bold;font-size:13px;");
    m_lblMaterialFee = new QLabel("材料费：¥0.00");
    m_lblMaterialFee->setStyleSheet("font-weight:bold;font-size:13px;");
    m_lblOtherFee = new QLabel("其它费：¥0.00");
    m_lblOtherFee->setStyleSheet("font-weight:bold;font-size:13px;");
    m_lblManagementFee = new QLabel("管理费：¥0.00");
    m_lblManagementFee->setStyleSheet("font-weight:bold;font-size:13px;");
    m_lblDeposit = new QLabel("订金：¥0.00");
    m_lblDeposit->setStyleSheet("font-weight:bold;font-size:13px;");
    m_lblTotal = new QLabel("总计：¥0.00");
    m_lblTotal->setStyleSheet("font-size:18px;font-weight:bold;color:#e74c3c;");

    feeGrid->addWidget(m_lblLaborFee, 0, 0);
    feeGrid->addWidget(m_lblMaterialFee, 0, 1);
    feeGrid->addWidget(m_lblOtherFee, 1, 0);
    feeGrid->addWidget(m_lblManagementFee, 1, 1);
    feeGrid->addWidget(m_lblDeposit, 2, 0);
    feeGrid->addWidget(m_lblTotal, 0, 2, 1, 2, Qt::AlignRight);

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_btnPrintQuote = new QPushButton("打印报价单");
    m_btnPrintQuote->setStyleSheet(S_BTNP);
    btnRow->addWidget(m_btnPrintQuote);
    m_btnPrintSettle = new QPushButton("打印结算单");
    m_btnPrintSettle->setStyleSheet(S_BTNP);
    btnRow->addWidget(m_btnPrintSettle);
    feeGrid->addLayout(btnRow, 3, 0, 1, 4);

    mainLayout->addWidget(feeGroup);

    // 信号
    connect(m_btnLoad, &QPushButton::clicked, this, &SettlementPage::onLoadOrder);
    connect(m_btnNotifyWH, &QPushButton::clicked, this, &SettlementPage::onNotifyWarehouse);
    connect(m_btnSettle, &QPushButton::clicked, this, &SettlementPage::onSettle);
    connect(m_btnPrintSettle, &QPushButton::clicked, this, &SettlementPage::onPrintSettle);
    connect(m_btnPrintQuote, &QPushButton::clicked, this, &SettlementPage::onPrintQuote);
}

void SettlementPage::onLoadOrder()
{
    QString orderNo = m_editOrderNo->text().trimmed();
    if (orderNo.isEmpty()) return;

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT w.id, w.order_no, w.status, "
        "  COALESCE(w.labor_fee,0), COALESCE(w.material_fee,0), "
        "  COALESCE(w.other_fee,0), COALESCE(w.management_fee,0), "
        "  COALESCE(w.total_amount,0), COALESCE(w.deposit,0), "
        "  v.plate_number, w.mileage, w.main_technician, "
        "  w.repair_date, w.vehicle_id "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE w.order_no LIKE :no ORDER BY w.id DESC LIMIT 1");
    query.bindValue(":no", "%" + orderNo + "%");
    DbManager::instance().executeQuery(query);

    if (!query.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }

    m_currentOrderId = query.value(0).toInt();
    QString status = query.value(2).toString();
    double labor = query.value(3).toDouble();
    double mat = query.value(4).toDouble();
    double oth = query.value(5).toDouble();
    double mgmt = query.value(6).toDouble();
    double dep = query.value(8).toDouble();
    QString plate = query.value(9).toString();

    m_lblOrderInfo->setText(
        QString("工单：%1 | 车牌：%2 | 状态：%3")
        .arg(query.value(1).toString(), plate, status));

    // 加载工单明细（备件使用明细）
    QSqlQuery q2(DbManager::instance().database());
    q2.prepare("SELECT part_name AS '备件名称', quantity AS '数量', "
               "unit_price AS '单价', subtotal AS '小计' "
               "FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料'");
    q2.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(q2);
    m_model->setQuery(std::move(q2));

    // 汇总材料费（从工单明细计算）
    QSqlQuery q3(DbManager::instance().database());
    q3.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
               "WHERE workorder_id = :oid AND item_type = '材料'");
    q3.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(q3);
    double matFromItems = q3.next() ? q3.value(0).toDouble() : 0;

    // 显示费用（如果工单有记录的材料费就用工单的，否则用明细算的）
    double displayMat = (mat > 0) ? mat : matFromItems;
    double displayTotal = labor + displayMat + oth + mgmt;

    m_lblLaborFee->setText(QString("工时费：¥%1").arg(labor, 0, 'f', 2));
    m_lblMaterialFee->setText(QString("材料费：¥%1").arg(displayMat, 0, 'f', 2));
    m_lblOtherFee->setText(QString("其它费：¥%1").arg(oth, 0, 'f', 2));
    m_lblManagementFee->setText(QString("管理费：¥%1").arg(mgmt, 0, 'f', 2));
    m_lblDeposit->setText(QString("订金：¥%1").arg(dep, 0, 'f', 2));
    m_lblTotal->setText(QString("总计：¥%1").arg(displayTotal, 0, 'f', 2));

    // 根据状态启用按钮
    // 流程: 已派工/维修中 → 通知库房 → 库房提单(已提单) → 结算
    if (status == "已派工" || status == "维修中") {
        m_btnNotifyWH->setEnabled(true);
        m_btnSettle->setEnabled(false);
    } else if (status == "已提单") {
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(true);
    } else if (status == "已结算") {
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(false);
        QMessageBox::information(this, "提示", "该工单已结算");
    } else {
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(false);
        QMessageBox::warning(this, "状态错误",
            QString("当前状态为「%1」，需要「已派工」或「维修中」才能进行结算流程").arg(status));
    }
}

void SettlementPage::onNotifyWarehouse()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "通知库房",
            "确认通知库房进行材料审核提单？\n\n"
            "流程说明：\n"
            "1. 前台通知库房后，工单状态变为「待提单」\n"
            "2. 库房人员在「库房工作台-材料结算」审核材料\n"
            "3. 审核通过后设置「已提单」，前台即可结算",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // 通知库房：记录日志即可，状态不改变
    // 库房看到"已派工"或"维修中"状态的工单就可以做提单操作

    // 记录通知日志
    QSqlQuery logQ(DbManager::instance().database());
    logQ.prepare("INSERT INTO t_system_log (operator_id, action_type, table_name, record_id, detail) "
                 "VALUES (:op, '通知库房', 't_workorder', :woid, :detail)");
    logQ.bindValue(":op", Session::instance().userId());
    logQ.bindValue(":woid", m_currentOrderId);
    logQ.bindValue(":detail", QString("前台通知库房进行工单 %1 的材料审核提单").arg(m_editOrderNo->text()));
    DbManager::instance().executeQuery(logQ);

    QMessageBox::information(this, "通知成功",
        "已通知库房进行材料审核提单。\n\n"
        "请等待库房人员在「库房工作台-材料结算/提单」中完成审核。\n"
        "审核通过后工单状态变为「已提单」，届时可进行结算。");
    m_btnNotifyWH->setEnabled(false);
}

void SettlementPage::onSettle()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "确认结算",
            "确认结算该工单？结算后不可修改。\n\n"
            "请确认已收到库房的提单确认。",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DbManager::instance().database());

    // 获取各项费用
    q.prepare("SELECT COALESCE(labor_fee,0), COALESCE(material_fee,0), "
              "COALESCE(other_fee,0), COALESCE(management_fee,0), "
              "COALESCE(deposit,0) FROM t_workorder WHERE id=:id AND status='已提单'");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        QMessageBox::warning(this, "结算失败",
            "工单状态不是「已提单」，无法结算。\n请确认库房已完成材料审核提单。");
        return;
    }

    double labor = q.value(0).toDouble();
    double mat = q.value(1).toDouble();
    double oth = q.value(2).toDouble();
    double mgmt = q.value(3).toDouble();
    // 材料费取工单明细合计
    QSqlQuery qMat(DbManager::instance().database());
    qMat.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
                 "WHERE workorder_id=:oid AND item_type='材料'");
    qMat.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(qMat);
    double matFromItems = qMat.next() ? qMat.value(0).toDouble() : 0;
    double matTotal = (mat > 0) ? mat : matFromItems;

    double total = labor + matTotal + oth + mgmt;

    DbManager::instance().beginTransaction();

    // 更新工单状态
    q.prepare("UPDATE t_workorder SET status='已结算', total_amount=:total WHERE id=:id");
    q.bindValue(":total", total);
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    // 写入结算记录
    q.prepare("INSERT INTO t_settlement (workorder_id, labor_fee, material_fee, total_amount, settled_by) "
              "VALUES (:oid, :labor, :mat, :total, :by)");
    q.bindValue(":oid", m_currentOrderId);
    q.bindValue(":labor", labor);
    q.bindValue(":mat", matTotal);
    q.bindValue(":total", total);
    q.bindValue(":by", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    // 更新车辆最后光顾日期
    q.prepare("UPDATE t_vehicle v JOIN t_workorder w ON w.vehicle_id=v.id SET v.last_visit_date=CURDATE() WHERE w.id=:id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    // 记录交易历史 + 获取车辆ID
    int vid = 0;
    q.prepare("SELECT vehicle_id FROM t_workorder WHERE id=:id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);
    if (q.next()) {
        vid = q.value(0).toInt();
        QSqlQuery txn(DbManager::instance().database());
        txn.prepare("INSERT INTO t_vehicle_transaction (vehicle_id, workorder_id, transaction_type, "
                     "description, amount, operator_id) "
                     "VALUES (:vid, :woid, '结算', :desc, :amt, :op)");
        txn.bindValue(":vid", vid);
        txn.bindValue(":woid", m_currentOrderId);
        txn.bindValue(":desc", QString("工单结算完成，工时费¥%1+材料费¥%2+其它费¥%3，共计¥%4")
                      .arg(labor,0,'f',2).arg(matTotal,0,'f',2).arg(oth,0,'f',2).arg(total,0,'f',2));
        txn.bindValue(":amt", total);
        txn.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(txn);
    }

    // ========== 保存维修历史记录 ==========
    if (vid > 0) {
        // 1. 构建维修项目摘要（按机电/钣金/喷漆分组）
        QString repairSummary;
        {
            QSqlQuery qRepair(DbManager::instance().database());
            qRepair.prepare("SELECT item_type, repair_content, fee FROM t_workorder_repair_item "
                           "WHERE workorder_id=:woid ORDER BY item_type, id");
            qRepair.bindValue(":woid", m_currentOrderId);
            DbManager::instance().executeQuery(qRepair);
            QString currentType;
            QStringList typeItems;
            while (qRepair.next()) {
                QString typ = qRepair.value(0).toString();
                QString content = qRepair.value(1).toString().trimmed();
                double fee = qRepair.value(2).toDouble();
                if (typ != currentType) {
                    if (!typeItems.isEmpty())
                        repairSummary += currentType + ": " + typeItems.join(", ") + "; ";
                    currentType = typ;
                    typeItems.clear();
                }
                if (!content.isEmpty() || fee > 0)
                    typeItems << QString("%1 ¥%2").arg(content).arg(fee, 0, 'f', 2);
            }
            if (!typeItems.isEmpty())
                repairSummary += currentType + ": " + typeItems.join(", ") + "; ";
            if (repairSummary.endsWith("; "))
                repairSummary.chop(2);
        }

        // 2. 构建备件使用摘要
        QString partsSummary;
        {
            QSqlQuery qParts(DbManager::instance().database());
            qParts.prepare("SELECT part_name, quantity FROM t_workorder_item "
                          "WHERE workorder_id=:woid AND item_type='材料'");
            qParts.bindValue(":woid", m_currentOrderId);
            DbManager::instance().executeQuery(qParts);
            QStringList partItems;
            while (qParts.next())
                partItems << QString("%1 x%2").arg(qParts.value(0).toString()).arg(qParts.value(1).toInt());
            partsSummary = partItems.join(", ");
        }

        // 3. 计算累计消费（之前所有历史总额 + 本次）
        double cumulative = total;
        {
            QSqlQuery qCum(DbManager::instance().database());
            qCum.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_maintenance_history WHERE vehicle_id=:vid");
            qCum.bindValue(":vid", vid);
            DbManager::instance().executeQuery(qCum);
            if (qCum.next())
                cumulative = qCum.value(0).toDouble() + total;
        }

        // 4. 插入维修历史记录
        {
            QSqlQuery qIns(DbManager::instance().database());
            qIns.prepare("INSERT INTO t_maintenance_history "
                        "(vehicle_id, workorder_id, maintenance_date, total_amount, cumulative_amount, parts_summary, repair_summary) "
                        "VALUES (:vid, :woid, NOW(), :total, :cumulative, :parts, :repair)");
            qIns.bindValue(":vid", vid);
            qIns.bindValue(":woid", m_currentOrderId);
            qIns.bindValue(":total", total);
            qIns.bindValue(":cumulative", cumulative);
            qIns.bindValue(":parts", partsSummary.isEmpty() ? QVariant() : partsSummary);
            qIns.bindValue(":repair", repairSummary.isEmpty() ? QVariant() : repairSummary);
            DbManager::instance().executeQuery(qIns);
        }

        // 5. 更新车辆最后保养日期
        {
            QSqlQuery qUpd(DbManager::instance().database());
            qUpd.prepare("UPDATE t_vehicle SET last_maintenance_date = CURDATE() WHERE id=:vid");
            qUpd.bindValue(":vid", vid);
            DbManager::instance().executeQuery(qUpd);
        }
    }

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "结算成功",
        QString("工单结算完成！\n总金额：¥%1").arg(total, 0, 'f', 2));
    m_btnNotifyWH->setEnabled(false);
    m_btnSettle->setEnabled(false);
    m_lblOrderInfo->setText(m_lblOrderInfo->text().replace("已提单", "已结算"));
}

void SettlementPage::onPrintSettle()
{
    if (m_currentOrderId == 0) { QMessageBox::warning(this,"提示","请先加载工单"); return; }

    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT w.order_no, v.plate_number, v.model, "
              "c.name, c.phone, w.mileage, w.repair_date, w.main_technician "
              "FROM t_workorder w "
              "LEFT JOIN t_vehicle v ON v.id=w.vehicle_id "
              "LEFT JOIN t_customer c ON c.vehicle_id=v.id AND c.type='车主' "
              "WHERE w.id=:id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString html = QString(
            "<div style='text-align:center;'><h2>维修结算单</h2><hr></div>"
            "<p><b>工单号：</b>%1 &nbsp;&nbsp; <b>车牌：</b>%2</p>"
            "<p><b>车型：</b>%3 &nbsp;&nbsp; <b>车主：</b>%4 &nbsp;&nbsp; <b>电话：</b>%5</p>"
            "<p><b>公里数：</b>%6 &nbsp;&nbsp; <b>报修日期：</b>%7</p>"
            "<p><b>主修人：</b>%8</p>"
            "<hr><table border='0' cellpadding='6' style='width:100%;font-size:15px;'>"
            "<tr><td>%9</td><td align='right'>%9</td></tr>"
            "<tr><td>%10</td><td align='right'>%10</td></tr>"
            "<tr><td>%11</td><td align='right'>%11</td></tr>"
            "<tr><td>%12</td><td align='right'>%12</td></tr>"
            "<tr><td>%13</td><td align='right'>%13</td></tr>"
            "<tr style='font-size:18px;color:#e74c3c;'><td><b>%14</b></td><td align='right'><b>%14</b></td></tr>"
            "</table>"
            "<hr><p style='color:#7f8c8d;font-size:12px;'>打印时间：%15</p>"
        ).arg(m_editOrderNo->text())
         .arg(q.value(1).toString()).arg(q.value(2).toString())
         .arg(q.value(3).toString()).arg(q.value(4).toString())
         .arg(q.value(5).toString()).arg(q.value(6).toDate().toString("yyyy-MM-dd"))
         .arg(q.value(7).toString())
         .arg(m_lblLaborFee->text())
         .arg(m_lblMaterialFee->text())
         .arg(m_lblOtherFee->text())
         .arg(m_lblManagementFee->text())
         .arg(m_lblDeposit->text())
         .arg(m_lblTotal->text())
         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        doc.setHtml(html);
        doc.print(p);
    });
    preview.exec();
}

void SettlementPage::onPrintQuote()
{
    if (m_currentOrderId == 0) { QMessageBox::warning(this,"提示","请先加载工单"); return; }

    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;

        // 获取工单维修项目明细
        QSqlQuery qItem(DbManager::instance().database());
        qItem.prepare("SELECT item_type, repair_person, repair_content, fee "
                      "FROM t_workorder_repair_item WHERE workorder_id=:oid");
        qItem.bindValue(":oid", m_currentOrderId);
        DbManager::instance().executeQuery(qItem);

        QString itemRows;
        while (qItem.next()) {
            QString sFee = QString::number(qItem.value(3).toDouble(), 'f', 2);
            itemRows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>¥%4</td></tr>")
                        .arg(qItem.value(0).toString(), qItem.value(1).toString(),
                             qItem.value(2).toString(), sFee);
        }

        // 获取备件明细
        QSqlQuery qPart(DbManager::instance().database());
        qPart.prepare("SELECT part_name, quantity, unit_price, subtotal "
                      "FROM t_workorder_item WHERE workorder_id=:oid AND item_type='材料'");
        qPart.bindValue(":oid", m_currentOrderId);
        DbManager::instance().executeQuery(qPart);

        QString partRows;
        while (qPart.next()) {
            partRows += QString("<tr><td>%1</td><td>%2</td><td>¥%3</td><td>¥%4</td></tr>")
                        .arg(qPart.value(0).toString())
                        .arg(qPart.value(1).toInt())
                        .arg(qPart.value(2).toDouble(), 0, 'f', 2)
                        .arg(qPart.value(3).toDouble(), 0, 'f', 2);
        }

        QString html = QString(
            "<div style='text-align:center;'><h2>维修报价单</h2><hr></div>"
            "<p><b>工单号：</b>%1</p>"
            "<p>%2<br>%3<br>%4<br>%5</p>"
            "<hr><h4>维修项目</h4>"
            "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
            "<tr style='background:#34495e;color:white;'><th>类型</th><th>维修人</th><th>内容</th><th>费用</th></tr>%6</table>"
            "<h4>材料明细</h4>"
            "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
            "<tr style='background:#34495e;color:white;'><th>名称</th><th>数量</th><th>单价</th><th>小计</th></tr>%7</table>"
            "<br><p style='font-size:16px;font-weight:bold;text-align:right;'>%8</p>"
            "<p style='font-size:14px;font-weight:bold;text-align:right;'>%9</p>"
            "<p style='font-size:14px;font-weight:bold;text-align:right;color:#e74c3c;'>%10</p>"
            "<hr><p style='color:#7f8c8d;font-size:12px;'>打印时间：%11</p>"
        ).arg(m_editOrderNo->text(),
              m_lblOrderInfo->text(),
              m_lblLaborFee->text(),
              m_lblMaterialFee->text(),
              m_lblOtherFee->text())
         .arg(itemRows, partRows)
         .arg(m_lblManagementFee->text())
         .arg(m_lblDeposit->text())
         .arg(m_lblTotal->text())
         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        doc.setHtml(html);
        doc.print(p);
    });
    preview.exec();
}
