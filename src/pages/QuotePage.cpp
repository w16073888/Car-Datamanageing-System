#include "QuotePage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDialog>
#include <QTableWidget>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QFileDialog>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

QuotePage::QuotePage(QWidget *parent)
    : QWidget(parent)
    , m_currentOrderId(0)
{
    setupUI();
}

QuotePage::~QuotePage() {}

void QuotePage::refreshData()
{
    m_searchOrder->clear();
    m_lblVehicleInfo->setText("请搜索工单");
    m_textPartsInfo->clear();
    m_lblTotalPrice->setText("¥ 0.00");
    m_currentOrderId = 0;
    m_currentOrderNo.clear();
    m_currentStatus.clear();
    m_btnNotifyBilling->setVisible(false);
    m_btnSettle->setVisible(false);
    m_btnSavePdf->setVisible(false);
    m_btnPrint->setVisible(false);
}

void QuotePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("结算管理");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // ==================== 1. 查询工单 ====================
    QGroupBox *searchGroup = new QGroupBox("查询工单");
    QHBoxLayout *searchLayout = new QHBoxLayout(searchGroup);
    searchLayout->addWidget(new QLabel("工单号/车牌:"));
    m_searchOrder = new QLineEdit;
    m_searchOrder->setPlaceholderText("输入工单号或车牌号，回车搜索（已派工/待提单/已提单）");
    searchLayout->addWidget(m_searchOrder, 1);
    m_btnSearch = new QPushButton("搜索");
    m_btnSearch->setStyleSheet("padding:6px 14px;background:#3498db;color:#fff;border-radius:3px;font-weight:bold;");
    m_btnSearch->setMinimumHeight(28);
    searchLayout->addWidget(m_btnSearch);
    mainLayout->addWidget(searchGroup);

    // ==================== 2. 状态显示区域 ====================
    QGroupBox *infoGroup = new QGroupBox("工单信息");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    m_lblVehicleInfo = new QLabel("请搜索工单");
    m_lblVehicleInfo->setStyleSheet(
        "padding:10px;background:#f0f3f5;border-radius:4px;font-size:13px;"
        "border:1px solid #dcdde1;");
    m_lblVehicleInfo->setWordWrap(true);
    m_lblVehicleInfo->setMinimumHeight(60);
    infoLayout->addWidget(m_lblVehicleInfo);

    infoLayout->addWidget(new QLabel("已使用备件明细:"));
    m_textPartsInfo = new QTextEdit;
    m_textPartsInfo->setReadOnly(true);
    m_textPartsInfo->setMaximumHeight(180);
    m_textPartsInfo->setStyleSheet("background:#fff;font-size:12px;");
    infoLayout->addWidget(m_textPartsInfo);

    m_lblTotalPrice = new QLabel("¥ 0.00");
    m_lblTotalPrice->setStyleSheet(
        "font-size:22px;font-weight:bold;color:#e74c3c;"
        "padding:10px;background:#fdf2f2;border-radius:4px;"
        "border:2px solid #e74c3c;");
    m_lblTotalPrice->setAlignment(Qt::AlignCenter);
    infoLayout->addWidget(m_lblTotalPrice);

    mainLayout->addWidget(infoGroup, 1);

    // ==================== 3. 操作按钮 ====================
    QHBoxLayout *btnLayout = new QHBoxLayout;

    m_btnNotifyBilling = new QPushButton("通知提单");
    m_btnNotifyBilling->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#e67e22;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#d35400;}");
    m_btnNotifyBilling->setMinimumHeight(40);
    m_btnNotifyBilling->setVisible(false);

    m_btnSettle = new QPushButton("结算");
    m_btnSettle->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#27ae60;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#219a52;}");
    m_btnSettle->setMinimumHeight(40);
    m_btnSettle->setVisible(false);

    m_btnSavePdf = new QPushButton("保存到PDF");
    m_btnSavePdf->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#8e44ad;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#7d3c98;}");
    m_btnSavePdf->setMinimumHeight(40);
    m_btnSavePdf->setVisible(false);

    m_btnPrint = new QPushButton("打印结算单");
    m_btnPrint->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#2980b9;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#1f6fa5;}");
    m_btnPrint->setMinimumHeight(40);
    m_btnPrint->setVisible(false);

    btnLayout->addStretch();
    btnLayout->addWidget(m_btnNotifyBilling);
    btnLayout->addWidget(m_btnSettle);
    btnLayout->addWidget(m_btnSavePdf);
    btnLayout->addWidget(m_btnPrint);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // ==================== 信号 ====================
    connect(m_btnSearch, &QPushButton::clicked, this, &QuotePage::onOrderSearch);
    connect(m_searchOrder, &QLineEdit::returnPressed, this, &QuotePage::onOrderSearch);
    connect(m_btnNotifyBilling, &QPushButton::clicked, this, &QuotePage::onNotifyBilling);
    connect(m_btnSettle, &QPushButton::clicked, this, &QuotePage::onSettle);
    connect(m_btnSavePdf, &QPushButton::clicked, this, &QuotePage::onSaveToPdf);
    connect(m_btnPrint, &QPushButton::clicked, this, &QuotePage::onPrintSettlement);
}

