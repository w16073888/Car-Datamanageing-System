#include "WarehousePage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QSqlRecord>
#include <QDialog>
#include <QTableWidget>

#define S_BTN1 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#3498db;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN1H S_BTN1 "QPushButton:hover{background:#2980b9;}"
#define S_BTN2 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN2H S_BTN2 "QPushButton:hover{background:#219a52;}"
#define S_BTNG "QPushButton{padding:6px 14px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;font-size:12px;}"
#define S_BTNGH S_BTNG "QPushButton:hover{background:#d5dbdb;}"

// ============================================================
// 工具函数实现
// ============================================================

QString WarehousePage::mergedSelectSQL(const QString &extraCols, const QString &whereClause,
                                        const QString &groupBy, const QString &orderBy) const
{
    // 基础合并查询: 从实例表JOIN目录表, 按(part_no, name, spec, supplier)分组
    // spec为空时用 part_no 生成唯一标识, 确保不与其他空规格零件合并
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商' "
        "%1 "
        "FROM t_parts p "
        "JOIN t_part_instance i ON i.part_id = p.id "
        "%2 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier %3 "
        "%4")
        .arg(extraCols.isEmpty() ? "" : ", " + extraCols,
             whereClause.isEmpty() ? "" : "WHERE " + whereClause,
             groupBy.isEmpty() ? "" : ", " + groupBy,
             orderBy.isEmpty() ? "ORDER BY p.name LIMIT 500" : orderBy);
    return sql;
}

QString WarehousePage::generateInstanceSN(const QString &partNo, int partId) const
{
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT COUNT(*) FROM t_part_instance WHERE part_id = :pid");
    q.bindValue(":pid", partId);
    DbManager::instance().executeQuery(q);
    int cnt = 0;
    if (q.next()) cnt = q.value(0).toInt();
    return QString("%1-%2").arg(partNo).arg(cnt + 1, 4, 10, QChar('0'));
}

QList<int> WarehousePage::getInStockInstanceIds(int partId, int count) const
{
    QList<int> ids;
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT id FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库' "
              "ORDER BY id ASC LIMIT :lim");
    q.bindValue(":pid", partId);
    q.bindValue(":lim", count);
    DbManager::instance().executeQuery(q);
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

QList<int> WarehousePage::getCheckedOutInstanceIds(int partId, int count) const
{
    QList<int> ids;
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT id FROM t_part_instance "
              "WHERE part_id = :pid AND status = '已领出' "
              "ORDER BY id ASC LIMIT :lim");
    q.bindValue(":pid", partId);
    q.bindValue(":lim", count);
    DbManager::instance().executeQuery(q);
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

bool WarehousePage::updateInstanceStatus(const QList<int> &instanceIds, const QString &newStatus,
                                          int vehicleId, int workorderId,
                                          const QString &recipient)
{
    if (instanceIds.isEmpty()) return true;

    QSqlQuery q(DbManager::instance().database());
    // 构建 IN 子句
    QStringList placeholders;
    for (int i = 0; i < instanceIds.size(); i++)
        placeholders << QString(":id%1").arg(i);

    QString sql = QString(
        "UPDATE t_part_instance SET status = :st, updated_at = NOW() "
        "%1 %2 %3 "
        "WHERE id IN (%4)")
        .arg(vehicleId >= 0 ? ", vehicle_id = :vid" : "",
             workorderId >= 0 ? ", workorder_id = :wid" : "",
             !recipient.isEmpty() ? ", recipient = :rec" : "",
             placeholders.join(","));

    q.prepare(sql);
    q.bindValue(":st", newStatus);
    if (vehicleId >= 0) q.bindValue(":vid", vehicleId);
    if (workorderId >= 0) q.bindValue(":wid", workorderId);
    if (!recipient.isEmpty()) q.bindValue(":rec", recipient);
    for (int i = 0; i < instanceIds.size(); i++)
        q.bindValue(QString(":id%1").arg(i), instanceIds[i]);

    return DbManager::instance().executeQuery(q);
}

// ============================================================
// 构造与析构
// ============================================================

WarehousePage::WarehousePage(QWidget *parent)
    : QWidget(parent)
    , m_issuePartId(0), m_billingOrderId(0), m_purPartId(0), m_retPartId(0), m_purRetPartId(0)
{
    setupUI();
}

WarehousePage::~WarehousePage() {}

void WarehousePage::refreshData()
{
    m_issueOrderNo->clear(); m_issueRecipient->clear();
    m_issuePartSearch->clear(); m_spinIssueQty->setValue(1);
    m_issuePartId = 0; m_lblIssuePartInfo->setText("请搜索备件");

    m_billingOrderNo->clear();
    m_lblBillingInfo->setText("请搜索工单");
    m_billingOrderId = 0;

    m_purPartSearch->clear();
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purCost->setValue(0); m_purPrice->setValue(0);
    m_purQty->setValue(1); m_purPartId = 0;

    m_stockKeyword->clear();

    m_retOrderNo->clear(); m_retPartSearch->clear();
    m_retQty->setValue(1); m_retPartId = 0;

    m_purRetPartSearch->clear();
    m_purRetQty->setValue(1); m_purRetPartId = 0;
}

// ============================================================
// setupUI — 6个Tab的界面布局
// ============================================================

void WarehousePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);

    QLabel *title = new QLabel("库房工作台");
    title->setStyleSheet("font-size: 17px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    m_tabWidget = new QTabWidget;
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #dcdde1; border-top: none; }"
        "QTabBar::tab { padding: 8px 16px; border: 1px solid #dcdde1;"
        "  border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px;"
        "  font-size: 12px; }"
        "QTabBar::tab:selected { background: #3498db; color: white; }"
        "QTabBar::tab:!selected { background: #ecf0f1; color: #2c3e50; }");

    // ============================================================
    // Tab 0: 备件领取 (Stage 2)
    // ============================================================
    m_tabIssue = new QWidget;
    QVBoxLayout *issueLayout = new QVBoxLayout(m_tabIssue);
    issueLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *issueTop = new QHBoxLayout;
    issueTop->addWidget(new QLabel("查询工单:"));
    m_issueOrderNo = new QLineEdit;
    m_issueOrderNo->setPlaceholderText("输入工单号或车牌号模糊搜索");
    issueTop->addWidget(m_issueOrderNo, 1);
    issueTop->addWidget(new QLabel("领取人:"));
    m_issueRecipient = new QLineEdit;
    m_issueRecipient->setPlaceholderText("输入领取人姓名");
    issueTop->addWidget(m_issueRecipient, 1);
    issueLayout->addLayout(issueTop);

    QHBoxLayout *issueSearch = new QHBoxLayout;
    issueSearch->addWidget(new QLabel("备件搜索:"));
    m_issuePartSearch = new QLineEdit;
    m_issuePartSearch->setPlaceholderText("备件编号/名称模糊搜索");
    issueSearch->addWidget(m_issuePartSearch, 1);
    m_btnIssueSearch = new QPushButton("搜索");
    m_btnIssueSearch->setStyleSheet(S_BTN1H);
    issueSearch->addWidget(m_btnIssueSearch);
    issueLayout->addLayout(issueSearch);

    m_issueTable = new QTableView;
    m_issueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_issueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_issueTable->setAlternatingRowColors(true);
    m_issueTable->horizontalHeader()->setStretchLastSection(true);
    m_issueTable->verticalHeader()->setVisible(false);
    m_issueTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    issueLayout->addWidget(m_issueTable, 1);
    m_issueModel = new QSqlQueryModel(this);
    m_issueTable->setModel(m_issueModel);

    QGroupBox *issueOp = new QGroupBox("出库操作");
    QHBoxLayout *issueOpLayout = new QHBoxLayout(issueOp);
    m_lblIssuePartInfo = new QLabel("请选择备件");
    m_lblIssuePartInfo->setStyleSheet("font-weight:bold;");
    issueOpLayout->addWidget(m_lblIssuePartInfo);
    issueOpLayout->addWidget(new QLabel("数量:"));
    m_spinIssueQty = new QSpinBox;
    m_spinIssueQty->setRange(1, 99999);
    issueOpLayout->addWidget(m_spinIssueQty);
    m_btnIssue = new QPushButton("确认出库");
    m_btnIssue->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#e67e22;color:#fff;font-weight:bold;}QPushButton:hover{background:#d35400;}");
    issueOpLayout->addWidget(m_btnIssue);
    issueOpLayout->addStretch();
    issueLayout->addWidget(issueOp);

    m_tabWidget->addTab(m_tabIssue, "备件领取");

    // ============================================================
    // Tab 1: 材料结算/提单 (Stage 3)
    // ============================================================
    m_tabBilling = new QWidget;
    QVBoxLayout *billLayout = new QVBoxLayout(m_tabBilling);
    billLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *billTop = new QHBoxLayout;
    billTop->addWidget(new QLabel("查询工单:"));
    m_billingOrderNo = new QLineEdit;
    m_billingOrderNo->setPlaceholderText("输入工单号或车牌号模糊搜索");
    billTop->addWidget(m_billingOrderNo, 1);
    billLayout->addLayout(billTop);

    m_lblBillingInfo = new QLabel("请搜索工单");
    m_lblBillingInfo->setStyleSheet("padding:8px;background:#f8f9fa;border-radius:4px;font-size:13px;");
    billLayout->addWidget(m_lblBillingInfo);

    m_billingTable = new QTableView;
    m_billingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_billingTable->setAlternatingRowColors(true);
    m_billingTable->horizontalHeader()->setStretchLastSection(true);
    m_billingTable->verticalHeader()->setVisible(false);
    m_billingTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    billLayout->addWidget(m_billingTable, 1);
    m_billingModel = new QSqlQueryModel(this);
    m_billingTable->setModel(m_billingModel);

    QHBoxLayout *billBottom = new QHBoxLayout;
    m_lblBillingTotal = new QLabel("材料费合计: ¥0.00");
    m_lblBillingTotal->setStyleSheet("font-size:15px;font-weight:bold;color:#e74c3c;");
    billBottom->addWidget(m_lblBillingTotal);
    billBottom->addStretch();
    m_btnConfirmBill = new QPushButton("确认提单");
    m_btnConfirmBill->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#8e44ad;color:#fff;font-weight:bold;font-size:14px;}QPushButton:hover{background:#7d3c98;}");
    m_btnConfirmBill->setEnabled(false);
    billBottom->addWidget(m_btnConfirmBill);
    billLayout->addLayout(billBottom);

    m_tabWidget->addTab(m_tabBilling, "材料结算/提单");

    // ============================================================
    // Tab 2: 采购入库
    // ============================================================
    m_tabPurchase = new QWidget;
    QVBoxLayout *purLayout = new QVBoxLayout(m_tabPurchase);
    purLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *purSearch = new QHBoxLayout;
    purSearch->addWidget(new QLabel("搜索备件:"));
    m_purPartSearch = new QLineEdit;
    m_purPartSearch->setPlaceholderText("编号/名称模糊搜索(留空显示全部)");
    purSearch->addWidget(m_purPartSearch, 1);
    m_btnPurSearch = new QPushButton("搜索");
    m_btnPurSearch->setStyleSheet(S_BTN1H);
    purSearch->addWidget(m_btnPurSearch);
    purLayout->addLayout(purSearch);

    m_purTable = new QTableView;
    m_purTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_purTable->setAlternatingRowColors(true);
    m_purTable->horizontalHeader()->setStretchLastSection(true);
    m_purTable->verticalHeader()->setVisible(false);
    m_purTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    purLayout->addWidget(m_purTable, 1);
    m_purModel = new QSqlQueryModel(this);
    m_purTable->setModel(m_purModel);

    QGroupBox *purOp = new QGroupBox("采购入库");
    QGridLayout *purGrid = new QGridLayout(purOp);
    purGrid->setSpacing(4);

    m_purPartNo = new QLineEdit; m_purPartNo->setPlaceholderText("*必填");
    m_purPartName = new QLineEdit; m_purPartName->setPlaceholderText("*必填");
    m_purSpec = new QLineEdit; m_purSpec->setPlaceholderText("选填，默认唯一");
    m_purSupplier = new QLineEdit; m_purSupplier->setPlaceholderText("选填");
    m_purCost = new QDoubleSpinBox; m_purCost->setRange(0, 999999.99); m_purCost->setPrefix("¥ "); m_purCost->setDecimals(2);
    m_purPrice = new QDoubleSpinBox; m_purPrice->setRange(0, 999999.99); m_purPrice->setPrefix("¥ "); m_purPrice->setDecimals(2);
    m_purQty = new QSpinBox; m_purQty->setRange(1, 99999);

    purGrid->addWidget(new QLabel("备件编号*:"), 0, 0); purGrid->addWidget(m_purPartNo, 0, 1);
    purGrid->addWidget(new QLabel("备件名称*:"), 0, 2); purGrid->addWidget(m_purPartName, 0, 3);
    purGrid->addWidget(new QLabel("规格型号:"), 1, 0); purGrid->addWidget(m_purSpec, 1, 1);
    purGrid->addWidget(new QLabel("供应商:"), 1, 2); purGrid->addWidget(m_purSupplier, 1, 3);
    purGrid->addWidget(new QLabel("进货价:"), 2, 0); purGrid->addWidget(m_purCost, 2, 1);
    purGrid->addWidget(new QLabel("销售价:"), 2, 2); purGrid->addWidget(m_purPrice, 2, 3);
    purGrid->addWidget(new QLabel("数量:"), 3, 0); purGrid->addWidget(m_purQty, 3, 1);

    m_btnPurConfirm = new QPushButton("确认入库");
    m_btnPurConfirm->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-weight:bold;}QPushButton:hover{background:#219a52;}");
    purGrid->addWidget(m_btnPurConfirm, 3, 2, 1, 2);
    purLayout->addWidget(purOp);

    m_tabWidget->addTab(m_tabPurchase, "采购入库");

    // ============================================================
    // Tab 3: 库存查询
    // ============================================================
    m_tabStock = new QWidget;
    QVBoxLayout *stockLayout = new QVBoxLayout(m_tabStock);
    stockLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *stockTop = new QHBoxLayout;
    stockTop->addWidget(new QLabel("关键词:"));
    m_stockKeyword = new QLineEdit;
    m_stockKeyword->setPlaceholderText("编号/名称/规格/供应商 模糊搜索");
    stockTop->addWidget(m_stockKeyword, 1);
    m_btnStockSearch = new QPushButton("查询");
    m_btnStockSearch->setStyleSheet(S_BTN1H);
    stockTop->addWidget(m_btnStockSearch);
    stockLayout->addLayout(stockTop);

    m_stockTable = new QTableView;
    m_stockTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stockTable->setAlternatingRowColors(true);
    m_stockTable->horizontalHeader()->setStretchLastSection(true);
    m_stockTable->verticalHeader()->setVisible(false);
    m_stockTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    stockLayout->addWidget(m_stockTable, 1);
    m_stockModel = new QSqlQueryModel(this);
    m_stockTable->setModel(m_stockModel);

    m_tabWidget->addTab(m_tabStock, "库存查询");

    // ============================================================
    // Tab 4: 备件退库
    // ============================================================
    m_tabReturn = new QWidget;
    QVBoxLayout *retLayout = new QVBoxLayout(m_tabReturn);
    retLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *retTop = new QHBoxLayout;
    retTop->addWidget(new QLabel("原工单号:"));
    m_retOrderNo = new QLineEdit;
    m_retOrderNo->setPlaceholderText("输入原出库工单号(选填)");
    retTop->addWidget(m_retOrderNo, 1);
    retTop->addWidget(new QLabel("搜索备件:"));
    m_retPartSearch = new QLineEdit;
    m_retPartSearch->setPlaceholderText("编号/名称");
    retTop->addWidget(m_retPartSearch, 1);
    m_btnRetSearch = new QPushButton("搜索");
    m_btnRetSearch->setStyleSheet(S_BTN1H);
    retTop->addWidget(m_btnRetSearch);
    retLayout->addLayout(retTop);

    m_retTable = new QTableView;
    m_retTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_retTable->setAlternatingRowColors(true);
    m_retTable->horizontalHeader()->setStretchLastSection(true);
    m_retTable->verticalHeader()->setVisible(false);
    m_retTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    retLayout->addWidget(m_retTable, 1);
    m_retModel = new QSqlQueryModel(this);
    m_retTable->setModel(m_retModel);

    QHBoxLayout *retOp = new QHBoxLayout;
    retOp->addWidget(new QLabel("退库数量:"));
    m_retQty = new QSpinBox;
    m_retQty->setRange(1, 99999);
    retOp->addWidget(m_retQty);
    m_btnRetConfirm = new QPushButton("确认退库");
    m_btnRetConfirm->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#e67e22;color:#fff;font-weight:bold;}QPushButton:hover{background:#d35400;}");
    retOp->addWidget(m_btnRetConfirm);
    retOp->addStretch();
    retLayout->addLayout(retOp);

    m_tabWidget->addTab(m_tabReturn, "备件退库");

    // ============================================================
    // Tab 5: 采购退货（只显示在库的备件）
    // ============================================================
    m_tabPurRet = new QWidget;
    QVBoxLayout *purRetLayout = new QVBoxLayout(m_tabPurRet);
    purRetLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *purRetTop = new QHBoxLayout;
    purRetTop->addWidget(new QLabel("搜索在库备件:"));
    m_purRetPartSearch = new QLineEdit;
    m_purRetPartSearch->setPlaceholderText("编号/名称(仅搜索在库中的备件)");
    purRetTop->addWidget(m_purRetPartSearch, 1);
    m_btnPurRetSearch = new QPushButton("搜索");
    m_btnPurRetSearch->setStyleSheet(S_BTN1H);
    purRetTop->addWidget(m_btnPurRetSearch);
    purRetLayout->addLayout(purRetTop);

    m_purRetTable = new QTableView;
    m_purRetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_purRetTable->setAlternatingRowColors(true);
    m_purRetTable->horizontalHeader()->setStretchLastSection(true);
    m_purRetTable->verticalHeader()->setVisible(false);
    m_purRetTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    purRetLayout->addWidget(m_purRetTable, 1);
    m_purRetModel = new QSqlQueryModel(this);
    m_purRetTable->setModel(m_purRetModel);

    QHBoxLayout *purRetOp = new QHBoxLayout;
    purRetOp->addWidget(new QLabel("退货数量:"));
    m_purRetQty = new QSpinBox;
    m_purRetQty->setRange(1, 99999);
    purRetOp->addWidget(m_purRetQty);
    m_btnPurRetConfirm = new QPushButton("确认退货");
    m_btnPurRetConfirm->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#e74c3c;color:#fff;font-weight:bold;}QPushButton:hover{background:#c0392b;}");
    purRetOp->addWidget(m_btnPurRetConfirm);
    purRetOp->addStretch();
    purRetLayout->addLayout(purRetOp);

    m_tabWidget->addTab(m_tabPurRet, "采购退货");

    mainLayout->addWidget(m_tabWidget, 1);

    // ============================================================
    // Signals
    // ============================================================
    connect(m_issueOrderNo, &QLineEdit::returnPressed, this, [this]() {
        onIssueOrderSearchTextChanged(m_issueOrderNo->text());
    });
    connect(m_issuePartSearch, &QLineEdit::returnPressed, this, &WarehousePage::onPartsSearch);
    connect(m_btnIssueSearch, &QPushButton::clicked, this, &WarehousePage::onPartsSearch);
    connect(m_btnIssue, &QPushButton::clicked, this, &WarehousePage::onPartsIssue);
    connect(m_issueTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        // 合并视图中 column 0 = catalog_id
        m_issuePartId = m_issueModel->data(m_issueModel->index(row, 0)).toInt();
        QString name = m_issueModel->data(m_issueModel->index(row, 2)).toString();
        int stock = m_issueModel->data(m_issueModel->index(row, 5)).toInt(); // 在库数量
        m_lblIssuePartInfo->setText(QString("已选: %1 | 在库: %2").arg(name).arg(stock));
        m_spinIssueQty->setMaximum(stock > 0 ? stock : 1);
    });

    connect(m_billingOrderNo, &QLineEdit::returnPressed, this, [this]() {
        onBillingOrderSearchTextChanged(m_billingOrderNo->text());
    });
    connect(m_btnConfirmBill, &QPushButton::clicked, this, &WarehousePage::onCompareAndBill);

    connect(m_btnPurSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseSearch);
    connect(m_purTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_purPartId = m_purModel->data(m_purModel->index(row, 0)).toInt();
        m_purPartNo->setText(m_purModel->data(m_purModel->index(row, 1)).toString());
        m_purPartName->setText(m_purModel->data(m_purModel->index(row, 2)).toString());
        // column 3 is 规格型号 from merged query (may contain auto-generated unique label)
        QString spec = m_purModel->data(m_purModel->index(row, 3)).toString();
        m_purSpec->setText(spec.startsWith("(无型号-") ? "" : spec);
    });
    connect(m_btnPurConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseConfirm);

    connect(m_btnStockSearch, &QPushButton::clicked, this, &WarehousePage::onStockSearch);

    connect(m_btnRetSearch, &QPushButton::clicked, this, &WarehousePage::onReturnSearch);
    connect(m_retTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_retPartId = m_retModel->data(m_retModel->index(row, 0)).toInt();
        int available = m_retModel->data(m_retModel->index(row, 5)).toInt(); // 可退库数量
        m_retQty->setMaximum(available > 0 ? available : 1);
    });
    connect(m_btnRetConfirm, &QPushButton::clicked, this, &WarehousePage::onReturnConfirm);

    connect(m_btnPurRetSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnSearch);
    connect(m_purRetTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_purRetPartId = m_purRetModel->data(m_purRetModel->index(row, 0)).toInt();
        int inStock = m_purRetModel->data(m_purRetModel->index(row, 5)).toInt(); // 在库数量
        m_purRetQty->setMaximum(inStock > 0 ? inStock : 1);
    });
    connect(m_btnPurRetConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnConfirm);
}

