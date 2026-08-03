#include "SettlementPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
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

// 表格通用样式
#define TBL_STYLE "QHeaderView::section{background:#34495e;color:#fff;padding:5px;font-weight:bold;}"

SettlementPage::SettlementPage(QWidget *parent) : QWidget(parent), m_currentOrderId(0)
{
    m_cachedMileage = 0;
    m_cachedLaborFee = m_cachedMatFee = m_cachedOtherFee = m_cachedMgmtFee = m_cachedDeposit = m_cachedTotal = 0;
    setupUI();
}

SettlementPage::~SettlementPage() {}

void SettlementPage::refreshData()
{
    m_editOrderNo->clear();
    m_lblOrderInfo->setText("请加载工单");
    m_lblAdvisor->setText("");
    m_lblAdvisor->setVisible(false);
    m_partsTable->setRowCount(0);
    m_lblPartsTitle->setVisible(false);
    m_laborTable->setRowCount(0);
    m_lblLaborTitle->setVisible(false);
    m_lblOtherFee->setText("其它费：¥0.00");
    m_lblManagementFee->setText("管理费：¥0.00");
    m_lblDeposit->setText("订金：¥0.00");
    m_lblTotal->setText("总计：¥0.00");
    m_btnNotifyWH->setEnabled(false);
    m_btnSettle->setEnabled(false);
    m_btnSettle->setVisible(true);
    m_currentOrderId = 0;
    m_cachedLaborItems.clear();
    m_cachedPartItems.clear();
}

void SettlementPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("工单结算");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // ==================== 工单搜索 + 操作按钮 ====================
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

    // ==================== 工单信息 ====================
    m_lblOrderInfo = new QLabel("请加载工单");
    m_lblOrderInfo->setStyleSheet("padding:8px;background:#f8f9fa;border-radius:4px;font-size:14px;");
    m_lblOrderInfo->setWordWrap(true);
    mainLayout->addWidget(m_lblOrderInfo);

    m_lblAdvisor = new QLabel("");
    m_lblAdvisor->setStyleSheet("padding:4px 8px;font-size:13px;color:#2c3e50;font-weight:bold;");
    m_lblAdvisor->setVisible(false);
    mainLayout->addWidget(m_lblAdvisor);

    // ==================== 工单明细 ====================
    QGroupBox *detailGroup = new QGroupBox("工单明细");
    QVBoxLayout *detailLayout = new QVBoxLayout(detailGroup);
    detailLayout->setSpacing(8);

    // ---- 备件明细 ----
    m_lblPartsTitle = new QLabel("▸ 备件明细");
    m_lblPartsTitle->setStyleSheet("font-weight:bold;font-size:13px;color:#2c3e50;");
    m_lblPartsTitle->setVisible(false);
    detailLayout->addWidget(m_lblPartsTitle);

    m_partsTable = new QTableWidget(0, 4);
    m_partsTable->setHorizontalHeaderLabels({"备件名称", "备件型号", "备件数量", "备件单价"});
    m_partsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partsTable->setAlternatingRowColors(true);
    m_partsTable->verticalHeader()->setVisible(false);
    m_partsTable->horizontalHeader()->setStretchLastSection(true);
    m_partsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partsTable->setStyleSheet(TBL_STYLE);
    m_partsTable->setMaximumHeight(200);
    detailLayout->addWidget(m_partsTable);

    // ---- 工时费明细 ----
    m_lblLaborTitle = new QLabel("▸ 工时费明细");
    m_lblLaborTitle->setStyleSheet("font-weight:bold;font-size:13px;color:#2c3e50;");
    m_lblLaborTitle->setVisible(false);
    detailLayout->addWidget(m_lblLaborTitle);

    m_laborTable = new QTableWidget(0, 4);
    m_laborTable->setHorizontalHeaderLabels({"类型", "主修人", "维修内容", "费用"});
    m_laborTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_laborTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_laborTable->setAlternatingRowColors(true);
    m_laborTable->verticalHeader()->setVisible(false);
    m_laborTable->horizontalHeader()->setStretchLastSection(true);
    m_laborTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_laborTable->setStyleSheet(TBL_STYLE);
    m_laborTable->setMaximumHeight(250);
    detailLayout->addWidget(m_laborTable);

    // ---- 其他费用 ----
    QGroupBox *otherGroup = new QGroupBox("▸ 其他");
    otherGroup->setStyleSheet("QGroupBox{font-weight:bold;font-size:13px;color:#2c3e50;border:none;}");
    QGridLayout *otherGrid = new QGridLayout(otherGroup);
    otherGrid->setSpacing(6);

    m_lblOtherFee = new QLabel("其它费：¥0.00");
    m_lblOtherFee->setStyleSheet("font-size:13px;");
    m_lblManagementFee = new QLabel("管理费：¥0.00");
    m_lblManagementFee->setStyleSheet("font-size:13px;");
    m_lblDeposit = new QLabel("订金：¥0.00");
    m_lblDeposit->setStyleSheet("font-size:13px;");
    m_lblTotal = new QLabel("总计：¥0.00");
    m_lblTotal->setStyleSheet("font-size:16px;font-weight:bold;color:#e74c3c;");

    otherGrid->addWidget(m_lblOtherFee, 0, 0);
    otherGrid->addWidget(m_lblManagementFee, 0, 1);
    otherGrid->addWidget(m_lblDeposit, 1, 0);
    otherGrid->addWidget(m_lblTotal, 1, 1);

    detailLayout->addWidget(otherGroup);
    mainLayout->addWidget(detailGroup, 1);

    // ==================== 打印按钮 ====================
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_btnPrintQuote = new QPushButton("打印报价单");
    m_btnPrintQuote->setStyleSheet(S_BTNP);
    btnRow->addWidget(m_btnPrintQuote);
    m_btnPrintSettle = new QPushButton("打印结算单");
    m_btnPrintSettle->setStyleSheet(S_BTNP);
    btnRow->addWidget(m_btnPrintSettle);
    mainLayout->addLayout(btnRow);

    // ==================== 信号 ====================
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
        "  w.repair_date, w.vehicle_id, v.model, "
        "  c.name, c.phone, "
        "  w.mechanic_tech_id, w.body_tech_id, w.paint_tech_id "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
        "WHERE w.order_no LIKE :no ORDER BY w.id DESC LIMIT 1");
    query.bindValue(":no", "%" + orderNo + "%");
    DbManager::instance().executeQuery(query);

    if (!query.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }

    m_currentOrderId = query.value(0).toInt();
    QString orderNoStr = query.value(1).toString();
    QString status = query.value(2).toString();
    double labor = query.value(3).toDouble();
    double mat = query.value(4).toDouble();
    double oth = query.value(5).toDouble();
    double mgmt = query.value(6).toDouble();
    double dep = query.value(8).toDouble();
    QString plate = query.value(9).toString();
    int repairDateIdx = 12;
    QString repairDate = query.value(repairDateIdx).toDate().toString("yyyy-MM-dd");
    QString model = query.value(13).toString();
    QString owner = query.value(14).toString();
    QString phone = query.value(15).toString();

    // 缓存基础信息（用于打印）
    m_cachedOrderNo = orderNoStr;
    m_cachedPlate = plate;
    m_cachedModel = model;
    m_cachedOwner = owner;
    m_cachedPhone = phone;
    m_cachedMileage = query.value(10).toInt();
    m_cachedRepairDate = repairDate;
    m_cachedTechnicians = query.value(11).toString();
    m_cachedStatus = status;

    // 查询服务顾问
    QString advisor;
    {
        QSqlQuery aq(DbManager::instance().database());
        aq.prepare("SELECT e.name FROM t_employee e "
                   "JOIN t_workorder w ON w.customer_service_id = e.id "
                   "WHERE w.id = :id");
        aq.bindValue(":id", m_currentOrderId);
        DbManager::instance().executeQuery(aq);
        if (aq.next())
            advisor = aq.value(0).toString();
    }
    m_cachedAdvisor = advisor;

    // 获取机电/钣金/喷漆主修人
    QString mechTech, bodyTech, paintTech;
    {
        QSqlQuery tq(DbManager::instance().database());
        tq.prepare("SELECT "
                   "(SELECT e.name FROM t_employee e WHERE e.id = w.mechanic_tech_id) AS mech, "
                   "(SELECT e.name FROM t_employee e WHERE e.id = w.body_tech_id) AS body, "
                   "(SELECT e.name FROM t_employee e WHERE e.id = w.paint_tech_id) AS paint "
                   "FROM t_workorder w WHERE w.id = :id");
        tq.bindValue(":id", m_currentOrderId);
        DbManager::instance().executeQuery(tq);
        if (tq.next()) {
            mechTech = tq.value(0).toString();
            bodyTech = tq.value(1).toString();
            paintTech = tq.value(2).toString();
        }
    }

    // 工单信息（两行展示：基本信息 + 服务顾问/公里数/报修日期）
    m_lblOrderInfo->setText(
        QString("工单: %1 | 车牌: %2 | 状态: %3 | 主修: %4")
        .arg(orderNoStr, plate, status, m_cachedTechnicians));

    // 服务顾问独立行
    if (!advisor.isEmpty()) {
        m_lblAdvisor->setText(
            QString("服务顾问: %1 | 公里数: %2 km | 报修日期: %3")
            .arg(advisor).arg(m_cachedMileage).arg(repairDate));
        m_lblAdvisor->setVisible(true);
    } else {
        m_lblAdvisor->setText(
            QString("公里数: %1 km | 报修日期: %2")
            .arg(m_cachedMileage).arg(repairDate));
        m_lblAdvisor->setVisible(true);
    }

    // ========== 加载备件明细 ==========
    m_cachedPartItems.clear();
    {
        QSqlQuery pq(DbManager::instance().database());
        pq.prepare("SELECT part_name, COALESCE(p.spec,'') AS spec, wi.quantity, wi.unit_price "
                   "FROM t_workorder_item wi "
                   "LEFT JOIN t_parts p ON p.id = wi.part_id "
                   "WHERE wi.workorder_id = :oid AND wi.item_type = '材料'");
        pq.bindValue(":oid", m_currentOrderId);
        DbManager::instance().executeQuery(pq);

        m_partsTable->setRowCount(0);
        int row = 0;
        while (pq.next()) {
            PartItem pi;
            pi.name = pq.value(0).toString();
            pi.spec = pq.value(1).toString();
            pi.qty = pq.value(2).toInt();
            pi.price = pq.value(3).toDouble();
            m_cachedPartItems << pi;

            m_partsTable->setRowCount(row + 1);
            m_partsTable->setItem(row, 0, new QTableWidgetItem(pi.name));
            m_partsTable->setItem(row, 1, new QTableWidgetItem(pi.spec.isEmpty() ? "-" : pi.spec));
            m_partsTable->setItem(row, 2, new QTableWidgetItem(QString::number(pi.qty)));
            m_partsTable->setItem(row, 3, new QTableWidgetItem(QString("¥%1").arg(pi.price, 0, 'f', 2)));
            row++;
        }
        m_lblPartsTitle->setVisible(row > 0);
    }

    // ========== 加载工时费明细（按机电/钣金/喷漆分组） ==========
    m_cachedLaborItems.clear();
    {
        QSqlQuery lq(DbManager::instance().database());
        lq.prepare("SELECT item_type, repair_person, repair_content, fee "
                   "FROM t_workorder_repair_item "
                   "WHERE workorder_id = :oid ORDER BY item_type, id");
        lq.bindValue(":oid", m_currentOrderId);
        DbManager::instance().executeQuery(lq);

        // 按类型组织，先收集再填充（确保有内容条目才显示行）
        struct TypeGroup {
            QString techName;
            QList<QPair<QString, double>> items;
        };
        QMap<QString, TypeGroup> groups;
        groups["机电"].techName = mechTech;
        groups["钣金"].techName = bodyTech;
        groups["喷漆"].techName = paintTech;

        while (lq.next()) {
            QString typ = lq.value(0).toString();
            QString person = lq.value(1).toString();
            QString content = lq.value(2).toString().trimmed();
            double fee = lq.value(3).toDouble();

            if (!groups.contains(typ))
                groups[typ].techName = person;

            LaborItem li;
            li.type = typ;
            li.person = person.isEmpty() ? groups[typ].techName : person;
            li.content = content;
            li.fee = fee;
            m_cachedLaborItems << li;

            if (!content.isEmpty() || fee > 0)
                groups[typ].items << QPair<QString, double>(content, fee);
        }

        m_laborTable->setRowCount(0);
        int row = 0;
        QStringList typeOrder = {"机电", "钣金", "喷漆"};
        for (const QString &typ : typeOrder) {
            if (!groups.contains(typ)) continue;
            const TypeGroup &g = groups[typ];
            if (g.items.isEmpty()) continue;
            for (const auto &item : g.items) {
                m_laborTable->setRowCount(row + 1);
                m_laborTable->setItem(row, 0, new QTableWidgetItem(typ));
                m_laborTable->setItem(row, 1, new QTableWidgetItem(g.techName.isEmpty() ? "-" : g.techName));
                m_laborTable->setItem(row, 2, new QTableWidgetItem(item.first.isEmpty() ? "(无内容)" : item.first));
                m_laborTable->setItem(row, 3, new QTableWidgetItem(QString("¥%1").arg(item.second, 0, 'f', 2)));
                row++;
            }
        }
        m_lblLaborTitle->setVisible(row > 0);
    }

    // 汇总材料费（从工单明细计算）
    QSqlQuery q3(DbManager::instance().database());
    q3.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
               "WHERE workorder_id = :oid AND item_type = '材料'");
    q3.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(q3);
    double matFromItems = q3.next() ? q3.value(0).toDouble() : 0;

    double displayMat = (mat > 0) ? mat : matFromItems;
    double displayTotal = labor + displayMat + oth + mgmt;

    // 缓存费用数据
    m_cachedLaborFee = labor;
    m_cachedMatFee = displayMat;
    m_cachedOtherFee = oth;
    m_cachedMgmtFee = mgmt;
    m_cachedDeposit = dep;
    m_cachedTotal = displayTotal;

    // 显示费用
    m_lblOtherFee->setText(QString("其它费：¥%1").arg(oth, 0, 'f', 2));
    m_lblManagementFee->setText(QString("管理费：¥%1").arg(mgmt, 0, 'f', 2));
    m_lblDeposit->setText(QString("订金：¥%1").arg(dep, 0, 'f', 2));
    m_lblTotal->setText(QString("总计：¥%1").arg(displayTotal, 0, 'f', 2));

    // ========== 根据状态启用按钮 ==========
    if (status == "已派工" || status == "待提单") {
        m_btnNotifyWH->setEnabled(true);
        m_btnSettle->setEnabled(false);
        m_btnSettle->setVisible(status != "已结算");
    } else if (status == "已提单") {
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(true);
        m_btnSettle->setVisible(true);
    } else if (status == "已结算") {
        // 已结算工单：正常展示所有信息，仅隐藏结算按钮
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(false);
        m_btnSettle->setVisible(false);
    } else {
        m_btnNotifyWH->setEnabled(false);
        m_btnSettle->setEnabled(false);
        m_btnSettle->setVisible(true);
        QMessageBox::warning(this, "状态错误",
            QString("当前状态为「%1」，需要「已派工」「待提单」或「已提单」才能进行结算流程").arg(status));
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

    // 更新工单状态为待提单
    QSqlQuery q(DbManager::instance().database());
    q.prepare("UPDATE t_workorder SET status = '待提单' "
              "WHERE id = :id AND status IN ('已派工')");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    // 记录通知日志
    QSqlQuery logQ(DbManager::instance().database());
    logQ.prepare("INSERT INTO t_system_log (operator_id, action_type, table_name, record_id, detail) "
                 "VALUES (:op, '通知库房', 't_workorder', :woid, :detail)");
    logQ.bindValue(":op", Session::instance().userId());
    logQ.bindValue(":woid", m_currentOrderId);
    logQ.bindValue(":detail", QString("前台通知库房进行工单 %1 的材料审核提单，状态 → 待提单").arg(m_editOrderNo->text()));
    DbManager::instance().executeQuery(logQ);

    // 更新界面状态
    m_cachedStatus = "待提单";
    m_lblOrderInfo->setText(m_lblOrderInfo->text().replace("已派工", "待提单"));

    QMessageBox::information(this, "通知成功",
        "已通知库房进行材料审核提单，工单状态已变更为「待提单」。\n\n"
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

    QSqlQuery qMat(DbManager::instance().database());
    qMat.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
                 "WHERE workorder_id=:oid AND item_type='材料'");
    qMat.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(qMat);
    double matFromItems = qMat.next() ? qMat.value(0).toDouble() : 0;
    double matTotal = (mat > 0) ? mat : matFromItems;

    double total = labor + matTotal + oth + mgmt;

    DbManager::instance().beginTransaction();

    q.prepare("UPDATE t_workorder SET status='已结算', total_amount=:total WHERE id=:id");
    q.bindValue(":total", total);
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    q.prepare("INSERT INTO t_settlement (workorder_id, labor_fee, material_fee, total_amount, settled_by) "
              "VALUES (:oid, :labor, :mat, :total, :by)");
    q.bindValue(":oid", m_currentOrderId);
    q.bindValue(":labor", labor);
    q.bindValue(":mat", matTotal);
    q.bindValue(":total", total);
    q.bindValue(":by", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    q.prepare("UPDATE t_vehicle v JOIN t_workorder w ON w.vehicle_id=v.id SET v.last_visit_date=CURDATE() WHERE w.id=:id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

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
        QString repairSummary;
        QJsonArray repairItemsJson;
        {
            QSqlQuery qRepair(DbManager::instance().database());
            qRepair.prepare("SELECT item_type, repair_person, repair_content, fee FROM t_workorder_repair_item "
                           "WHERE workorder_id=:woid ORDER BY item_type, id");
            qRepair.bindValue(":woid", m_currentOrderId);
            DbManager::instance().executeQuery(qRepair);
            QString currentType;
            QStringList typeItems;
            while (qRepair.next()) {
                QString typ = qRepair.value(0).toString();
                QString person = qRepair.value(1).toString();
                QString content = qRepair.value(2).toString().trimmed();
                double fee = qRepair.value(3).toDouble();
                QJsonObject obj;
                obj["type"] = typ;
                obj["person"] = person;
                obj["content"] = content;
                obj["fee"] = fee;
                repairItemsJson.append(obj);
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

        double cumulative = total;
        {
            QSqlQuery qCum(DbManager::instance().database());
            qCum.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_maintenance_history WHERE vehicle_id=:vid");
            qCum.bindValue(":vid", vid);
            DbManager::instance().executeQuery(qCum);
            if (qCum.next())
                cumulative = qCum.value(0).toDouble() + total;
        }

        {
            QString repairItemsStr = QJsonDocument(repairItemsJson).toJson(QJsonDocument::Compact);
            QSqlQuery qIns(DbManager::instance().database());
            qIns.prepare("INSERT INTO t_maintenance_history "
                        "(vehicle_id, workorder_id, status, maintenance_date, total_amount, cumulative_amount, "
                        "parts_summary, repair_summary, repair_items) "
                        "VALUES (:vid, :woid, '已结算', NOW(), :total, :cumulative, :parts, :repair, :ritems) "
                        "ON DUPLICATE KEY UPDATE status='已结算', completion_date=NOW(), "
                        "total_amount=VALUES(total_amount), cumulative_amount=VALUES(cumulative_amount), "
                        "parts_summary=VALUES(parts_summary), repair_summary=VALUES(repair_summary), "
                        "repair_items=VALUES(repair_items)");
            qIns.bindValue(":vid", vid);
            qIns.bindValue(":woid", m_currentOrderId);
            qIns.bindValue(":total", total);
            qIns.bindValue(":cumulative", cumulative);
            qIns.bindValue(":parts", partsSummary.isEmpty() ? QVariant() : partsSummary);
            qIns.bindValue(":repair", repairSummary.isEmpty() ? QVariant() : repairSummary);
            qIns.bindValue(":ritems", repairItemsStr.isEmpty() ? QVariant() : repairItemsStr);
            DbManager::instance().executeQuery(qIns);
        }

        {
            QSqlQuery qUpd(DbManager::instance().database());
            qUpd.prepare("UPDATE t_vehicle SET last_maintenance_date = CURDATE(), "
                         "last_maintenance_mileage = :mile WHERE id=:vid");
            qUpd.bindValue(":mile", m_cachedMileage);
            qUpd.bindValue(":vid", vid);
            DbManager::instance().executeQuery(qUpd);
        }
    }

    // 记录结算日志（仅记录工单号，访问时可通过该工单号查询 t_workorder）
    {
        QSqlQuery logQ(DbManager::instance().database());
        logQ.prepare("INSERT INTO t_system_log (operator_id, action_type, table_name, record_id, detail) "
                     "VALUES (:op, '结算', 't_workorder', :oid, :detail)");
        logQ.bindValue(":op", Session::instance().userId());
        logQ.bindValue(":oid", m_currentOrderId);
        logQ.bindValue(":detail", QString("工单结算，工单号：%1").arg(m_editOrderNo->text().trimmed()));
        DbManager::instance().executeQuery(logQ);
    }

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "结算成功",
        QString("工单结算完成！\n总金额：¥%1").arg(total, 0, 'f', 2));
    m_btnNotifyWH->setEnabled(false);
    m_btnSettle->setEnabled(false);
    m_btnSettle->setVisible(false);
    m_cachedStatus = "已结算";
    m_lblOrderInfo->setText(m_lblOrderInfo->text().replace("已提单", "已结算"));
}

// ============================================================
// 构建结算单/报价单HTML（共用）
// ============================================================
static QString buildSettleHtml(const QString &orderNo, const QString &plate, const QString &model,
    const QString &owner, const QString &phone, const QString &advisor,
    int mileage, const QString &repairDate, const QString &technicians,
    const QString &status, const QList<SettlementPage::LaborItem> &laborItems,
    const QList<SettlementPage::PartItem> &partItems,
    double laborFee, double matFee, double otherFee, double mgmtFee,
    double deposit, double total, bool isSettle)
{
    auto Y = [](double v) { return QString("¥%1").arg(v, 0, 'f', 2); };
    auto F = [](const QString &s) { return s.isEmpty() ? "-" : s; };

    // 备件明细 HTML
    QString partsHtml;
    if (partItems.isEmpty()) {
        partsHtml = "<p style='color:#999;font-style:italic;padding:4px 8px;'>无备件使用记录</p>";
    } else {
        partsHtml = "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
                    "<tr style='background:#34495e;color:white;'>"
                    "<th>备件名称</th><th>备件型号</th><th>备件数量</th><th>备件单价</th></tr>";
        for (auto &p : partItems) {
            partsHtml += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>%4</td></tr>")
                .arg(p.name, F(p.spec), QString::number(p.qty), Y(p.price));
        }
        partsHtml += "</table>";
    }

    // 工时费明细 HTML（按机电/钣金/喷漆分组，每组显示主修人+维修内容+费用）
    QString laborHtml;
    if (laborItems.isEmpty()) {
        laborHtml = "<p style='color:#999;font-style:italic;padding:4px 8px;'>无工时费记录</p>";
    } else {
        QStringList typeOrder = {"机电", "钣金", "喷漆"};
        for (const QString &typ : typeOrder) {
            // 找到该类型的主修人
            QString techName;
            for (auto &li : laborItems) {
                if (li.type == typ && !li.person.isEmpty()) {
                    techName = li.person;
                    break;
                }
            }

            // 收集该类型的项目
            QList<QPair<QString,double>> typeItems;
            for (auto &li : laborItems) {
                if (li.type != typ) continue;
                if (li.content.isEmpty() && li.fee == 0) continue;
                typeItems << QPair<QString,double>(li.content, li.fee);
            }
            if (typeItems.isEmpty()) continue;

            // 类型分组标题（机电/钣金/喷漆 + 主修人）
            laborHtml += QString(
                "<div style='font-weight:bold;padding:4px 8px;margin-top:6px;"
                "background:#eaf2f8;border:1px solid #d5dbdb;font-size:10pt;'>"
                "%1  |  主修人: %2</div>"
            ).arg(typ, F(techName));

            // 该类型的维修项目表格
            laborHtml += "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;margin-bottom:4px;'>"
                        "<tr style='background:#34495e;color:white;'>"
                        "<th>维修内容</th><th style='width:20%;'>费用</th></tr>";
            for (auto &item : typeItems) {
                laborHtml += QString("<tr><td>%1</td><td align='right'>%2</td></tr>")
                    .arg(F(item.first), Y(item.second));
            }
            laborHtml += "</table>";
        }
    }

    QString title = isSettle ? "维修结算单" : "工单明细单";
    QString advisorRow = advisor.isEmpty() ? "" : QString("<p><b>服务顾问：</b>%1</p>").arg(advisor);

    return QString(
        "<html><head><meta charset='utf-8'><style>"
        "body{font-family:'Microsoft YaHei','SimHei',sans-serif;font-size:11pt;color:#222;}"
        "h2{font-size:18pt;text-align:center;margin:6pt 0;color:#1a1a1a;}"
        ".sect-title{font-size:12pt;font-weight:bold;color:#2c3e50;background:#ecf0f1;"
        "padding:4pt 8pt;margin:10pt 0 4pt 0;border-left:4pt solid #2980b9;}"
        "table.info{width:100%;border-collapse:collapse;margin:3pt 0;}"
        "table.info td{padding:3pt 6pt;border:0.5pt solid #ddd;font-size:10pt;}"
        "table.info td.label{background:#f8f9fa;font-weight:bold;width:13%;}"
        "hr{border:none;border-top:1pt solid #bdc3c7;margin:6pt 0;}"
        ".footer{font-size:8pt;color:#888;text-align:center;margin-top:10pt;}"
        "</style></head><body>"
        "<h2>%1</h2><hr>"
        // 基本信息
        "<table class='info'>"
        "<tr><td class='label'>工单号</td><td>%2</td>"
        "<td class='label'>车牌号</td><td>%3</td>"
        "<td class='label'>状态</td><td>%4</td></tr>"
        "<tr><td class='label'>车型</td><td>%5</td>"
        "<td class='label'>车主</td><td>%6</td>"
        "<td class='label'>电话</td><td>%7</td></tr>"
        "<tr><td class='label'>公里数</td><td>%8 km</td>"
        "<td class='label'>报修日期</td><td>%9</td>"
        "<td class='label'>主修人</td><td>%10</td></tr>"
        "</table>"
        "%11"
        // 备件明细
        "<div class='sect-title'>一、备件明细</div>%12"
        // 工时费明细
        "<div class='sect-title'>二、工时费明细</div>%13"
        // 费用汇总
        "<div class='sect-title'>三、其他</div>"
        "<table class='info'>"
        "<tr><td class='label'>工时费</td><td>%14</td>"
        "<td class='label'>材料费</td><td>%15</td></tr>"
        "<tr><td class='label'>其它费</td><td>%16</td>"
        "<td class='label'>管理费</td><td>%17</td></tr>"
        "<tr><td class='label'>订金</td><td>%18</td>"
        "<td class='label' style='font-size:13pt;color:#c0392b;'>总计</td>"
        "<td style='font-size:13pt;font-weight:bold;color:#c0392b;'>%19</td></tr>"
        "</table>"
        "<hr><p class='footer'>打印时间：%20</p>"
        "</body></html>"
    ).arg(title,
          orderNo, F(plate), F(status),
          F(model), F(owner), F(phone),
          QString::number(mileage), repairDate, F(technicians),
          advisorRow,
          partsHtml, laborHtml,
          Y(laborFee), Y(matFee), Y(otherFee), Y(mgmtFee),
          Y(deposit), Y(total),
          QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
}

void SettlementPage::onPrintSettle()
{
    if (m_currentOrderId == 0) { QMessageBox::warning(this,"提示","请先加载工单"); return; }

    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString html = buildSettleHtml(
            m_cachedOrderNo, m_cachedPlate, m_cachedModel,
            m_cachedOwner, m_cachedPhone, m_cachedAdvisor,
            m_cachedMileage, m_cachedRepairDate, m_cachedTechnicians,
            m_cachedStatus,
            m_cachedLaborItems, m_cachedPartItems,
            m_cachedLaborFee, m_cachedMatFee, m_cachedOtherFee,
            m_cachedMgmtFee, m_cachedDeposit, m_cachedTotal, true);
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
        QString html = buildSettleHtml(
            m_cachedOrderNo, m_cachedPlate, m_cachedModel,
            m_cachedOwner, m_cachedPhone, m_cachedAdvisor,
            m_cachedMileage, m_cachedRepairDate, m_cachedTechnicians,
            m_cachedStatus,
            m_cachedLaborItems, m_cachedPartItems,
            m_cachedLaborFee, m_cachedMatFee, m_cachedOtherFee,
            m_cachedMgmtFee, m_cachedDeposit, m_cachedTotal, false);
        doc.setHtml(html);
        doc.print(p);
    });
    preview.exec();
}