// ============================================================
// 查询工单 — 等价于库房工作台备件领取的查询工单功能
// ============================================================
void QuotePage::onOrderSearch()
{
    QString text = m_searchOrder->text().trimmed();
    if (text.isEmpty()) return;

    QString kw = text;
    kw.replace("'", "''");

    QSqlQuery q(DbManager::instance().database());
    q.prepare(QString(
        "SELECT w.id, w.order_no, w.status, COALESCE(v.plate_number,'') AS plate, "
        "w.repair_content, w.created_at "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE w.status IN ('已派工','待提单','已提单') "
        "AND (w.order_no LIKE '%%1%' OR v.plate_number LIKE '%%1%') "
        "ORDER BY w.id DESC LIMIT 30").arg(kw));
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        QMessageBox::information(this, "未找到", "未找到匹配的工单（已派工/待提单/已提单）");
        return;
    }
    q.seek(-1);

    QList<QStringList> rows;
    while (q.next()) {
        QStringList row;
        row << q.value(1).toString()  // order_no
            << q.value(2).toString()  // status
            << q.value(3).toString()  // plate
            << q.value(4).toString()  // repair_content
            << q.value(5).toDateTime().toString("yyyy-MM-dd HH:mm");
        rows << row;
    }

    // 唯一匹配 → 直接锁定
    if (rows.size() == 1) {
        m_searchOrder->setText(rows[0][0]);
        loadOrderInfo(rows[0][0]);
        return;
    }

    // 多结果 → 弹窗选择
    QDialog dlg(this);
    dlg.setWindowTitle("选择工单 — 已派工 / 待提单 / 已提单");
    dlg.resize(680, 340);

    QVBoxLayout *dl = new QVBoxLayout(&dlg);
    QTableWidget *tbl = new QTableWidget;
    tbl->setColumnCount(5);
    tbl->setHorizontalHeaderLabels({"工单号", "状态", "车牌号", "报修内容", "创建时间"});
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setSelectionMode(QAbstractItemView::SingleSelection);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->verticalHeader()->setVisible(false);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tbl->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tbl->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    tbl->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");
    tbl->setAlternatingRowColors(true);

    tbl->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++) {
        tbl->setItem(i, 0, new QTableWidgetItem(rows[i][0]));
        tbl->setItem(i, 1, new QTableWidgetItem(rows[i][1]));
        tbl->setItem(i, 2, new QTableWidgetItem(rows[i][2]));
        tbl->setItem(i, 3, new QTableWidgetItem(rows[i][3]));
        tbl->setItem(i, 4, new QTableWidgetItem(rows[i][4]));
    }

    dl->addWidget(tbl, 1);
    QHBoxLayout *bb = new QHBoxLayout;
    QPushButton *ok = new QPushButton("选择");
    ok->setStyleSheet("padding:6px 14px;background:#3498db;color:#fff;border-radius:3px;font-weight:bold;");
    QPushButton *ca = new QPushButton("取消");
    ca->setStyleSheet("padding:6px 14px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;");
    bb->addStretch(); bb->addWidget(ok); bb->addWidget(ca);
    dl->addLayout(bb);

    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(ca, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(tbl, &QTableWidget::cellDoubleClicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted && tbl->currentRow() >= 0) {
        int r = tbl->currentRow();
        m_searchOrder->setText(rows[r][0]);
        loadOrderInfo(rows[r][0]);
    }
}