// ============================================================
// 工单搜索弹窗（共用）— 按工单号/车牌模糊搜索，仅"已派工"工单
// ============================================================

bool WarehousePage::showWorkOrderSearchPopup(QLineEdit *targetField)
{
    QString text = targetField->text().trimmed();
    if (text.isEmpty()) return false;

    QString kw = text;
    kw.replace("'", "''");

    QSqlQuery q(DbManager::instance().database());
    q.prepare(QString(
        "SELECT w.id, w.order_no, w.status, COALESCE(v.plate_number,'') AS plate, "
        "w.repair_content, w.created_at "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE w.status = '已派工' "
        "AND (w.order_no LIKE '%%1%' OR v.plate_number LIKE '%%1%') "
        "ORDER BY w.id DESC LIMIT 30").arg(kw));
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        // 无匹配结果，不弹窗
        return false;
    }
    q.seek(-1); // 回到第一条之前

    // 收集所有匹配行
    QList<QStringList> rows;
    while (q.next()) {
        QStringList row;
        row << q.value(1).toString()  // order_no
            << q.value(2).toString()  // status
            << q.value(3).toString()  // plate
            << q.value(4).toString()  // repair_content
            << q.value(5).toDateTime().toString("yyyy-MM-dd HH:mm"); // created_at
        rows << row;
    }

    // 唯一匹配 → 直接锁定，不弹窗
    if (rows.size() == 1) {
        targetField->blockSignals(true);
        targetField->setText(rows[0][0]);  // 填入工单号
        targetField->blockSignals(false);
        return true;
    }

    // 多结果 → 弹窗选择
    QDialog dlg(targetField->window());
    dlg.setWindowTitle("选择工单 — 已派工");
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
        tbl->setItem(i, 0, new QTableWidgetItem(rows[i][0])); // order_no
        tbl->setItem(i, 1, new QTableWidgetItem(rows[i][1])); // status
        tbl->setItem(i, 2, new QTableWidgetItem(rows[i][2])); // plate
        tbl->setItem(i, 3, new QTableWidgetItem(rows[i][3])); // repair_content
        tbl->setItem(i, 4, new QTableWidgetItem(rows[i][4])); // created_at
    }

    dl->addWidget(tbl, 1);

    QHBoxLayout *bb = new QHBoxLayout;
    QPushButton *ok = new QPushButton("选择");
    ok->setStyleSheet(S_BTN1H);
    QPushButton *ca = new QPushButton("取消");
    ca->setStyleSheet(S_BTNGH);
    bb->addStretch();
    bb->addWidget(ok);
    bb->addWidget(ca);
    dl->addLayout(bb);

    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(ca, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(tbl, &QTableWidget::cellDoubleClicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted && tbl->currentRow() >= 0) {
        int r = tbl->currentRow();
        targetField->blockSignals(true);
        targetField->setText(rows[r][0]);  // 填入工单号
        targetField->blockSignals(false);
        return true;
    }
    return false;
}

