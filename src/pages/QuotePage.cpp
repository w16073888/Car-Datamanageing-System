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
#include <QScrollArea>
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
    m_laborTable->setRowCount(0);
    m_partsTable->setRowCount(0);
    // 清空费用总计表的值列（row 0: col 1,3,5,7,9,11,13）
    for (int c = 1; c < 14; c += 2) {
        QTableWidgetItem *val = m_summaryTable->item(0, c);
        if (val) val->setText("¥0.00");
    }
    m_currentOrderId = 0;
    m_currentOrderNo.clear();
    m_currentStatus.clear();
    m_btnNotifyBilling->setVisible(false);
    m_btnCancelNotify->setVisible(false);
    m_btnSettle->setVisible(false);
    m_btnSavePdf->setVisible(false);
    m_btnPrint->setVisible(false);
}

void QuotePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("工单查询");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // ==================== 1. 查询工单 ====================
    QGroupBox *searchGroup = new QGroupBox("查询工单");
    QHBoxLayout *searchLayout = new QHBoxLayout(searchGroup);
    searchLayout->addWidget(new QLabel("工单号/车牌:"));
    m_searchOrder = new QLineEdit;
    m_searchOrder->setPlaceholderText("输入工单号或车牌号，回车搜索（已派工/待提单/已提单/已结算）");
    searchLayout->addWidget(m_searchOrder, 1);
    m_btnSearch = new QPushButton("搜索");
    m_btnSearch->setStyleSheet("padding:6px 14px;background:#3498db;color:#fff;border-radius:3px;font-weight:bold;");
    m_btnSearch->setMinimumHeight(28);
    searchLayout->addWidget(m_btnSearch);
    // ==================== 可滚动内容区域 ====================
    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    scrollLayout->addWidget(searchGroup);

    // ==================== 2. 状态显示区域 ====================
    QGroupBox *infoGroup = new QGroupBox("工单信息");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    m_lblVehicleInfo = new QLabel("请搜索工单");
    m_lblVehicleInfo->setStyleSheet(
        "padding:10px;background:#f0f3f5;border-radius:4px;font-size:13px;"
        "border:1px solid #dcdde1;");
    m_lblVehicleInfo->setWordWrap(true);
    m_lblVehicleInfo->setMinimumHeight(40);
    infoLayout->addWidget(m_lblVehicleInfo);

    infoLayout->addWidget(new QLabel("工单明细:"));
    // ---- 工时费明细表 ----
    QLabel *laborLabel = new QLabel("▸ 工时费明细");
    laborLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    infoLayout->addWidget(laborLabel);
    m_laborTable = new QTableWidget(0, 9);
    m_laborTable->setHorizontalHeaderLabels({"类别","主修人","维修内容","费用","","类别","主修人","维修内容","费用"});
    m_laborTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_laborTable->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_laborTable->verticalHeader()->setVisible(false);
    m_laborTable->horizontalHeader()->setStretchLastSection(true);
    // 左组: 类别/主修人 不压缩，维修内容/费用 压缩
    m_laborTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    // 分隔列
    m_laborTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_laborTable->horizontalHeader()->resizeSection(4, 6);
    // 右组: 同上
    m_laborTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);
    m_laborTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_laborTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_laborTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_laborTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    infoLayout->addWidget(m_laborTable);

    // ---- 材料明细表 ----
    QLabel *partsLabel = new QLabel("▸ 材料明细");
    partsLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    infoLayout->addWidget(partsLabel);
    m_partsTable = new QTableWidget(0, 13);
    m_partsTable->setHorizontalHeaderLabels({"材料名称","数量","成本","总成本","单价","总价","","材料名称","数量","成本","总成本","单价","总价"});
    m_partsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partsTable->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_partsTable->verticalHeader()->setVisible(false);
    m_partsTable->horizontalHeader()->setStretchLastSection(true);
    // 左组: 材料名称/总价 压缩，数量/成本/总成本/单价 不压缩
    m_partsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    // 分隔列
    m_partsTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_partsTable->horizontalHeader()->resizeSection(6, 6);
    // 右组: 同上
    m_partsTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(12, QHeaderView::Stretch);
    m_partsTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_partsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_partsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_partsTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    infoLayout->addWidget(m_partsTable);

    // ---- 费用总计表 ----
    QLabel *summaryLabel = new QLabel("▸ 费用总计");
    summaryLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    infoLayout->addWidget(summaryLabel);
    m_summaryTable = new QTableWidget(1, 14);
    m_summaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->horizontalHeader()->setVisible(false);
    m_summaryTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_summaryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_summaryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 全部 Stretch，14 列平分一行
    for (int c = 0; c < 14; c++)
        m_summaryTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    m_summaryTable->setMaximumHeight(50);
    m_summaryTable->setStyleSheet(
        "QTableWidget{background:#f8f9fa;border:1px solid #dcdde1;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    // 单行: 工时费合计|v|材料费合计|v|其他费|v|管理费|v|订金|v|应收合计|v|应付尾款|v
    struct { int col; QString label; bool highlight; } summaryFields[] = {
        {0,  "工时费合计", false},
        {2,  "材料费合计", false},
        {4,  "其他费",     false},
        {6,  "管理费",     false},
        {8,  "订金",       false},
        {10, "应收合计",   true},
        {12, "应付尾款",   true},
    };
    for (const auto &f : summaryFields) {
        QTableWidgetItem *item = new QTableWidgetItem(f.label);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        if (f.highlight) {
            item->setForeground(QColor("#e74c3c"));
            QFont font = item->font(); font.setBold(true); item->setFont(font);
        }
        m_summaryTable->setItem(0, f.col, item);
        QTableWidgetItem *val = new QTableWidgetItem("¥0.00");
        val->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        val->setFlags(val->flags() & ~Qt::ItemIsEditable);
        if (f.highlight) {
            val->setForeground(QColor("#e74c3c"));
            QFont font = val->font(); font.setBold(true); val->setFont(font);
        }
        m_summaryTable->setItem(0, f.col + 1, val);
    }
    infoLayout->addWidget(m_summaryTable);

    // ---- 费用编辑区（已派工/待提单时显示） ----
    QHBoxLayout *editFeeRow = new QHBoxLayout;
    editFeeRow->setSpacing(8);
    editFeeRow->addWidget(new QLabel("其他费:"));
    m_editOtherFee = new QDoubleSpinBox;
    m_editOtherFee->setRange(0, 999999.99);
    m_editOtherFee->setPrefix("¥ ");
    m_editOtherFee->setDecimals(2);
    editFeeRow->addWidget(m_editOtherFee);
    editFeeRow->addWidget(new QLabel("管理费:"));
    m_editMgmtFee = new QDoubleSpinBox;
    m_editMgmtFee->setRange(0, 999999.99);
    m_editMgmtFee->setPrefix("¥ ");
    m_editMgmtFee->setDecimals(2);
    editFeeRow->addWidget(m_editMgmtFee);
    editFeeRow->addStretch();
    m_btnSaveEdit = new QPushButton("保存修改");
    m_btnSaveEdit->setStyleSheet(
        "QPushButton{padding:6px 16px;border:none;border-radius:3px;"
        "background:#e67e22;color:#fff;font-weight:bold;}"
        "QPushButton:hover{background:#d35400;}");
    m_btnSaveEdit->setVisible(false);
    editFeeRow->addWidget(m_btnSaveEdit);
    infoLayout->addLayout(editFeeRow);

    scrollLayout->addWidget(infoGroup);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // ==================== 3. 操作按钮 ====================
    QHBoxLayout *btnLayout = new QHBoxLayout;

    m_btnNotifyBilling = new QPushButton("通知提单");
    m_btnNotifyBilling->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#e67e22;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#d35400;}");
    m_btnNotifyBilling->setMinimumHeight(40);
    m_btnNotifyBilling->setVisible(false);

    m_btnCancelNotify = new QPushButton("取消提单");
    m_btnCancelNotify->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#c0392b;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#a93226;}");
    m_btnCancelNotify->setMinimumHeight(40);
    m_btnCancelNotify->setVisible(false);

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
    btnLayout->addWidget(m_btnCancelNotify);
    btnLayout->addWidget(m_btnSettle);
    btnLayout->addWidget(m_btnSavePdf);
    btnLayout->addWidget(m_btnPrint);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // ==================== 信号 ====================
    connect(m_btnSearch, &QPushButton::clicked, this, &QuotePage::onOrderSearch);
    connect(m_searchOrder, &QLineEdit::returnPressed, this, &QuotePage::onOrderSearch);
    connect(m_btnNotifyBilling, &QPushButton::clicked, this, &QuotePage::onNotifyBilling);
    connect(m_btnCancelNotify, &QPushButton::clicked, this, &QuotePage::onCancelNotify);
    connect(m_btnSettle, &QPushButton::clicked, this, &QuotePage::onSettle);
    connect(m_btnSavePdf, &QPushButton::clicked, this, &QuotePage::onSaveToPdf);
    connect(m_btnPrint, &QPushButton::clicked, this, &QuotePage::onPrintSettlement);
    connect(m_btnSaveEdit, &QPushButton::clicked, this, &QuotePage::onSaveEdit);
    connect(m_editOtherFee, &QDoubleSpinBox::valueChanged,
            this, &QuotePage::onFeeEditChanged);
    connect(m_editMgmtFee, &QDoubleSpinBox::valueChanged,
            this, &QuotePage::onFeeEditChanged);
    connect(m_partsTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
        // 单价列编辑后自动刷新总价
        if (col == 4 || col == 11) {
            QTableWidgetItem *priceItem = m_partsTable->item(row, col);
            QTableWidgetItem *qtyItem   = m_partsTable->item(row, col - 3);  // 数量在单价左边
            QTableWidgetItem *subItem   = m_partsTable->item(row, col + 1);  // 总价在单价右边
            if (priceItem && qtyItem && subItem) {
                double price = priceItem->text().remove("¥").toDouble();
                int    qty   = qtyItem->text().toInt();
                subItem->setText(QString("¥%1").arg(qty * price, 0, 'f', 2));
            }
        }
        onFeeEditChanged();
    });
    connect(m_laborTable, &QTableWidget::cellChanged, this, [this](int, int) {
        onFeeEditChanged();
    });
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
        "WHERE w.status IN ('已派工','待提单','已提单','已结算') "
        "AND (w.order_no LIKE '%%1%' OR v.plate_number LIKE '%%1%') "
        "ORDER BY w.id DESC LIMIT 30").arg(kw));
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        QMessageBox::information(this, "未找到", "未找到匹配的工单（已派工/待提单/已提单/已结算）");
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
    dlg.setWindowTitle("选择工单 — 已派工 / 待提单 / 已提单 / 已结算");
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
        "  COALESCE(c.name,''), COALESCE(c.phone,''), COALESCE(c.address,''), "
        "  COALESCE(e.name,'') "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id "
        "LEFT JOIN t_employee e ON e.id = w.customer_service_id "
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
    double otherFee = q.value(5).toDouble();
    double mgmtFee = q.value(6).toDouble();
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
    QString svcAdvisor = q.value(18).toString();

    // 车辆 + 车主信息（两行）
    QString vehicleHtml = QString(
        "<table width='100%%' cellspacing='2' style='font-size:12px;'>"
        // Row 1: 工单号 / 状态 / 车牌号 / 车型 / VIN码 / 发动机号
        "<tr>"
        "<td><b>工单号:</b> %1</td>"
        "<td><b>状态:</b> <span style='color:#e67e22;font-weight:bold;'>%2</span></td>"
        "<td><b>车牌号:</b> %3</td>"
        "<td><b>车型:</b> %4</td>"
        "<td><b>VIN码:</b> %5</td>"
        "<td><b>发动机号:</b> %6</td>"
        "</tr>"
        // Row 2: 车主 / 电话 / 服务顾问 / 报修内容(colspan 2) / 创建时间
        "<tr>"
        "<td><b>车主:</b> %7</td>"
        "<td><b>电话:</b> %8</td>"
        "<td><b>服务顾问:</b> %9</td>"
        "<td colspan='2'><b>报修内容:</b> %10</td>"
        "<td><b>创建:</b> %11</td>"
        "</tr>"
        "</table>")
        .arg(orderNo, m_currentStatus, plate, model, vin, engine,
             ownerName, ownerPhone, svcAdvisor, repairContent, createdAt);
    m_lblVehicleInfo->setText(vehicleHtml);

    // 2. 工时费明细表 — 先收集全部条目，再按"先左列后右列"填充
    QSqlQuery lq(DbManager::instance().database());
    lq.prepare("SELECT id, item_type, repair_person, repair_content, fee "
               "FROM t_workorder_repair_item "
               "WHERE workorder_id = :oid ORDER BY item_type, id");
    lq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(lq);

    struct LaborItem { int id; QString typ, person, content; double fee; };
    QList<LaborItem> laborItems;
    double laborFromItems = 0;
    while (lq.next()) {
        LaborItem it;
        it.id      = lq.value(0).toInt();
        it.typ     = lq.value(1).toString();
        it.person  = lq.value(2).toString();
        it.content = lq.value(3).toString().trimmed();
        it.fee     = lq.value(4).toDouble();
        laborFromItems += it.fee;
        laborItems.append(it);
    }
    double displayLabor = (laborFee > 0) ? laborFee : laborFromItems;

    int laborRowCount = (laborItems.size() + 1) / 2;  // ceil(N/2)
    m_laborTable->setRowCount(laborRowCount);
    bool canEdit = (m_currentStatus == "已派工" || m_currentStatus == "待提单");
    for (int i = 0; i < laborItems.size(); i++) {
        int row = (i < laborRowCount) ? i : (i - laborRowCount);
        int colBase = (i < laborRowCount) ? 0 : 5;
        const LaborItem &it = laborItems[i];

        QTableWidgetItem *typItem = new QTableWidgetItem(it.typ);
        typItem->setFlags(typItem->flags() & ~Qt::ItemIsEditable);
        typItem->setData(Qt::UserRole, it.id);
        m_laborTable->setItem(row, colBase + 0, typItem);

        QTableWidgetItem *personItem = new QTableWidgetItem(it.person.isEmpty() ? "-" : it.person);
        personItem->setFlags(personItem->flags() & ~Qt::ItemIsEditable);
        m_laborTable->setItem(row, colBase + 1, personItem);

        QTableWidgetItem *contentItem = new QTableWidgetItem(it.content.isEmpty() ? "-" : it.content);
        contentItem->setFlags(contentItem->flags() & ~Qt::ItemIsEditable);
        m_laborTable->setItem(row, colBase + 2, contentItem);

        QTableWidgetItem *feeItem = new QTableWidgetItem(QString("¥%1").arg(it.fee, 0, 'f', 2));
        if (!canEdit) feeItem->setFlags(feeItem->flags() & ~Qt::ItemIsEditable);
        m_laborTable->setItem(row, colBase + 3, feeItem);
    }

    // 3. 材料明细表 — 先收集全部条目，再按"先左列后右列"填充
    QSqlQuery pq(DbManager::instance().database());
    pq.prepare(
        "SELECT wi.part_name, COUNT(*) AS qty, wi.unit_price, SUM(wi.subtotal) AS subtotal, "
        "  COALESCE(MAX(p.purchase_price), 0) AS cost "
        "FROM t_workorder_item wi "
        "LEFT JOIN t_parts p ON p.id = wi.part_id "
        "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
        "GROUP BY wi.part_name, wi.unit_price "
        "ORDER BY wi.part_name");
    pq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(pq);

    struct PartItem { QString name; int qty; double cost, price, sub; };
    QList<PartItem> partItems;
    double partsTotal = 0;
    while (pq.next()) {
        PartItem it;
        it.name  = pq.value(0).toString();
        it.qty   = pq.value(1).toInt();
        it.price = pq.value(2).toDouble();
        it.sub   = pq.value(3).toDouble();
        it.cost  = pq.value(4).toDouble();
        partsTotal += it.sub;
        partItems.append(it);
    }

    int partsRowCount = (partItems.size() + 1) / 2;
    m_partsTable->setRowCount(partsRowCount);
    for (int i = 0; i < partItems.size(); i++) {
        int row = (i < partsRowCount) ? i : (i - partsRowCount);
        int colBase = (i < partsRowCount) ? 0 : 7;
        const PartItem &it = partItems[i];

        QTableWidgetItem *nameItem = new QTableWidgetItem(it.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setData(Qt::UserRole, it.name);  // 存 part_name 用于保存
        m_partsTable->setItem(row, colBase + 0, nameItem);

        QTableWidgetItem *qtyItem = new QTableWidgetItem(QString::number(it.qty));
        qtyItem->setFlags(qtyItem->flags() & ~Qt::ItemIsEditable);
        m_partsTable->setItem(row, colBase + 1, qtyItem);

        // 成本 = 进货价（仅展示，不可编辑）
        QTableWidgetItem *costItem = new QTableWidgetItem(
            it.cost > 0 ? QString("¥%1").arg(it.cost, 0, 'f', 2) : "-");
        costItem->setFlags(costItem->flags() & ~Qt::ItemIsEditable);
        m_partsTable->setItem(row, colBase + 2, costItem);

        // 总成本 = 成本 × 数量（仅展示，不可编辑）
        QTableWidgetItem *totalCostItem = new QTableWidgetItem(
            it.cost > 0 ? QString("¥%1").arg(it.cost * it.qty, 0, 'f', 2) : "-");
        totalCostItem->setFlags(totalCostItem->flags() & ~Qt::ItemIsEditable);
        m_partsTable->setItem(row, colBase + 3, totalCostItem);

        QTableWidgetItem *priceItem = new QTableWidgetItem(QString("¥%1").arg(it.price, 0, 'f', 2));
        if (!canEdit) priceItem->setFlags(priceItem->flags() & ~Qt::ItemIsEditable);
        m_partsTable->setItem(row, colBase + 4, priceItem);

        QTableWidgetItem *subItem = new QTableWidgetItem(QString("¥%1").arg(it.sub, 0, 'f', 2));
        subItem->setFlags(subItem->flags() & ~Qt::ItemIsEditable);
        m_partsTable->setItem(row, colBase + 5, subItem);
    }

    // 4. 费用总计表
    double grandTotal = displayLabor + partsTotal + otherFee + mgmtFee;
    double unpaid = grandTotal - deposit;
    // 单行: 工时费(col1) 材料费(col3) 其他费(col5) 管理费(col7) 订金(col9) 应收合计(col11) 应付尾款(col13)
    struct { int c; double v; } feeMap[] = {
        {1,  displayLabor}, {3,  partsTotal}, {5,  otherFee}, {7,  mgmtFee},
        {9,  deposit},      {11, grandTotal}, {13, unpaid},
    };
    for (const auto &f : feeMap) {
        QTableWidgetItem *val = m_summaryTable->item(0, f.c);
        if (val) val->setText(QString("¥%1").arg(f.v, 0, 'f', 2));
    }

    // 5. 填充费用编辑控件并控制编辑权限
    m_editOtherFee->blockSignals(true);
    m_editOtherFee->setValue(otherFee);
    m_editOtherFee->blockSignals(false);
    m_editMgmtFee->blockSignals(true);
    m_editMgmtFee->setValue(mgmtFee);
    m_editMgmtFee->blockSignals(false);
    m_editOtherFee->setEnabled(canEdit);
    m_editMgmtFee->setEnabled(canEdit);
    m_btnSaveEdit->setVisible(canEdit);

    // 6. 根据状态显示操作按钮
    updateActionButtons(m_currentStatus);
}