// ============================================================
// 加载工单详细信息
// ============================================================
void QuotePage::loadOrderInfo(const QString &orderNo)
{
    m_currentOrderNo = orderNo;
    QSqlQuery q(DbManager::instance().database());

    // 1. 工单 + 车辆 + 车主信息
    q.prepare(
        "SELECT w.id, w.order_no, w.status, w.labor_fee, w.material_fee, "
        "  w.other_fee, w.management_fee, w.total_amount, w.deposit, "
        "  w.repair_content, w.created_at, "
        "  v.plate_number, v.vin, v.model, v.engine_number, "
        "  COALESCE(c.name,''), COALESCE(c.phone,''), COALESCE(c.address,'') "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id "
        "WHERE w.order_no = :no");
    q.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }

    m_currentOrderId = q.value(0).toInt();
    m_currentStatus = q.value(2).toString();
    double laborFee = q.value(3).toDouble();
    double materialFee = q.value(4).toDouble();
    double otherFee = q.value(5).toDouble();
    double mgmtFee = q.value(6).toDouble();
    double totalAmount = q.value(7).toDouble();
    double deposit = q.value(8).toDouble();
    QString repairContent = q.value(9).toString();
    QString createdAt = q.value(10).toDateTime().toString("yyyy-MM-dd HH:mm");
    QString plate = q.value(11).toString();
    QString vin = q.value(12).toString();
    QString model = q.value(13).toString();
    QString engine = q.value(14).toString();
    QString ownerName = q.value(15).toString();
    QString ownerPhone = q.value(16).toString();
    QString ownerAddr = q.value(17).toString();

    // 车辆 + 车主信息
    QString vehicleHtml = QString(
        "<table width='100%%' cellspacing='4' style='font-size:13px;'>"
        "<tr><td width='15%%'><b>工单号:</b></td><td width='35%%'>%1</td>"
        "<td width='15%%'><b>状态:</b></td><td width='35%%' style='color:#e67e22;font-weight:bold;'>%2</td></tr>"
        "<tr><td><b>车牌号:</b></td><td>%3</td>"
        "<td><b>车型:</b></td><td>%4</td></tr>"
        "<tr><td><b>VIN码:</b></td><td>%5</td>"
        "<td><b>发动机号:</b></td><td>%6</td></tr>"
        "<tr><td><b>车主:</b></td><td>%7</td>"
        "<td><b>电话:</b></td><td>%8</td></tr>"
        "<tr><td><b>地址:</b></td><td colspan='3'>%9</td></tr>"
        "<tr><td><b>报修内容:</b></td><td colspan='3'>%10</td></tr>"
        "<tr><td><b>创建时间:</b></td><td colspan='3'>%11</td></tr>"
        "</table>")
        .arg(orderNo, m_currentStatus, plate, model, vin, engine,
             ownerName, ownerPhone, ownerAddr, repairContent, createdAt);
    m_lblVehicleInfo->setText(vehicleHtml);

    // 2. 实际使用备件明细
    QSqlQuery pq(DbManager::instance().database());
    pq.prepare(
        "SELECT wi.part_name, COUNT(*) AS qty, wi.unit_price, SUM(wi.subtotal) AS subtotal "
        "FROM t_workorder_item wi "
        "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
        "GROUP BY wi.part_name, wi.unit_price "
        "ORDER BY wi.part_name");
    pq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(pq);

    QString partsText;
    double partsTotal = 0;
    while (pq.next()) {
        QString name = pq.value(0).toString();
        int qty = pq.value(1).toInt();
        double price = pq.value(2).toDouble();
        double sub = pq.value(3).toDouble();
        partsTotal += sub;
        partsText += QString("%1  ×%2  @¥%3  = ¥%4\n")
                     .arg(name).arg(qty).arg(price, 0, 'f', 2).arg(sub, 0, 'f', 2);
    }
    if (partsText.isEmpty()) {
        partsText = "（暂无备件使用记录）\n";
    }

    // 费用汇总
    double grandTotal = laborFee + partsTotal + otherFee + mgmtFee;
    partsText += QString(
        "\n━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "工时费: ¥%1 | 材料费: ¥%2 | 其它费: ¥%3 | 管理费: ¥%4\n"
        "订金(已收): ¥%5 | 应收合计: ¥%6 | 应付尾款: ¥%7")
        .arg(laborFee, 0, 'f', 2)
        .arg(partsTotal, 0, 'f', 2)
        .arg(otherFee, 0, 'f', 2)
        .arg(mgmtFee, 0, 'f', 2)
        .arg(deposit, 0, 'f', 2)
        .arg(grandTotal, 0, 'f', 2)
        .arg(grandTotal - deposit, 0, 'f', 2);

    m_textPartsInfo->setPlainText(partsText);

    // 3. 总价格（强调）
    m_lblTotalPrice->setText(QString("¥ %1").arg(grandTotal, 0, 'f', 2));

    // 4. 根据状态显示操作按钮
    updateActionButtons(m_currentStatus);
}