// ============================================================
// 备件领取 — 工单搜索（textChanged 触发）
// ============================================================
void WarehousePage::onIssueOrderSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) return;
    showWorkOrderSearchPopup(m_issueOrderNo);
}

// ============================================================
// 材料结算/提单 — 工单搜索（textChanged 触发）
// ============================================================
void WarehousePage::onBillingOrderSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) return;
    if (showWorkOrderSearchPopup(m_billingOrderNo)) {
        // 用户选择了工单 → 自动触发票据搜索加载明细
        onBillingSearchOrder();
    }
}

// ============================================================
// Tab 0: 备件领取 (Stage 2) — 合并显示 + 实例级出库
// ============================================================

void WarehousePage::onPartsSearch()
{
    QString keyword = m_issuePartSearch->text().trimmed();
    QSqlQuery q(DbManager::instance().database());

    // LEFT JOIN: 显示所有备件(含库存为0的)，按模糊关键字搜索
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库数量', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "LEFT JOIN t_part_instance i ON i.part_id = p.id "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(!keyword.isEmpty()
             ? QString("WHERE p.part_no LIKE '%%1%' OR p.name LIKE '%%1%' OR p.supplier LIKE '%%1%' OR p.spec LIKE '%%1%'")
               .arg(keyword.replace("'", "''"))
             : "");

    q.prepare(sql);
    DbManager::instance().executeQuery(q);
    m_issueModel->setQuery(std::move(q));
    m_issuePartId = 0;
    m_lblIssuePartInfo->setText("请选择备件");
}