// ============================================================
// 根据状态显示/隐藏按钮
// ============================================================
void QuotePage::updateActionButtons(const QString &status)
{
    m_btnNotifyBilling->setVisible(status == "已派工");
    m_btnCancelNotify->setVisible(status == "待提单");
    m_btnSettle->setVisible(status == "已提单");
    m_btnSavePdf->setVisible(status == "已提单" || status == "已结算");
    m_btnPrint->setVisible(status == "已提单" || status == "已结算");
}

// ============================================================
// 费用编辑变化 → 刷新费用总计
// ============================================================
void QuotePage::onFeeEditChanged()
{
    // 汇总工时费
    double laborTotal = 0;
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        for (int colBase : {0, 5}) {
            QTableWidgetItem *feeItem = m_laborTable->item(r, colBase + 3);
            if (feeItem)
                laborTotal += feeItem->text().remove("¥").toDouble();
        }
    }

    // 汇总材料费
    double partsTotal = 0;
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        for (int colBase : {0, 7}) {
            QTableWidgetItem *subItem = m_partsTable->item(r, colBase + 5);
            if (subItem)
                partsTotal += subItem->text().remove("¥").toDouble();
        }
    }

    double otherFee = m_editOtherFee->value();
    double mgmtFee  = m_editMgmtFee->value();
    double deposit  = 0;
    {
        QTableWidgetItem *depItem = m_summaryTable->item(0, 9);
        if (depItem) deposit = depItem->text().remove("¥").toDouble();
    }

    double grandTotal = laborTotal + partsTotal + otherFee + mgmtFee;
    double unpaid = grandTotal - deposit;

    struct { int c; double v; } feeMap[] = {
        {1, laborTotal}, {3, partsTotal}, {5, otherFee}, {7, mgmtFee},
        {9, deposit},    {11, grandTotal}, {13, unpaid},
    };
    for (const auto &f : feeMap) {
        QTableWidgetItem *val = m_summaryTable->item(0, f.c);
        if (val) val->setText(QString("¥%1").arg(f.v, 0, 'f', 2));
    }
}