// ============================================================
// 根据状态显示/隐藏按钮
// ============================================================
void QuotePage::updateActionButtons(const QString &status)
{
    m_btnNotifyBilling->setVisible(status == "已派工");
    m_btnSettle->setVisible(status == "已提单");
    m_btnSavePdf->setVisible(status == "已提单");
    m_btnPrint->setVisible(status == "已提单");
}

// ============================================================
// 通知提单: 已派工 → 待提单
// ============================================================
void QuotePage::onNotifyBilling()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "确认通知提单",
            QString("确认向库房发送提单通知？\n\n工单号: %1\n当前状态: 已派工 → 待提单\n\n"
                    "库房收到通知后将进行材料审核提单。")
            .arg(m_currentOrderNo),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DbManager::instance().database());
    q.prepare("UPDATE t_workorder SET status = '待提单' WHERE id = :id AND status = '已派工'");
    q.bindValue(":id", m_currentOrderId);
    if (DbManager::instance().executeQuery(q) && q.numRowsAffected() > 0) {
        m_currentStatus = "待提单";
        updateActionButtons(m_currentStatus);
        // 刷新信息显示
        loadOrderInfo(m_currentOrderNo);
        QMessageBox::information(this, "成功", "已通知库房提单，状态更新为「待提单」");
    } else {
        QMessageBox::warning(this, "失败", "状态更新失败，工单可能已被其他人操作");
    }
}