void WarehousePage::onPartsIssue()
{
    if (m_issuePartId == 0) {
        QMessageBox::warning(this, "提示", "请先选择备件");
        return;
    }
    QString orderNo = m_issueOrderNo->text().trimmed();
    if (orderNo.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入工单号");
        return;
    }
    QString recipient = m_issueRecipient->text().trimmed();
    if (recipient.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入领取人");
        return;
    }
    int qty = m_spinIssueQty->value();

    // 获取在库实例
    QList<int> instanceIds = getInStockInstanceIds(m_issuePartId, qty);
    if (instanceIds.size() < qty) {
        QMessageBox::warning(this, "库存不足",
            QString("当前在库 %1 件，出库数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取备件信息
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT part_no, name, COALESCE(sale_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_issuePartId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) return;
    QString partNo = q.value(0).toString();
    QString partName = q.value(1).toString();
    double salePrice = q.value(2).toDouble();

    // 查找工单ID
    q.prepare("SELECT id, vehicle_id FROM t_workorder WHERE order_no = :no");
    q.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(q);
    int woid = 0;
    if (q.next()) woid = q.value(0).toInt();

    DbManager::instance().beginTransaction();

    // 更新实例状态为已领出
    if (!updateInstanceStatus(instanceIds, "已领出", -1, woid > 0 ? woid : -1, recipient)) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "出库失败", "更新备件实例状态失败");
        return;
    }

    // 更新目录表库存缓存
    q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    q.bindValue(":pid", m_issuePartId);
    q.bindValue(":pid2", m_issuePartId);
    DbManager::instance().executeQuery(q);

    // 记录流水(每个实例一条)
    for (int instId : instanceIds) {
        q.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
                  "operation_type, ref_order_no, operator_id, recipient) "
                  "VALUES (:pid, :iid, -1, :price, :total, '维修出库', :ref, :op, :rec)");
        q.bindValue(":pid", m_issuePartId);
        q.bindValue(":iid", instId);
        q.bindValue(":price", salePrice);
        q.bindValue(":total", -salePrice);
        q.bindValue(":ref", orderNo);
        q.bindValue(":op", Session::instance().userId());
        q.bindValue(":rec", recipient);
        DbManager::instance().executeQuery(q);
    }

    // 写入工单备件明细
    if (woid > 0) {
        for (int instId : instanceIds) {
            q.prepare("INSERT INTO t_workorder_item (workorder_id, part_id, part_instance_id, part_name, "
                      "quantity, unit_price, item_type) "
                      "VALUES (:oid, :pid, :iid, :name, 1, :price, '材料')");
            q.bindValue(":oid", woid);
            q.bindValue(":pid", m_issuePartId);
            q.bindValue(":iid", instId);
            q.bindValue(":name", partName);
            q.bindValue(":price", salePrice);
            DbManager::instance().executeQuery(q);
        }
    }

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "出库成功",
        QString("备件「%1」x %2 已出库\n工单: %3\n领取人: %4")
        .arg(partName).arg(qty).arg(orderNo).arg(recipient));
    onPartsSearch();
}

// ============================================================
// Tab 1: 材料结算/提单 (Stage 3) — 实例状态流转到已安装并绑定车辆
// ============================================================