// ============================================================
// 保存修改到数据库
// ============================================================
void QuotePage::onSaveEdit()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "确认保存",
            "确认将费用修改保存到数据库？\n\n工单号: " + m_currentOrderNo,
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DbManager::instance().database());

    // 1. 更新工时费明细（t_workorder_repair_item.fee）
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        for (int colBase : {0, 5}) {
            QTableWidgetItem *idItem  = m_laborTable->item(r, colBase + 0);
            QTableWidgetItem *feeItem = m_laborTable->item(r, colBase + 3);
            if (!idItem || !feeItem) continue;
            int repairId = idItem->data(Qt::UserRole).toInt();
            if (repairId == 0) continue;
            double newFee = feeItem->text().remove("¥").toDouble();
            q.prepare("UPDATE t_workorder_repair_item SET fee = :f WHERE id = :id");
            q.bindValue(":f", newFee);
            q.bindValue(":id", repairId);
            DbManager::instance().executeQuery(q);
        }
    }

    // 2. 更新材料单价（t_workorder_item.unit_price）
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        for (int colBase : {0, 7}) {
            QTableWidgetItem *nameItem  = m_partsTable->item(r, colBase + 0);
            QTableWidgetItem *priceItem = m_partsTable->item(r, colBase + 4);
            if (!nameItem || !priceItem) continue;
            QString partName = nameItem->data(Qt::UserRole).toString();
            if (partName.isEmpty()) continue;
            double newPrice = priceItem->text().remove("¥").toDouble();
            q.prepare("UPDATE t_workorder_item SET unit_price = :p "
                      "WHERE workorder_id = :oid AND part_name = :n AND item_type = '材料'");
            q.bindValue(":p", newPrice);
            q.bindValue(":oid", m_currentOrderId);
            q.bindValue(":n", partName);
            DbManager::instance().executeQuery(q);
        }
    }

    // 3. 汇总当前编辑后的费用
    double laborTotal = 0;
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        for (int colBase : {0, 5}) {
            QTableWidgetItem *feeItem = m_laborTable->item(r, colBase + 3);
            if (feeItem) laborTotal += feeItem->text().remove("¥").toDouble();
        }
    }
    double partsTotal = 0;
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        for (int colBase : {0, 7}) {
            QTableWidgetItem *subItem = m_partsTable->item(r, colBase + 5);
            if (subItem) partsTotal += subItem->text().remove("¥").toDouble();
        }
    }
    double otherFee = m_editOtherFee->value();
    double mgmtFee  = m_editMgmtFee->value();
    double grandTotal = laborTotal + partsTotal + otherFee + mgmtFee;

    // 4. 更新工单费用
    q.prepare("UPDATE t_workorder SET labor_fee = :lf, other_fee = :of, "
              "management_fee = :mf, total_amount = :total WHERE id = :id");
    q.bindValue(":lf", laborTotal);
    q.bindValue(":of", otherFee);
    q.bindValue(":mf", mgmtFee);
    q.bindValue(":total", grandTotal);
    q.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(q);

    QMessageBox::information(this, "成功", "费用修改已保存");
    loadOrderInfo(m_currentOrderNo);
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
        // 同步更新维修历史状态
        QSqlQuery mu(DbManager::instance().database());
        mu.prepare("UPDATE t_maintenance_history SET status = '待提单' WHERE workorder_id = :id");
        mu.bindValue(":id", m_currentOrderId);
        DbManager::instance().executeQuery(mu);

        m_currentStatus = "待提单";
        updateActionButtons(m_currentStatus);
        loadOrderInfo(m_currentOrderNo);
        QMessageBox::information(this, "成功", "已通知库房提单，状态更新为「待提单」");
    } else {
        QMessageBox::warning(this, "失败", "状态更新失败，工单可能已被其他人操作");
    }
}