// ============================================================
// 结算: 已提单 → 已结算
// ============================================================
void QuotePage::onSettle()
{
    if (m_currentOrderId == 0) return;

    // 重新计算总金额
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
              "WHERE workorder_id = :oid AND item_type = '材料'");
    q.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(q);
    double matTotal = q.next() ? q.value(0).toDouble() : 0;

    q.prepare("SELECT COALESCE(labor_fee,0), COALESCE(other_fee,0), "
              "COALESCE(management_fee,0), COALESCE(deposit,0) "
              "FROM t_workorder WHERE id = :id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);
    double laborFee = 0, otherFee = 0, mgmtFee = 0, deposit = 0;
    if (q.next()) {
        laborFee = q.value(0).toDouble();
        otherFee = q.value(1).toDouble();
        mgmtFee = q.value(2).toDouble();
        deposit = q.value(3).toDouble();
    }
    double grandTotal = laborFee + matTotal + otherFee + mgmtFee;

    if (QMessageBox::question(this, "确认结算",
            QString("确认结算此工单？\n\n工单号: %1\n应收合计: ¥%2\n已收订金: ¥%3\n应付尾款: ¥%4\n\n"
                    "结算后状态将变为「已结算」，不可撤销。")
            .arg(m_currentOrderNo)
            .arg(grandTotal, 0, 'f', 2)
            .arg(deposit, 0, 'f', 2)
            .arg(grandTotal - deposit, 0, 'f', 2),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // 查询工单附加信息（入厂时间、服务顾问、维修人员、报修项目）
    int vid = 0, mileage = 0;
    QString entryDate, svcAdvisor, techNames, repairItemsJson;
    q.prepare("SELECT w.vehicle_id, w.mileage, w.customer_service_id, "
              "w.repair_date, w.created_at, w.repair_content "
              "FROM t_workorder w WHERE w.id = :id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);
    if (q.next()) {
        vid = q.value(0).toInt();
        mileage = q.value(1).toInt();
        int svcId = q.value(2).toInt();
        entryDate = q.value(3).isNull() ? q.value(4).toDateTime().toString("yyyy-MM-dd HH:mm")
                                        : q.value(3).toDateTime().toString("yyyy-MM-dd HH:mm");
        // 服务顾问姓名
        if (svcId > 0) {
            QSqlQuery eq(DbManager::instance().database());
            eq.prepare("SELECT name FROM t_employee WHERE id = :eid");
            eq.bindValue(":eid", svcId);
            if (eq.exec() && eq.next()) svcAdvisor = eq.value(0).toString();
        }
    }

    // 维修人员（从报修条目中收集各类型技工姓名）
    QStringList techList;
    QSqlQuery tq(DbManager::instance().database());
    tq.prepare("SELECT DISTINCT repair_person FROM t_workorder_repair_item "
               "WHERE workorder_id = :oid AND repair_person IS NOT NULL AND repair_person != ''");
    tq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(tq);
    while (tq.next()) techList << tq.value(0).toString();
    techNames = techList.join(", ");

    // 报修项目明细 JSON
    QStringList repairJsonParts;
    QSqlQuery rq(DbManager::instance().database());
    rq.prepare("SELECT item_type, repair_content, fee FROM t_workorder_repair_item "
               "WHERE workorder_id = :oid ORDER BY item_type");
    rq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(rq);
    while (rq.next()) {
        repairJsonParts << QString("{\"type\":\"%1\",\"content\":\"%2\",\"fee\":%3}")
                         .arg(rq.value(0).toString(),
                              rq.value(1).toString().replace("\"", "\\\""),
                              QString::number(rq.value(2).toDouble(), 'f', 2));
    }
    repairItemsJson = "[" + repairJsonParts.join(",") + "]";

    // 备件摘要
    QStringList partSummaryList;
    QSqlQuery psq(DbManager::instance().database());
    psq.prepare("SELECT part_name, COUNT(*), unit_price FROM t_workorder_item "
                "WHERE workorder_id = :oid AND item_type = '材料' "
                "GROUP BY part_name, unit_price");
    psq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(psq);
    while (psq.next())
        partSummaryList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
    QString partsSummary = partSummaryList.join(", ");

    // 报修项目文字摘要
    QStringList repairSummaryParts;
    QSqlQuery rsq(DbManager::instance().database());
    rsq.prepare("SELECT item_type, repair_content, fee FROM t_workorder_repair_item "
                "WHERE workorder_id = :oid ORDER BY item_type");
    rsq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(rsq);
    while (rsq.next())
        repairSummaryParts << QString("[%1] %2 ¥%3").arg(rsq.value(0).toString(),
            rsq.value(1).toString(), QString::number(rsq.value(2).toDouble(), 'f', 2));
    QString repairSummary = repairSummaryParts.join("; ");

    // 累计消费
    double cumulative = grandTotal;
    {
        QSqlQuery cq(DbManager::instance().database());
        cq.prepare("SELECT COALESCE(SUM(total_amount),0) FROM t_maintenance_history WHERE vehicle_id=:v");
        cq.bindValue(":v", vid);
        if (cq.exec() && cq.next()) cumulative = cq.value(0).toDouble() + grandTotal;
    }

    // 写入维修历史
    QSqlQuery ih(DbManager::instance().database());
    ih.prepare("INSERT INTO t_maintenance_history "
               "(vehicle_id, workorder_id, maintenance_date, entry_date, completion_date, "
               "mileage, service_advisor, technicians, total_amount, cumulative_amount, "
               "labor_fee, material_fee, other_fee, management_fee, deposit, "
               "parts_summary, repair_summary, repair_items) "
               "VALUES (:vid, :woid, NOW(), :entry, NOW(), :mile, :svc, :tech, "
               ":total, :cum, :labor, :mat, :oth, :mgmt, :dep, :parts, :repair, :ritems)");
    ih.bindValue(":vid", vid);
    ih.bindValue(":woid", m_currentOrderId);
    ih.bindValue(":entry", entryDate.isEmpty() ? QVariant() : entryDate);
    ih.bindValue(":mile", mileage);
    ih.bindValue(":svc", svcAdvisor.isEmpty() ? QVariant() : svcAdvisor);
    ih.bindValue(":tech", techNames.isEmpty() ? QVariant() : techNames);
    ih.bindValue(":total", grandTotal);
    ih.bindValue(":cum", cumulative);
    ih.bindValue(":labor", laborFee);
    ih.bindValue(":mat", matTotal);
    ih.bindValue(":oth", otherFee);
    ih.bindValue(":mgmt", mgmtFee);
    ih.bindValue(":dep", deposit);
    ih.bindValue(":parts", partsSummary.isEmpty() ? QVariant() : partsSummary);
    ih.bindValue(":repair", repairSummary.isEmpty() ? QVariant() : repairSummary);
    ih.bindValue(":ritems", repairItemsJson.isEmpty() ? QVariant() : repairItemsJson);
    DbManager::instance().executeQuery(ih);

    // 更新车辆最后保养日期
    if (vid > 0) {
        QSqlQuery vu(DbManager::instance().database());
        vu.prepare("UPDATE t_vehicle SET last_maintenance_date = CURDATE() WHERE id = :v");
        vu.bindValue(":v", vid);
        DbManager::instance().executeQuery(vu);
    }

    q.prepare("UPDATE t_workorder SET status = '已结算', total_amount = :total "
              "WHERE id = :id AND status = '已提单'");
    q.bindValue(":total", grandTotal);
    q.bindValue(":id", m_currentOrderId);
    if (DbManager::instance().executeQuery(q) && q.numRowsAffected() > 0) {
        m_currentStatus = "已结算";
        updateActionButtons(m_currentStatus);
        loadOrderInfo(m_currentOrderNo);
        QMessageBox::information(this, "结算成功",
            QString("工单 %1 已结算\n应收合计: ¥%2")
            .arg(m_currentOrderNo).arg(grandTotal, 0, 'f', 2));
    } else {
        QMessageBox::warning(this, "结算失败", "状态更新失败，请确认工单状态为「已提单」");
    }
}

// ============================================================
// 构建结算单HTML
// ============================================================
QString QuotePage::buildSettlementHtml() const
{
    if (m_currentOrderId == 0) return QString();

    QSqlQuery q(DbManager::instance().database());
    q.prepare(
        "SELECT w.order_no, w.status, w.labor_fee, w.material_fee, "
        "  w.other_fee, w.management_fee, w.total_amount, w.deposit, "
        "  w.repair_content, w.created_at, w.repair_date, w.estimated_date, "
        "  v.plate_number, v.vin, v.model, v.engine_number, "
        "  COALESCE(c.name,''), COALESCE(c.phone,''), COALESCE(c.address,'') "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id "
        "WHERE w.id = :id");
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) return QString();

    QString orderNo = q.value(0).toString();
    double laborFee = q.value(2).toDouble();
    double otherFee = q.value(4).toDouble();
    double mgmtFee = q.value(5).toDouble();
    double deposit = q.value(7).toDouble();
    QString plate = q.value(12).toString();
    QString model = q.value(13).toString();
    QString ownerName = q.value(15).toString();
    QString ownerPhone = q.value(16).toString();
    QString ownerAddr = q.value(17).toString();
    QString repairDate = q.value(10).toDate().toString("yyyy-MM-dd");

    // 备件明细
    QString partsRows;
    double partsTotal = 0;
    QSqlQuery pq(DbManager::instance().database());
    pq.prepare("SELECT part_name, COUNT(*), unit_price, SUM(subtotal) "
               "FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料' "
               "GROUP BY part_name, unit_price ORDER BY part_name");
    pq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(pq);
    while (pq.next()) {
        double sub = pq.value(3).toDouble();
        partsTotal += sub;
        partsRows += QString(
            "<tr><td>%1</td><td align='center'>%2</td>"
            "<td align='right'>¥%3</td><td align='right'>¥%4</td></tr>")
            .arg(pq.value(0).toString())
            .arg(pq.value(1).toInt())
            .arg(pq.value(2).toDouble(), 0, 'f', 2)
            .arg(sub, 0, 'f', 2);
    }

    double grandTotal = laborFee + partsTotal + otherFee + mgmtFee;

    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
        "body{font-family:'Microsoft YaHei',sans-serif;font-size:13px;margin:30px;}"
        "h2{text-align:center;margin-bottom:5px;}"
        ".sub{text-align:center;color:#7f8c8d;margin-bottom:15px;font-size:12px;}"
        ".info-table, .parts-table{width:100%%;border-collapse:collapse;margin-bottom:15px;}"
        ".info-table td{padding:6px 10px;border:1px solid #ddd;}"
        ".info-table .label{background:#f5f6fa;font-weight:bold;width:18%%;}"
        ".parts-table th{background:#34495e;color:#fff;padding:8px;text-align:center;}"
        ".parts-table td{padding:6px 10px;border:1px solid #ddd;}"
        ".total{text-align:right;font-size:16px;font-weight:bold;color:#e74c3c;"
        "margin-top:15px;padding:10px;border-top:2px solid #333;}"
        ".footer{margin-top:30px;text-align:center;color:#95a5a6;font-size:11px;}"
        "</style></head><body>"
        "<h2>汽车维修结算单</h2>"
        "<p class='sub'>工单号: %1 | 报修日期: %2 | 结算日期: %3</p>"
        "<table class='info-table'>"
        "<tr><td class='label'>车牌号</td><td>%4</td>"
        "<td class='label'>车型</td><td>%5</td></tr>"
        "<tr><td class='label'>车主</td><td>%6</td>"
        "<td class='label'>电话</td><td>%7</td></tr>"
        "<tr><td class='label'>地址</td><td colspan='3'>%8</td></tr>"
        "</table>"
        "<h3>费用明细</h3>"
        "<table class='parts-table'>"
        "<tr><th>备件名称</th><th>数量</th><th>单价</th><th>小计</th></tr>"
        "%9"
        "<tr style='background:#f5f6fa;'><td colspan='2'></td>"
        "<td align='right'><b>材料费合计</b></td>"
        "<td align='right'><b>¥%10</b></td></tr>"
        "<tr><td colspan='2'></td><td align='right'><b>工时费</b></td>"
        "<td align='right'><b>¥%11</b></td></tr>"
        "<tr><td colspan='2'></td><td align='right'><b>其它费</b></td>"
        "<td align='right'><b>¥%12</b></td></tr>"
        "<tr><td colspan='2'></td><td align='right'><b>管理费</b></td>"
        "<td align='right'><b>¥%13</b></td></tr>"
        "<tr><td colspan='2'></td><td align='right'><b>已收订金</b></td>"
        "<td align='right'><b>¥%14</b></td></tr>"
        "</table>"
        "<p class='total'>应收合计: ¥%15 | 应付尾款: ¥%16</p>"
        "<p class='footer'>打印时间: %17</p>"
        "</body></html>")
        .arg(orderNo, repairDate,
             QDate::currentDate().toString("yyyy-MM-dd"),
             plate, model, ownerName, ownerPhone, ownerAddr,
             partsRows)
        .arg(partsTotal, 0, 'f', 2)
        .arg(laborFee, 0, 'f', 2)
        .arg(otherFee, 0, 'f', 2)
        .arg(mgmtFee, 0, 'f', 2)
        .arg(deposit, 0, 'f', 2)
        .arg(grandTotal, 0, 'f', 2)
        .arg(grandTotal - deposit, 0, 'f', 2)
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    return html;
}

// ============================================================
// 保存到PDF
// ============================================================
void QuotePage::onSaveToPdf()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先搜索并选择工单");
        return;
    }
    if (m_currentStatus != "已提单") {
        QMessageBox::warning(this, "提示", "只有「已提单」状态的工单才能保存结算单");
        return;
    }

    QString defaultName = QString("结算单_%1.pdf").arg(m_currentOrderNo);
    QString filePath = QFileDialog::getSaveFileName(
        this, "保存结算单PDF", defaultName, "PDF 文件 (*.pdf)");
    if (filePath.isEmpty()) return;

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QTextDocument doc;
    doc.setHtml(buildSettlementHtml());
    doc.setPageSize(printer.pageLayout().paintRectPixels(printer.resolution()).size());
    doc.print(&printer);

    QMessageBox::information(this, "导出成功",
        QString("结算单已保存到:\n%1").arg(filePath));
}

// ============================================================
// 打印结算单
// ============================================================
void QuotePage::onPrintSettlement()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先搜索并选择工单");
        return;
    }
    if (m_currentStatus != "已提单") {
        QMessageBox::warning(this, "提示", "只有「已提单」状态的工单才能打印结算单");
        return;
    }

    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle("打印结算单");
    connect(&preview, &QPrintPreviewDialog::paintRequested, [this](QPrinter *p) {
        QTextDocument doc;
        doc.setHtml(buildSettlementHtml());
        doc.setPageSize(p->pageLayout().paintRectPixels(p->resolution()).size());
        doc.print(p);
    });
    preview.exec();
}