void WarehousePage::onBillingSearchOrder()
{
    QString orderNo = m_billingOrderNo->text().trimmed();
    if (orderNo.isEmpty()) return;

    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT w.id, w.order_no, w.status, v.plate_number, "
              "COALESCE(w.material_fee,0) "
              "FROM t_workorder w LEFT JOIN t_vehicle v ON v.id=w.vehicle_id "
              "WHERE w.order_no LIKE :no ORDER BY w.id DESC LIMIT 1");
    q.bindValue(":no", "%" + orderNo + "%");
    DbManager::instance().executeQuery(q);

    if (!q.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }

    m_billingOrderId = q.value(0).toInt();
    QString status = q.value(2).toString();
    QString plate = q.value(3).toString();

    m_lblBillingInfo->setText(
        QString("工单: %1 | 车牌: %2 | 状态: %3")
        .arg(q.value(1).toString(), plate, status));

    // 加载该工单的备件使用明细（合并显示）
    QSqlQuery q2(DbManager::instance().database());
    q2.prepare("SELECT wi.part_name AS '备件名称', COUNT(*) AS '数量', "
               "wi.unit_price AS '单价', SUM(wi.subtotal) AS '小计' "
               "FROM t_workorder_item wi "
               "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
               "GROUP BY wi.part_name, wi.unit_price");
    q2.bindValue(":oid", m_billingOrderId);
    DbManager::instance().executeQuery(q2);
    m_billingModel->setQuery(std::move(q2));

    // 汇总材料费
    QSqlQuery q3(DbManager::instance().database());
    q3.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
               "WHERE workorder_id = :oid AND item_type = '材料'");
    q3.bindValue(":oid", m_billingOrderId);
    DbManager::instance().executeQuery(q3);
    double matTotal = q3.next() ? q3.value(0).toDouble() : 0;
    m_lblBillingTotal->setText(QString("材料费合计: ¥%1").arg(matTotal, 0, 'f', 2));

    if (status == "已派工" || status == "待提单" || status == "维修中") {
        m_btnConfirmBill->setEnabled(true);
    } else if (status == "已提单") {
        m_btnConfirmBill->setEnabled(false);
        QMessageBox::information(this, "提示", "该工单已完成提单");
    } else if (status == "已结算") {
        m_btnConfirmBill->setEnabled(false);
        QMessageBox::information(this, "提示", "该工单已结算");
    } else {
        m_btnConfirmBill->setEnabled(false);
        QMessageBox::warning(this, "状态错误",
            QString("当前状态为「%1」，需要「已派工」「待提单」或「维修中」才能提单").arg(status));
    }
}

void WarehousePage::onCompareAndBill()
{
    if (m_billingOrderId == 0) return;

    if (QMessageBox::question(this, "确认提单",
            "确认对本次材料数据进行提单？\n"
            "提单后前台即可进行结算操作。\n"
            "请确认材料明细无误后再操作。",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // 获取材料费总额及工单车辆信息
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
              "WHERE workorder_id = :oid AND item_type = '材料'");
    q.bindValue(":oid", m_billingOrderId);
    DbManager::instance().executeQuery(q);
    double matTotal = q.next() ? q.value(0).toDouble() : 0;

    q.prepare("SELECT vehicle_id FROM t_workorder WHERE id = :id");
    q.bindValue(":id", m_billingOrderId);
    DbManager::instance().executeQuery(q);
    int vehicleId = q.next() ? q.value(0).toInt() : -1;

    DbManager::instance().beginTransaction();

    // 更新工单状态为"已提单"
    q.prepare("UPDATE t_workorder SET status = '已提单', material_fee = :mat "
              "WHERE id = :id AND status IN ('已派工','待提单','维修中')");
    q.bindValue(":mat", matTotal);
    q.bindValue(":id", m_billingOrderId);
    if (!DbManager::instance().executeQuery(q) || q.numRowsAffected() == 0) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "提单失败", "状态更新失败，请确认工单状态为「已派工」「待提单」或「维修中」");
        return;
    }

    // 将该工单关联的已领出实例更新为"已安装"并绑定车辆
    q.prepare("SELECT id, part_id FROM t_part_instance "
              "WHERE workorder_id = :wid AND status = '已领出'");
    q.bindValue(":wid", m_billingOrderId);
    DbManager::instance().executeQuery(q);
    QList<QPair<int,int>> instances; // (instance_id, part_id)
    while (q.next())
        instances << QPair<int,int>(q.value(0).toInt(), q.value(1).toInt());

    for (const auto &pair : instances) {
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_part_instance SET status = '已安装', vehicle_id = :vid, "
                  "updated_at = NOW() WHERE id = :iid");
        u.bindValue(":vid", vehicleId > 0 ? vehicleId : QVariant(QMetaType::fromType<int>()));
        u.bindValue(":iid", pair.first);
        DbManager::instance().executeQuery(u);

        // 记录材料结算流水
        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, "
                    "operation_type, ref_order_no, operator_id, remark) "
                    "VALUES (:pid, :iid, 1, '材料结算', :ref, :op, '材料审核提单')");
        log.bindValue(":pid", pair.second);
        log.bindValue(":iid", pair.first);
        log.bindValue(":ref", m_billingOrderNo->text().trimmed());
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 更新库存缓存
    for (const auto &pair : instances) {
        q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
        q.bindValue(":pid", pair.second);
        q.bindValue(":pid2", pair.second);
        DbManager::instance().executeQuery(q);
    }

    // 记录交易历史
    if (vehicleId > 0) {
        QSqlQuery txn(DbManager::instance().database());
        txn.prepare("INSERT INTO t_vehicle_transaction (vehicle_id, workorder_id, "
                     "transaction_type, description, operator_id) "
                     "VALUES (:vid, :woid, '提单', :desc, :op)");
        txn.bindValue(":vid", vehicleId);
        txn.bindValue(":woid", m_billingOrderId);
        txn.bindValue(":desc", QString("材料审核提单完成，材料费合计 ¥%1").arg(matTotal, 0, 'f', 2));
        txn.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(txn);
    }

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "提单成功",
        QString("工单材料审核已通过，已设置为「已提单」状态\n"
                "备件已绑定到车辆，材料费合计: ¥%1\n前台可进行结算操作")
        .arg(matTotal, 0, 'f', 2));
    m_btnConfirmBill->setEnabled(false);
    {
        QString txt = m_lblBillingInfo->text();
        txt.replace("已派工", "已提单").replace("待提单", "已提单").replace("维修中", "已提单");
        m_lblBillingInfo->setText(txt);
    }
}

// ============================================================
// Tab 2: 采购入库 — 每个数量创建独立实例
// ============================================================

void WarehousePage::onPurchaseSearch()
{
    QString keyword = m_purPartSearch->text().trimmed();
    QString extraCols = QString(
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库', "
        "COUNT(CASE WHEN i.status NOT IN ('已退货') THEN 1 END) AS '总数', "
        "COALESCE(p.purchase_price, 0) AS '进货价', "
        "COALESCE(p.sale_price, 0) AS '销售价'");

    QString where;
    if (!keyword.isEmpty()) {
        where = QString("p.part_no LIKE '%%1%' OR p.name LIKE '%%1%'")
                .arg(keyword.replace("'", "''"));
    }

    QString sql = mergedSelectSQL(extraCols, where,
        "p.purchase_price, p.sale_price", "ORDER BY p.name LIMIT 200");
    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    DbManager::instance().executeQuery(q);
    m_purModel->setQuery(std::move(q));
}