// ============================================================
// 取消提单: 待提单 → 已派工
// ============================================================

// ============================================================
// 取消提单: 待提单 → 已派工
// ============================================================
void QuotePage::onCancelNotify()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "确认取消提单",
            QString("确认撤销提单通知？\n\n工单号: %1\n当前状态: 待提单 → 已派工\n\n"
                    "撤销后库房将无法对该工单进行材料审核提单。")
            .arg(m_currentOrderNo),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DbManager::instance().database());
    q.prepare("UPDATE t_workorder SET status = '已派工' WHERE id = :id AND status = '待提单'");
    q.bindValue(":id", m_currentOrderId);
    if (DbManager::instance().executeQuery(q) && q.numRowsAffected() > 0) {
        // 同步更新维修历史状态
        QSqlQuery mu(DbManager::instance().database());
        mu.prepare("UPDATE t_maintenance_history SET status = '已派工' WHERE workorder_id = :id");
        mu.bindValue(":id", m_currentOrderId);
        DbManager::instance().executeQuery(mu);

        m_currentStatus = "已派工";
        updateActionButtons(m_currentStatus);
        loadOrderInfo(m_currentOrderNo);
        QMessageBox::information(this, "成功", "提单通知已撤销，工单状态恢复为「已派工」");
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

    // 报修项目明细 JSON（含主修人）
    QStringList repairJsonParts;
    QSqlQuery rq(DbManager::instance().database());
    rq.prepare("SELECT item_type, repair_person, repair_content, fee FROM t_workorder_repair_item "
               "WHERE workorder_id = :oid ORDER BY item_type");
    rq.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(rq);
    while (rq.next()) {
        repairJsonParts << QString("{\"type\":\"%1\",\"person\":\"%2\",\"content\":\"%3\",\"fee\":%4}")
                         .arg(rq.value(0).toString(),
                              rq.value(1).toString().replace("\"", "\\\""),
                              rq.value(2).toString().replace("\"", "\\\""),
                              QString::number(rq.value(3).toDouble(), 'f', 2));
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

    // 写入/更新维修历史
    QSqlQuery ih(DbManager::instance().database());
    ih.prepare("INSERT INTO t_maintenance_history "
               "(vehicle_id, workorder_id, status, maintenance_date, entry_date, completion_date, "
               "mileage, service_advisor, technicians, total_amount, cumulative_amount, "
               "labor_fee, material_fee, other_fee, management_fee, deposit, "
               "parts_summary, repair_summary, repair_items) "
               "VALUES (:vid, :woid, '已结算', NOW(), :entry, NOW(), :mile, :svc, :tech, "
               ":total, :cum, :labor, :mat, :oth, :mgmt, :dep, :parts, :repair, :ritems) "
               "ON DUPLICATE KEY UPDATE status='已结算', completion_date=NOW(), "
               "total_amount=VALUES(total_amount), cumulative_amount=VALUES(cumulative_amount), "
               "labor_fee=VALUES(labor_fee), material_fee=VALUES(material_fee), "
               "other_fee=VALUES(other_fee), management_fee=VALUES(management_fee), "
               "deposit=VALUES(deposit), parts_summary=VALUES(parts_summary), "
               "repair_summary=VALUES(repair_summary), repair_items=VALUES(repair_items)");
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
        vu.prepare("UPDATE t_vehicle SET last_maintenance_date = CURDATE(), "
                   "last_maintenance_mileage = :mile WHERE id = :v");
        vu.bindValue(":mile", mileage);
        vu.bindValue(":v", vid);
        DbManager::instance().executeQuery(vu);
    }

    q.prepare("UPDATE t_workorder SET status = '已结算', total_amount = :total "
              "WHERE id = :id AND status = '已提单'");
    q.bindValue(":total", grandTotal);
    q.bindValue(":id", m_currentOrderId);
    if (DbManager::instance().executeQuery(q) && q.numRowsAffected() > 0) {
        // ===== 写入结算记录 =====
        {
            QSqlQuery sq(DbManager::instance().database());
            sq.prepare("INSERT INTO t_settlement "
                       "(workorder_id, labor_fee, material_fee, total_amount, settled_by, settled_at) "
                       "VALUES (:oid, :labor, :mat, :total, :by, NOW())");
            sq.bindValue(":oid", m_currentOrderId);
            sq.bindValue(":labor", laborFee);
            sq.bindValue(":mat", matTotal);
            sq.bindValue(":total", grandTotal);
            sq.bindValue(":by", Session::instance().isLoggedIn()
                              ? QVariant(Session::instance().userId())
                              : QVariant());
            if (!DbManager::instance().executeQuery(sq)) {
                qWarning() << "[QuotePage] 写入结算记录失败:" << DbManager::instance().lastError();
            }
        }

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
    return buildSettlementHtmlFor(m_currentOrderId);
}

// ============================================================
// 构建结算单HTML（静态：本页打印/PDF 与工单详情弹窗共用）
// ============================================================
QString QuotePage::buildSettlementHtmlFor(int orderId)
{
    if (orderId == 0) return QString();

    // ======================== 查询工单基本信息（含里程数） ========================
    QSqlQuery q(DbManager::instance().database());
    q.prepare(
        "SELECT w.order_no, w.status, w.labor_fee, w.material_fee, "
        "  w.other_fee, w.management_fee, w.total_amount, w.deposit, "
        "  w.repair_content, w.created_at, w.repair_date, w.mileage, "
        "  v.plate_number, v.vin, v.model, v.engine_number, "
        "  COALESCE(c.name,''), COALESCE(c.phone,''), COALESCE(c.address,'') "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id "
        "WHERE w.id = :id");
    q.bindValue(":id", orderId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) return QString();

    QString orderNo      = q.value(0).toString();
    double  laborFee     = q.value(2).toDouble();
    double  otherFee     = q.value(4).toDouble();
    double  mgmtFee      = q.value(5).toDouble();
    double  deposit      = q.value(7).toDouble();
    QString createdDate  = q.value(9).toDateTime().toString("yyyy-MM-dd");
    QString repairDate   = q.value(10).toDate().toString("yyyy-MM-dd");
    int     mileage      = q.value(11).toInt();
    QString plate        = q.value(12).toString();
    QString vin          = q.value(13).toString();
    QString model        = q.value(14).toString();
    QString engine       = q.value(15).toString();
    QString ownerName    = q.value(16).toString();
    QString ownerPhone   = q.value(17).toString();
    QString ownerAddr    = q.value(18).toString();
    QString entryDate    = !repairDate.isEmpty() ? repairDate : createdDate;

    // ======================== 工时费明细 ========================
    QString laborRows;
    double  laborFromItems = 0;
    int     laborSeq = 0;
    {
        QSqlQuery lq(DbManager::instance().database());
        lq.prepare("SELECT item_type, repair_person, repair_content, fee "
                   "FROM t_workorder_repair_item "
                   "WHERE workorder_id = :oid ORDER BY item_type, id");
        lq.bindValue(":oid", orderId);
        DbManager::instance().executeQuery(lq);
        while (lq.next()) {
            laborSeq++;
            QString typ     = lq.value(0).toString();
            QString person  = lq.value(1).toString();
            QString content = lq.value(2).toString().trimmed();
            double  fee     = lq.value(3).toDouble();
            laborFromItems += fee;

            QString serviceItem = typ;
            if (!content.isEmpty())
                serviceItem += ": " + content;

            laborRows += QString(
                "<tr><td align='center'>%1</td>"
                "<td>%2</td>"
                "<td align='center'>%3</td>"
                "<td align='right'>%4</td></tr>")
                .arg(laborSeq)
                .arg(serviceItem.toHtmlEscaped())
                .arg(person.isEmpty() ? "" : person.toHtmlEscaped())
                .arg(fee, 0, 'f', 2);
        }
    }
    double displayLabor = (laborFee > 0) ? laborFee : laborFromItems;

    // ======================== 材料明细（含配件件号 + 单位） ========================
    QString partsRows;
    double  partsTotal = 0;
    int     partsSeq = 0;
    {
        QSqlQuery pq(DbManager::instance().database());
        pq.prepare(
            "SELECT COALESCE(p.part_no, ''), wi.part_name, "
            "  COALESCE(NULLIF(p.spec,''), '个') AS unit, "
            "  COUNT(*) AS qty, wi.unit_price, SUM(wi.subtotal) AS subtotal, "
            "  COALESCE(MAX(p.purchase_price), 0) AS cost "
            "FROM t_workorder_item wi "
            "LEFT JOIN t_parts p ON p.id = wi.part_id "
            "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
            "GROUP BY p.part_no, wi.part_name, p.spec, wi.unit_price "
            "ORDER BY wi.part_name");
        pq.bindValue(":oid", orderId);
        DbManager::instance().executeQuery(pq);
        while (pq.next()) {
            partsSeq++;
            QString partNo   = pq.value(0).toString();
            QString partName = pq.value(1).toString();
            QString unit     = pq.value(2).toString();
            int     qty      = pq.value(3).toInt();
            double  price    = pq.value(4).toDouble();
            double  sub      = pq.value(5).toDouble();
            double  cost     = pq.value(6).toDouble();
            partsTotal += sub;

            partsRows += QString(
                "<tr><td align='center'>%1</td>"
                "<td>%2</td><td>%3</td>"
                "<td align='center'>%4</td>"
                "<td align='center'>%5</td>"
                "<td align='right'>%6</td>"
                "<td align='right'>%7</td>"
                "<td align='right'>%8</td>"
                "<td align='right'>%9</td></tr>")
                .arg(partsSeq)
                .arg(partNo.toHtmlEscaped())
                .arg(partName.toHtmlEscaped())
                .arg(unit.toHtmlEscaped())
                .arg(qty)
                .arg(cost, 0, 'f', 2)
                .arg(cost * qty, 0, 'f', 2)
                .arg(price, 0, 'f', 2)
                .arg(sub, 0, 'f', 2);
        }
    }

    double grandTotal = displayLabor + partsTotal + otherFee + mgmtFee;

    // ======================== 大写金额转换 ========================
    auto toChineseUpper = [](double amount) -> QString {
        if (amount < 0.005) return QString();
        const QStringList digits  = {"零","壹","贰","叁","肆","伍","陆","柒","捌","玖"};
        const QStringList radices = {"","拾","佰","仟"};
        const int divs[] = {1000, 100, 10, 1};

        qint64 yuan = static_cast<qint64>(amount);
        int jiao = static_cast<int>((amount - yuan) * 100 + 0.5) / 10;
        int fen  = static_cast<int>((amount - yuan) * 100 + 0.5) % 10;

        if (yuan == 0 && jiao == 0 && fen == 0) return "零元整";

        QString result;
        if (yuan > 0) {
            QList<int> segs;
            qint64 t = yuan;
            while (t > 0) { segs.prepend(static_cast<int>(t % 10000)); t /= 10000; }
            for (int s = 0; s < segs.size(); s++) {
                int seg = segs[s];
                if (seg == 0) continue;
                QString part;
                bool needZero = false;
                for (int i = 0; i < 4; i++) {
                    int d = (seg / divs[i]) % 10;
                    if (d != 0) {
                        if (needZero) { part += "零"; needZero = false; }
                        part += digits[d] + radices[i];
                    } else {
                        if (!part.isEmpty()) needZero = true;
                    }
                }
                if (part.endsWith("零")) part.chop(1);
                int bigIdx = segs.size() - 1 - s;
                if (bigIdx == 1) part += "万";
                else if (bigIdx == 2) part += "亿";
                result += part;
            }
            result += "元";
        }
        if (jiao > 0) result += digits[jiao] + "角";
        else if (yuan > 0 && fen > 0) result += "零";
        if (fen > 0) result += digits[fen] + "分";
        else if (jiao == 0 && fen == 0) result += "整";

        return result;
    };

    // 应收款 = 总费用 - 订金
    double unpaid = grandTotal - deposit;
    double receivable = (deposit > 0) ? unpaid : grandTotal;
    QString amountInWords = toChineseUpper(receivable);

    // ======================== 结算信息 ========================
    QString settlementPerson = Session::instance().userName();
    QString settlementDate   = QDate::currentDate().toString("yyyy-MM-dd");

    // ======================== CSS 样式 ========================
    QString style = QString(
        "@page{margin:8mm;size:A4;}"
        "body{font-family:SimSun,serif;font-size:11px;margin:0;padding:0;color:#000;}"
        "h2{text-align:center;font-size:15px;margin:0 0 6px 0;padding:0;font-weight:bold;}"
        ".order-no{font-size:10px;margin-bottom:4px;text-align:right;}"
        "table{border-collapse:collapse;width:100%;}"
        "table.info td{border:1px solid #000;padding:3px 5px;font-size:10px;}"
        "table.info td.lbl{background:#f0f0f0;font-weight:bold;text-align:right;width:11%;}"
        "table.data{border:1px solid #000;}"
        "table.data th{border:1px solid #000;padding:4px 2px;font-size:10px;text-align:center;background:#e8e8e8;}"
        "table.data td{border:1px solid #000;padding:3px 4px;font-size:10px;}"
        "h3{font-size:12px;margin:8px 0 3px 0;font-weight:bold;}"
        ".subtotal{font-weight:bold;background:#f5f5f5;}"
        ".total td{border:1px solid #000;padding:3px 5px;font-size:10px;}"
        ".total td.lbl{background:#f0f0f0;font-weight:bold;text-align:right;}"
        ".big td{font-weight:bold;font-size:11px;}"
        "p.ft{font-size:9px;margin-top:8px;text-align:center;color:#555;}"
        ".footer td{font-size:10px;padding:3px 5px;}");
    // ------------------------------------------------------------------
    // 组装 HTML
    // 为了避开大量 %%1 占位符冲突，这里使用 replace() 逐项替换标记
    // ------------------------------------------------------------------
    QString html = QString(
        "<!DOCTYPE html><html><head><meta charset='utf-8'><style>%STYLE%</style></head><body>"

        // ---- 标题 + 工号 ----
        "<h2>成都科盟汽车服务有限责任公司维修结算单</h2>"
        "<div class='order-no'>工号: %ORDER%</div>"

        // ---- 客户信息 4 列 ----
        "<table class='info'>"
        "<tr><td class='lbl'>送修单位</td><td>%OWNER%</td>"
        "<td class='lbl'>联系人</td><td>%CONTACT%</td></tr>"
        "<tr><td class='lbl'>联系电话</td><td>%PHONE%</td>"
        "<td class='lbl'>车牌号</td><td>%PLATE%</td></tr>"
        "<tr><td class='lbl'>车型</td><td>%MODEL%</td>"
        "<td class='lbl'>发动机号</td><td>%ENGINE%</td></tr>"
        "<tr><td class='lbl'>车身号 (VIN)</td><td>%VIN%</td>"
        "<td class='lbl'>送修日期</td><td>%ENTRYDATE%</td></tr>"
        "<tr><td class='lbl'>里程数</td><td>%MILEAGE% km</td>"
        "<td class='lbl'></td><td></td></tr>"
        "</table>"

        // ---- 工时费明细 ----
        "<h3>工时费明细</h3>"
        "<table class='data'>"
        "<tr><th>序号</th><th>维修项目</th><th>主修人</th><th>工时费</th></tr>"
        "%LABORROWS%"
        "<tr class='subtotal'><td colspan='3' align='right'>工时费合计</td>"
        "<td align='right'>%LABORTOTAL%</td></tr>"
        "</table>"

        // ---- 材料明细 ----
        "<h3>材料明细</h3>"
        "<table class='data'>"
        "<tr><th>序号</th><th>配件件号</th><th>配件名称</th><th>单位</th><th>数量</th><th>成本</th><th>总成本</th><th>单价</th><th>金额</th></tr>"
        "%PARTSROWS%"
        "<tr class='subtotal'><td colspan='8' align='right'>材料费合计</td>"
        "<td align='right'>%PARTSTOTAL%</td></tr>"
        "</table>"

        // ---- 费用总计（复杂网格） ----
        "<h3>费用总计</h3>"
        "<table class='total'>"
        // --- 第 1 行：材料费 | 工时费 ---
        "<tr>"
        "<td class='lbl' width='11%'>材料费</td><td align='right' width='14%'>%MFEE%</td>"
        "<td width='2%'></td>"
        "<td class='lbl' width='11%'>工时费</td><td align='right' width='14%'>%LFEE%</td>"
        "</tr>"
        // --- 第 2 行：管理费 | 材料优惠费 ---
        "<tr>"
        "<td class='lbl'>管理费</td><td align='right'>%MGM%</td>"
        "<td></td>"
        "<td class='lbl'>材料优惠费</td><td align='right'></td>"
        "</tr>"
        // --- 第 3 行：三包工时费 | 优惠工时费 ---
        "<tr>"
        "<td class='lbl'>三包工时费</td><td align='right'></td>"
        "<td></td>"
        "<td class='lbl'>优惠工时费</td><td align='right'></td>"
        "</tr>"
        // --- 第 4 行：(空) | 保修材料费 ---
        "<tr>"
        "<td></td><td></td>"
        "<td></td>"
        "<td class='lbl'>保修材料费</td><td align='right'></td>"
        "</tr>"
        // --- 第 5 行：(空) | 保修工时费 ---
        "<tr>"
        "<td></td><td></td>"
        "<td></td>"
        "<td class='lbl'>保修工时费</td><td align='right'></td>"
        "</tr>"
        // --- 第 6 行：(空) | 优惠保修材料费 ---
        "<tr>"
        "<td></td><td></td>"
        "<td></td>"
        "<td class='lbl'>优惠保修材料费</td><td align='right'></td>"
        "</tr>"
        // --- 第 7 行：(空) | 优惠保修工时费 ---
        "<tr>"
        "<td></td><td></td>"
        "<td></td>"
        "<td class='lbl'>优惠保修工时费</td><td align='right'></td>"
        "</tr>"
        // --- 合计大行：总费用 | 总优惠 | 应收款 ---
        "<tr class='big'>"
        "<td class='lbl'>总费用</td><td align='right'>%GRAND%</td>"
        "<td></td>"
        "<td class='lbl'>总优惠</td><td align='right'></td>"
        "<td class='lbl'>应收款</td><td align='right'>%RECV%</td>"
        "</tr>"
        // --- 大写金额 ---
        "<tr>"
        "<td class='lbl'>大写金额</td>"
        "<td colspan='6'>%WORDS%</td>"
        "</tr>"
        "</table>"

        // ---- 页脚 ----
        "<table style='border:none;margin-top:12px;'><tr>"
        "<td style='border:none;font-size:10px;'>地址: 四川省成都市______区______路______号</td>"
        "</tr><tr>"
        "<td style='border:none;font-size:10px;'>服务电话: 028-________</td>"
        "</tr></table>"
        "<table class='info' style='margin-top:8px;'>"
        "<tr><td class='lbl'>结算人</td><td>%SETTLER%</td>"
        "<td class='lbl'>结算日期</td><td>%SETTLEDATE%</td></tr>"
        "<tr><td class='lbl'>出厂日期</td><td></td>"
        "<td class='lbl'>收款人签字</td><td></td></tr>"
        "</table>"

        "<p class='ft'>打印时间: %PRINTTIME%</p>"
        "</body></html>");

    // ---- 替换所有标记 ----
    // 工时费行（若为空则显示占位行）
    QString laborRowsHtml = laborRows.isEmpty()
        ? QString("<tr><td align='center'>-</td><td align='center' style='color:#999;'>暂无工时费明细</td>"
                  "<td align='center'>-</td><td align='right'>0.00</td></tr>")
        : laborRows;

    // 材料行（若为空则显示占位行）
    QString partsRowsHtml = partsRows.isEmpty()
        ? QString("<tr><td align='center'>-</td><td></td>"
                  "<td align='center' style='color:#999;'>暂无材料明细</td>"
                  "<td align='center'>-</td><td align='center'>-</td>"
                  "<td align='right'>-</td><td align='right'>-</td>"
                  "<td align='right'>-</td><td align='right'>0.00</td></tr>")
        : partsRows;

    auto esc = [](const QString &s) { return s.toHtmlEscaped(); };
    auto fmt = [](double v) { return QString("¥%1").arg(v, 0, 'f', 2); };
    auto mi  = [](int v) -> QString { return v > 0 ? QString::number(v) : QString(); };

    html.replace("%STYLE%",       style);
    html.replace("%ORDER%",       esc(orderNo));
    html.replace("%OWNER%",       esc(ownerName));
    html.replace("%CONTACT%",     esc(ownerName));
    html.replace("%PHONE%",       esc(ownerPhone));
    html.replace("%PLATE%",       esc(plate));
    html.replace("%MODEL%",       esc(model));
    html.replace("%ENGINE%",      esc(engine));
    html.replace("%VIN%",         esc(vin));
    html.replace("%ENTRYDATE%",   entryDate);
    html.replace("%MILEAGE%",     mi(mileage));
    html.replace("%LABORROWS%",   laborRowsHtml);
    html.replace("%LABORTOTAL%",  fmt(displayLabor));
    html.replace("%PARTSROWS%",   partsRowsHtml);
    html.replace("%PARTSTOTAL%",  fmt(partsTotal));
    html.replace("%MFEE%",        fmt(partsTotal));
    html.replace("%MGM%",         fmt(mgmtFee));
    html.replace("%LFEE%",        fmt(displayLabor));
    html.replace("%GRAND%",       fmt(grandTotal));
    html.replace("%RECV%",        fmt(receivable));
    html.replace("%WORDS%",       amountInWords);
    html.replace("%SETTLER%",     esc(settlementPerson));
    html.replace("%SETTLEDATE%",  settlementDate);
    html.replace("%PRINTTIME%",   QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

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
    if (m_currentStatus != "已提单" && m_currentStatus != "已结算") {
        QMessageBox::warning(this, "提示", "只有「已提单」或「已结算」状态的工单才能保存结算单");
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
    if (m_currentStatus != "已提单" && m_currentStatus != "已结算") {
        QMessageBox::warning(this, "提示", "只有「已提单」或「已结算」状态的工单才能打印结算单");
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