void WarehousePage::onPurchaseConfirm()
{
    QString partNo = m_purPartNo->text().trimmed();
    QString partName = m_purPartName->text().trimmed();
    if (partNo.isEmpty() || partName.isEmpty()) {
        QMessageBox::warning(this, "提示", "备件编号和名称为必填项");
        return;
    }
    int qty = m_purQty->value();
    double cost = m_purCost->value();
    double price = m_purPrice->value();
    QString spec = m_purSpec->text().trimmed();
    QString supplier = m_purSupplier->text().trimmed();

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    int catalogId;
    // 检查是否已有该备件编号
    q.prepare("SELECT id FROM t_parts WHERE part_no = :no");
    q.bindValue(":no", partNo);
    DbManager::instance().executeQuery(q);

    if (q.next()) {
        // 已有备件 → 更新目录信息
        catalogId = q.value(0).toInt();
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_parts SET name = :name, "
                  "spec = :spec, supplier = :sup, "
                  "purchase_price = COALESCE(:cost, purchase_price), "
                  "sale_price = COALESCE(:price, sale_price) "
                  "WHERE id = :id");
        u.bindValue(":name", partName);
        u.bindValue(":spec", spec.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : spec);
        u.bindValue(":sup", supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : supplier);
        u.bindValue(":cost", cost > 0 ? cost : QVariant(QMetaType::fromType<double>()));
        u.bindValue(":price", price > 0 ? price : QVariant(QMetaType::fromType<double>()));
        u.bindValue(":id", catalogId);
        DbManager::instance().executeQuery(u);
    } else {
        // 新建备件目录
        q.prepare("INSERT INTO t_parts (part_no, name, spec, stock, purchase_price, sale_price, supplier) "
                  "VALUES (:no, :name, :spec, 0, :cost, :price, :sup)");
        q.bindValue(":no", partNo);
        q.bindValue(":name", partName);
        q.bindValue(":spec", spec.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : spec);
        q.bindValue(":cost", cost > 0 ? cost : QVariant(QMetaType::fromType<double>()));
        q.bindValue(":price", price > 0 ? price : QVariant(QMetaType::fromType<double>()));
        q.bindValue(":sup", supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : supplier);
        if (!DbManager::instance().executeQuery(q)) {
            DbManager::instance().rollbackTransaction();
            QMessageBox::warning(this, "入库失败", q.lastError().text());
            return;
        }
        catalogId = q.lastInsertId().toInt();
    }

    // 创建 qty 个实例
    for (int i = 0; i < qty; i++) {
        QString sn = generateInstanceSN(partNo, catalogId);
        QSqlQuery ins(DbManager::instance().database());
        ins.prepare("INSERT INTO t_part_instance (part_id, instance_sn, status, "
                    "unit_purchase_price, unit_sale_price, remark) "
                    "VALUES (:pid, :sn, '在库', :cost, :price, :rmk)");
        ins.bindValue(":pid", catalogId);
        ins.bindValue(":sn", sn);
        ins.bindValue(":cost", cost > 0 ? cost : QVariant(QMetaType::fromType<double>()));
        ins.bindValue(":price", price > 0 ? price : QVariant(QMetaType::fromType<double>()));
        ins.bindValue(":rmk", supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : supplier);
        DbManager::instance().executeQuery(ins);

        int instId = ins.lastInsertId().toInt();

        // 记录流水
        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
                    "operation_type, operator_id) "
                    "VALUES (:pid, :iid, 1, :price, :total, '采购入库', :op)");
        log.bindValue(":pid", catalogId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", cost);
        log.bindValue(":total", cost);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 记录采购批次
    q.prepare("INSERT INTO t_part_purchase (part_id, supplier, quantity, unit_cost, total_cost, operator_id) "
              "VALUES (:pid, :sup, :qty, :cost, :total, :op)");
    q.bindValue(":pid", catalogId);
    q.bindValue(":sup", supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : supplier);
    q.bindValue(":qty", qty);
    q.bindValue(":cost", cost);
    q.bindValue(":total", qty * cost);
    q.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    // 更新库存缓存
    q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    q.bindValue(":pid", catalogId);
    q.bindValue(":pid2", catalogId);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "入库成功",
        QString("备件「%1」x %2 已入库（已创建 %2 个独立实例）").arg(partName).arg(qty));
    onPurchaseSearch();
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purCost->setValue(0); m_purPrice->setValue(0);
    m_purQty->setValue(1);
}

// ============================================================
// Tab 3: 库存查询 — 合并显示所有状态计数
// ============================================================

void WarehousePage::onStockSearch()
{
    QString keyword = m_stockKeyword->text().trimmed();
    QString extraCols = QString(
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库数量', "
        "COUNT(CASE WHEN i.status='已领出' THEN 1 END) AS '已领出', "
        "COUNT(CASE WHEN i.status='已安装' THEN 1 END) AS '已安装', "
        "COUNT(CASE WHEN i.status NOT IN ('已退货') THEN 1 END) AS '总数', "
        "COALESCE(p.purchase_price, 0) AS '进货价', "
        "COALESCE(p.sale_price, 0) AS '销售价'");

    QString where;
    if (!keyword.isEmpty()) {
        QString kw = keyword;
        kw.replace("'", "''");
        where = QString("p.part_no LIKE '%%1%' OR p.name LIKE '%%1%' "
                        "OR p.spec LIKE '%%1%' OR p.supplier LIKE '%%1%'").arg(kw);
    }

    QString sql = mergedSelectSQL(extraCols, where,
        "p.purchase_price, p.sale_price", "ORDER BY p.name LIMIT 500");
    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    DbManager::instance().executeQuery(q);
    m_stockModel->setQuery(std::move(q));
}

// ============================================================
// Tab 4: 备件退库 — 将已领出/已安装实例退回在库
// ============================================================

void WarehousePage::onReturnSearch()
{
    QString keyword = m_retPartSearch->text().trimmed();
    QString extraCols = QString(
        "COUNT(CASE WHEN i.status IN ('已领出','已安装') THEN 1 END) AS '可退数量', "
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库数量', "
        "COALESCE(p.sale_price, 0) AS '销售价'");

    QString where = "i.status IN ('已领出','已安装')";
    if (!keyword.isEmpty()) {
        where += QString(" AND (p.part_no LIKE '%%1%' OR p.name LIKE '%%1%')")
                 .arg(keyword.replace("'", "''"));
    }

    QString sql = mergedSelectSQL(extraCols, where,
        "p.sale_price", "ORDER BY p.name LIMIT 200");
    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    DbManager::instance().executeQuery(q);
    m_retModel->setQuery(std::move(q));
    m_retPartId = 0;
}

void WarehousePage::onReturnConfirm()
{
    if (m_retPartId == 0) {
        QMessageBox::warning(this, "提示", "请先选择备件");
        return;
    }
    int qty = m_retQty->value();
    QString orderNo = m_retOrderNo->text().trimmed();

    // 获取可退库的实例（优先已领出、其次已安装）
    QList<int> instanceIds;
    QSqlQuery q(DbManager::instance().database());

    // 如果指定了工单号，从该工单查找
    if (!orderNo.isEmpty()) {
        q.prepare("SELECT i.id FROM t_part_instance i "
                  "JOIN t_workorder w ON w.id = i.workorder_id "
                  "WHERE i.part_id = :pid AND i.status IN ('已领出','已安装') "
                  "AND w.order_no LIKE :ono "
                  "ORDER BY i.status = '已领出' DESC LIMIT :lim");
        q.bindValue(":ono", "%" + orderNo + "%");
    } else {
        q.prepare("SELECT id FROM t_part_instance "
                  "WHERE part_id = :pid AND status IN ('已领出','已安装') "
                  "ORDER BY status = '已领出' DESC LIMIT :lim");
    }
    q.bindValue(":pid", m_retPartId);
    q.bindValue(":lim", qty);
    DbManager::instance().executeQuery(q);
    while (q.next()) instanceIds << q.value(0).toInt();

    if (instanceIds.size() < qty) {
        QMessageBox::warning(this, "退库失败",
            QString("可退库的备件仅 %1 件，退库数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取备件信息
    q.prepare("SELECT name, COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_retPartId);
    DbManager::instance().executeQuery(q);
    QString partName = q.next() ? q.value(0).toString() : "未知";
    double costPrice = q.value(1).toDouble();

    DbManager::instance().beginTransaction();

    for (int instId : instanceIds) {
        // 更新实例状态为在库，清除绑定
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_part_instance SET status = '在库', vehicle_id = NULL, "
                  "workorder_id = NULL, recipient = NULL, updated_at = NOW() "
                  "WHERE id = :iid");
        u.bindValue(":iid", instId);
        DbManager::instance().executeQuery(u);

        // 记录流水
        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
                    "operation_type, ref_order_no, operator_id, remark) "
                    "VALUES (:pid, :iid, 1, :price, :total, '备件退库', :ref, :op, '备件退库')");
        log.bindValue(":pid", m_retPartId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", costPrice);
        log.bindValue(":total", costPrice);
        log.bindValue(":ref", orderNo.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : orderNo);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 更新库存缓存
    q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    q.bindValue(":pid", m_retPartId);
    q.bindValue(":pid2", m_retPartId);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退库成功",
        QString("备件「%1」x %2 已退回库房").arg(partName).arg(qty));
    onReturnSearch();
}

// ============================================================
// Tab 5: 采购退货 — 只能操作在库的备件
// ============================================================

void WarehousePage::onPurchaseReturnSearch()
{
    QString keyword = m_purRetPartSearch->text().trimmed();
    QString extraCols = QString(
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库数量', "
        "COALESCE(p.purchase_price, 0) AS '进货价'");

    // 只搜索在库状态的实例
    QString where = "i.status = '在库'";
    if (!keyword.isEmpty()) {
        where += QString(" AND (p.part_no LIKE '%%1%' OR p.name LIKE '%%1%')")
                 .arg(keyword.replace("'", "''"));
    }

    QString sql = mergedSelectSQL(extraCols, where,
        "p.purchase_price", "ORDER BY p.name LIMIT 200");
    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    DbManager::instance().executeQuery(q);
    m_purRetModel->setQuery(std::move(q));
    m_purRetPartId = 0;
}

void WarehousePage::onPurchaseReturnConfirm()
{
    if (m_purRetPartId == 0) {
        QMessageBox::warning(this, "提示", "请先选择备件");
        return;
    }
    int qty = m_purRetQty->value();

    // 获取在库的实例
    QList<int> instanceIds = getInStockInstanceIds(m_purRetPartId, qty);
    if (instanceIds.size() < qty) {
        QMessageBox::warning(this, "退货失败",
            QString("当前仅有 %1 件在库可退，退货数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取备件信息
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT name, COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_purRetPartId);
    DbManager::instance().executeQuery(q);
    QString partName = q.next() ? q.value(0).toString() : "未知";
    double costPrice = q.value(1).toDouble();

    DbManager::instance().beginTransaction();

    for (int instId : instanceIds) {
        // 更新实例状态为已退货
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_part_instance SET status = '已退货', "
                  "workorder_id = NULL, vehicle_id = NULL, recipient = NULL, "
                  "updated_at = NOW() WHERE id = :iid");
        u.bindValue(":iid", instId);
        DbManager::instance().executeQuery(u);

        // 记录流水
        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
                    "operation_type, operator_id, remark) "
                    "VALUES (:pid, :iid, -1, :price, :total, '采购退货', :op, '采购退货')");
        log.bindValue(":pid", m_purRetPartId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", costPrice);
        log.bindValue(":total", -costPrice);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 更新库存缓存
    q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    q.bindValue(":pid", m_purRetPartId);
    q.bindValue(":pid2", m_purRetPartId);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退货成功",
        QString("备件「%1」x %2 已退货").arg(partName).arg(qty));
    onPurchaseReturnSearch();
}

void WarehousePage::loadTechCombos() {}
void WarehousePage::loadPartCombos() {}
