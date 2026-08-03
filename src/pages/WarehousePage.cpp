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
#include <algorithm>

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
    , m_issuePartId(0), m_billingOrderId(0), m_retPartId(0), m_retLockedWorkOrderId(0), m_purRetPartId(0)
{
    setupUI();
}

WarehousePage::~WarehousePage() {}

void WarehousePage::refreshData()
{
    m_issueOrderNo->clear(); m_issueRecipient->clear();
    m_issuePartSearch->clear(); m_spinIssueQty->setValue(1);
    m_issuePartId = 0; m_lblIssuePartInfo->setText("请搜索备件");
    m_issueStatusBar->setText("当前未锁定工单");

    m_billingOrderNo->clear();
    m_lblBillingInfo->setText("请搜索工单");
    m_billingOrderId = 0;

    m_purPartSearch->clear();
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purCost->setValue(0); m_purPrice->setValue(0);
    m_purQty->setValue(1);
    m_purchaseList.clear();
    refreshPurchaseList();

    m_stockKeyword->clear();

    m_retOrderNo->clear(); m_retPartSearch->clear();
    m_retQty->setValue(1); m_retPartId = 0; m_retLockedWorkOrderId = 0;
    m_retStatusBar->setText("当前未锁定工单");

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

    // 状态栏：显示锁定的工单号和车牌号
    m_issueStatusBar = new QLabel("当前未锁定工单");
    m_issueStatusBar->setStyleSheet(
        "padding:6px 10px;background:#eaf2f8;border:1px solid #aed6f1;"
        "border-radius:3px;font-size:13px;font-weight:bold;color:#2c3e50;");
    issueLayout->addWidget(m_issueStatusBar);

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

    m_btnCancelBill = new QPushButton("取消提单");
    m_btnCancelBill->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#c0392b;color:#fff;font-weight:bold;font-size:14px;}QPushButton:hover{background:#a93226;}");
    m_btnCancelBill->setEnabled(false);
    billBottom->addWidget(m_btnCancelBill);
    billLayout->addLayout(billBottom);

    m_tabWidget->addTab(m_tabBilling, "材料结算/提单");

    // ============================================================
    // Tab 2: 采购入库
    // ============================================================
    m_tabPurchase = new QWidget;
    QVBoxLayout *purLayout = new QVBoxLayout(m_tabPurchase);
    purLayout->setContentsMargins(10, 8, 10, 8);

    // 本批入库清单标题 + 合计 + 移除按钮
    QHBoxLayout *purListHeader = new QHBoxLayout;
    QLabel *lblPurListTitle = new QLabel("本批入库清单:");
    lblPurListTitle->setStyleSheet("font-size:13px;font-weight:bold;color:#2c3e50;");
    purListHeader->addWidget(lblPurListTitle);
    purListHeader->addStretch();
    m_lblPurTotal = new QLabel("共 0 项 | 合计金额: ¥0.00");
    m_lblPurTotal->setStyleSheet("font-size:13px;font-weight:bold;color:#e74c3c;");
    purListHeader->addWidget(m_lblPurTotal);
    m_btnPurRemoveItem = new QPushButton("移除选中");
    m_btnPurRemoveItem->setStyleSheet(S_BTNGH);
    purListHeader->addWidget(m_btnPurRemoveItem);
    purLayout->addLayout(purListHeader);

    m_purTable = new QTableView;
    m_purTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_purTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_purTable->setAlternatingRowColors(true);
    m_purTable->horizontalHeader()->setStretchLastSection(true);
    m_purTable->verticalHeader()->setVisible(false);
    m_purTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    purLayout->addWidget(m_purTable, 1);
    m_purModel = new QStandardItemModel(this);
    m_purModel->setHorizontalHeaderLabels(
        {"备件编号","备件名称","规格型号","供应商","进货价","销售价","数量","小计(¥)"});
    m_purTable->setModel(m_purModel);

    // 采购入库输入区：含模糊搜索（搜索已有备件自动填充）
    QGroupBox *purOp = new QGroupBox("采购入库 — 填写备件信息并「加入清单」");
    QGridLayout *purGrid = new QGridLayout(purOp);
    purGrid->setSpacing(4);

    m_purPartSearch = new QLineEdit; m_purPartSearch->setPlaceholderText("搜索已有备件(编号/名称)自动填充");
    m_btnPurSearch = new QPushButton("搜索");
    m_btnPurSearch->setStyleSheet(S_BTN1H);
    m_purPartNo = new QLineEdit; m_purPartNo->setPlaceholderText("*必填");
    m_purPartName = new QLineEdit; m_purPartName->setPlaceholderText("*必填");
    m_purSpec = new QLineEdit; m_purSpec->setPlaceholderText("选填，默认唯一");
    m_purSupplier = new QLineEdit; m_purSupplier->setPlaceholderText("选填");
    m_purCost = new QDoubleSpinBox; m_purCost->setRange(0, 999999.99); m_purCost->setPrefix("¥ "); m_purCost->setDecimals(2);
    m_purPrice = new QDoubleSpinBox; m_purPrice->setRange(0, 999999.99); m_purPrice->setPrefix("¥ "); m_purPrice->setDecimals(2);
    m_purQty = new QSpinBox; m_purQty->setRange(1, 99999);

    purGrid->addWidget(new QLabel("模糊搜索:"), 0, 0);
    purGrid->addWidget(m_purPartSearch, 0, 1, 1, 3);
    purGrid->addWidget(new QLabel("备件编号*:"), 1, 0); purGrid->addWidget(m_purPartNo, 1, 1);
    purGrid->addWidget(new QLabel("备件名称*:"), 1, 2); purGrid->addWidget(m_purPartName, 1, 3);
    purGrid->addWidget(new QLabel("规格型号:"), 2, 0); purGrid->addWidget(m_purSpec, 2, 1);
    purGrid->addWidget(new QLabel("供应商:"), 2, 2); purGrid->addWidget(m_purSupplier, 2, 3);
    purGrid->addWidget(new QLabel("进货价:"), 3, 0); purGrid->addWidget(m_purCost, 3, 1);
    purGrid->addWidget(new QLabel("销售价:"), 3, 2); purGrid->addWidget(m_purPrice, 3, 3);
    purGrid->addWidget(new QLabel("数量:"), 4, 0); purGrid->addWidget(m_purQty, 4, 1);
    // 按钮顺序: 搜索 → 加入清单 → 确认入库
    purGrid->addWidget(m_btnPurSearch, 4, 2);
    m_btnPurAddItem = new QPushButton("加入清单");
    m_btnPurAddItem->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#3498db;color:#fff;font-weight:bold;}QPushButton:hover{background:#2980b9;}");
    purGrid->addWidget(m_btnPurAddItem, 4, 3);
    m_btnPurConfirm = new QPushButton("确认入库");
    m_btnPurConfirm->setStyleSheet("QPushButton{padding:9px 20px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-weight:bold;font-size:13px;}QPushButton:hover{background:#219a52;}");
    purGrid->addWidget(m_btnPurConfirm, 5, 0, 1, 4);
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
    retTop->addWidget(new QLabel("查询工单:"));
    m_retOrderNo = new QLineEdit;
    m_retOrderNo->setPlaceholderText("输入工单号或车牌号模糊搜索");
    retTop->addWidget(m_retOrderNo, 1);
    retTop->addWidget(new QLabel("搜索备件:"));
    m_retPartSearch = new QLineEdit;
    m_retPartSearch->setPlaceholderText("编号/名称（仅搜索已领出备件）");
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

    // 状态栏：显示锁定的工单号和车牌号
    m_retStatusBar = new QLabel("当前未锁定工单");
    m_retStatusBar->setStyleSheet(
        "padding:6px 10px;background:#eaf2f8;border:1px solid #aed6f1;"
        "border-radius:3px;font-size:13px;font-weight:bold;color:#2c3e50;");
    retLayout->addWidget(m_retStatusBar);

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
    m_purRetPartSearch->setPlaceholderText("编号/名称（仅搜索在库备件）");
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

    // Tab 切换时自动刷新对应页面
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &WarehousePage::onTabChanged);

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
    connect(m_btnCancelBill, &QPushButton::clicked, this, &WarehousePage::onCancelBill);

    connect(m_btnPurSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseSearch);
    connect(m_purPartSearch, &QLineEdit::returnPressed, this, &WarehousePage::onPurchaseSearch);
    connect(m_btnPurAddItem, &QPushButton::clicked, this, &WarehousePage::onPurchaseAddItem);
    connect(m_btnPurRemoveItem, &QPushButton::clicked, this, &WarehousePage::onPurchaseRemoveItem);
    connect(m_btnPurConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseConfirm);

    connect(m_btnStockSearch, &QPushButton::clicked, this, &WarehousePage::onStockSearch);

    // 备件退库 — 工单搜索弹窗 + 状态栏更新 + 锁定工单ID
    connect(m_retOrderNo, &QLineEdit::returnPressed, this, [this]() {
        if (m_retOrderNo->text().trimmed().isEmpty()) return;
        if (showWorkOrderSearchPopup(m_retOrderNo)) {
            QString orderNo = m_retOrderNo->text().trimmed();
            QSqlQuery q(DbManager::instance().database());
            q.prepare("SELECT w.id, v.plate_number FROM t_workorder w "
                      "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
                      "WHERE w.order_no = :no");
            q.bindValue(":no", orderNo);
            DbManager::instance().executeQuery(q);
            if (q.next()) {
                m_retLockedWorkOrderId = q.value(0).toInt();
                QString plate = q.value(1).toString();
                if (!plate.isEmpty())
                    m_retStatusBar->setText(QString("当前工单: %1 | 车牌号: %2 | 仅搜索本工单已领出备件").arg(orderNo, plate));
                else
                    m_retStatusBar->setText(QString("当前工单: %1 | 仅搜索本工单已领出备件").arg(orderNo));
            }
        }
    });
    // 清空工单号时重置锁定
    connect(m_retOrderNo, &QLineEdit::textChanged, this, [this](const QString &txt) {
        if (txt.trimmed().isEmpty()) {
            m_retLockedWorkOrderId = 0;
            m_retStatusBar->setText("当前未锁定工单");
        }
    });
    connect(m_btnRetSearch, &QPushButton::clicked, this, &WarehousePage::onReturnSearch);
    connect(m_retPartSearch, &QLineEdit::returnPressed, this, &WarehousePage::onReturnSearch);
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

bool WarehousePage::showWorkOrderSearchPopup(QLineEdit *targetField, const QString &statusFilter)
{
    QString text = targetField->text().trimmed();
    if (text.isEmpty()) return false;

    QString kw = text;
    kw.replace("'", "''");

    // 构建状态条件（支持逗号分隔的多状态）
    QString statusWhere;
    QStringList statusList = statusFilter.split(',', Qt::SkipEmptyParts);
    if (statusList.size() == 1) {
        statusWhere = QString("w.status = '%1'").arg(statusList[0].trimmed());
    } else {
        QStringList parts;
        for (const QString &s : statusList)
            parts << "'" + s.trimmed() + "'";
        statusWhere = "w.status IN (" + parts.join(",") + ")";
    }

    QSqlQuery q(DbManager::instance().database());
    q.prepare(QString(
        "SELECT w.id, w.order_no, w.status, COALESCE(v.plate_number,'') AS plate, "
        "w.repair_content, w.created_at "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE %2 "
        "AND (w.order_no LIKE '%%1%' OR v.plate_number LIKE '%%1%') "
        "ORDER BY w.id DESC LIMIT 30").arg(kw, statusWhere));
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
    QString displayFilter = statusFilter;
    dlg.setWindowTitle(QString("选择工单 — %1").arg(displayFilter.replace(',', " / ")));
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
    if (showWorkOrderSearchPopup(m_issueOrderNo)) {
        // 工单锁定成功 → 查询车牌号并更新状态栏
        QString orderNo = m_issueOrderNo->text().trimmed();
        QSqlQuery q(DbManager::instance().database());
        q.prepare("SELECT v.plate_number FROM t_workorder w "
                  "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
                  "WHERE w.order_no = :no");
        q.bindValue(":no", orderNo);
        DbManager::instance().executeQuery(q);
        if (q.next() && !q.value(0).toString().isEmpty()) {
            m_issueStatusBar->setText(
                QString("当前工单: %1 | 车牌号: %2").arg(orderNo, q.value(0).toString()));
        } else {
            m_issueStatusBar->setText(QString("当前工单: %1").arg(orderNo));
        }
        m_issueStatusBar->setVisible(true);
    } else {
        // 无匹配结果 → 提示用户
        QMessageBox::information(this, "未找到",
            QString("未找到包含「%1」的已派工工单。\n\n"
                    "可能原因：\n"
                    "• 工单号或车牌号输入有误\n"
                    "• 工单状态不是「已派工」（可能已进入维修/提单/结算阶段）")
            .arg(text.trimmed()));
    }
}

// ============================================================
// 材料结算/提单 — 工单搜索（textChanged 触发）
// ============================================================
void WarehousePage::onBillingOrderSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) return;
    if (showWorkOrderSearchPopup(m_billingOrderNo, "待提单,已提单")) {
        // 用户选择了工单 → 自动触发票据搜索加载明细
        onBillingSearchOrder();
    } else {
        QMessageBox::information(this, "未找到",
            QString("未找到包含「%1」的待提单或已提单工单。\n\n"
                    "可能原因：\n"
                    "• 工单号或车牌号输入有误\n"
                    "• 工单状态不是「待提单」或「已提单」")
            .arg(text.trimmed()));
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
    q.prepare("SELECT part_no, name, "
              "COALESCE(NULLIF(sale_price, 0), purchase_price, 0) FROM t_parts WHERE id = :id");
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

    // 同步更新维修历史：备件摘要
    if (woid > 0) {
        QStringList psList;
        QSqlQuery psq(DbManager::instance().database());
        psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                    "WHERE workorder_id = :oid AND item_type = '材料' "
                    "GROUP BY part_name");
        psq.bindValue(":oid", woid);
        DbManager::instance().executeQuery(psq);
        while (psq.next())
            psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
        QSqlQuery mu(DbManager::instance().database());
        mu.prepare("UPDATE t_maintenance_history SET parts_summary=:ps WHERE workorder_id=:oid");
        mu.bindValue(":ps", psList.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : psList.join(", "));
        mu.bindValue(":oid", woid);
        DbManager::instance().executeQuery(mu);
    }

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

    m_btnConfirmBill->setEnabled(status == "待提单");
    m_btnCancelBill->setEnabled(status == "已提单");

    if (status == "已派工") {
        QMessageBox::information(this, "提示", "该工单尚未通知提单，请先由前台通知提单后再操作");
    } else if (status == "已结算") {
        QMessageBox::information(this, "提示", "该工单已结算，无法操作");
    } else if (status != "待提单" && status != "已提单") {
        QMessageBox::warning(this, "状态错误",
            QString("当前状态为「%1」，需要「待提单」或「已提单」才能操作").arg(status));
    }
}

void WarehousePage::onCompareAndBill()
{
    if (m_billingOrderId == 0) return;

    // 检查当前工单状态
    {
        QSqlQuery cq(DbManager::instance().database());
        cq.prepare("SELECT status FROM t_workorder WHERE id = :id");
        cq.bindValue(":id", m_billingOrderId);
        DbManager::instance().executeQuery(cq);
        if (cq.next()) {
            QString st = cq.value(0).toString();
            if (st == "已提单") {
                QMessageBox::warning(this, "操作无效",
                    "该工单状态为「已提单」，无需再次提单。\n如需撤销提单，请使用「取消提单」按钮。");
                return;
            }
            if (st != "待提单" && st != "已派工") {
                QMessageBox::warning(this, "操作无效",
                    QString("当前工单状态为「%1」，无法执行提单操作。").arg(st));
                return;
            }
        }
    }

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
              "WHERE id = :id AND status IN ('已派工','待提单')");
    q.bindValue(":mat", matTotal);
    q.bindValue(":id", m_billingOrderId);
    if (!DbManager::instance().executeQuery(q) || q.numRowsAffected() == 0) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "提单失败", "状态更新失败，请确认工单状态为「已派工」或「待提单」");
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

    // 同步更新维修历史：状态 + 材料费 + 备件摘要
    {
        QStringList psList;
        QSqlQuery psq(DbManager::instance().database());
        psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                    "WHERE workorder_id = :oid AND item_type = '材料' "
                    "GROUP BY part_name");
        psq.bindValue(":oid", m_billingOrderId);
        DbManager::instance().executeQuery(psq);
        while (psq.next())
            psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
        QString ps = psList.join(", ");

        QSqlQuery mu(DbManager::instance().database());
        mu.prepare("UPDATE t_maintenance_history SET status='已提单', material_fee=:mat, "
                   "parts_summary=:ps WHERE workorder_id=:oid");
        mu.bindValue(":mat", matTotal);
        mu.bindValue(":ps", ps.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : ps);
        mu.bindValue(":oid", m_billingOrderId);
        DbManager::instance().executeQuery(mu);
    }

    QMessageBox::information(this, "提单成功",
        QString("工单材料审核已通过，已设置为「已提单」状态\n"
                "备件已绑定到车辆，材料费合计: ¥%1\n前台可进行结算操作")
        .arg(matTotal, 0, 'f', 2));
    m_btnConfirmBill->setEnabled(false);
    m_btnCancelBill->setEnabled(true);
    {
        QString txt = m_lblBillingInfo->text();
        txt.replace("已派工", "已提单").replace("待提单", "已提单");
        m_lblBillingInfo->setText(txt);
    }
}

// ============================================================
// 取消提单: 已提单 → 待提单
// ============================================================
void WarehousePage::onCancelBill()
{
    if (m_billingOrderId == 0) return;

    // 检查当前工单状态
    QSqlQuery cq(DbManager::instance().database());
    cq.prepare("SELECT status FROM t_workorder WHERE id = :id");
    cq.bindValue(":id", m_billingOrderId);
    DbManager::instance().executeQuery(cq);
    if (cq.next()) {
        QString st = cq.value(0).toString();
        if (st == "待提单") {
            QMessageBox::warning(this, "操作无效",
                "该工单状态为「待提单」，尚未确认提单，无需取消。\n如需提单，请使用「确认提单」按钮。");
            return;
        }
        if (st != "已提单") {
            QMessageBox::warning(this, "操作无效",
                QString("当前工单状态为「%1」，无法执行取消提单操作。").arg(st));
            return;
        }
    }

    if (QMessageBox::question(this, "确认取消提单",
            "确认撤销本次提单？\n\n"
            "撤销后工单状态将从「已提单」恢复为「待提单」，\n"
            "已安装的备件将恢复为已领出状态。\n\n"
            "请谨慎操作。",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    // 更新工单状态为待提单
    q.prepare("UPDATE t_workorder SET status = '待提单' WHERE id = :id AND status = '已提单'");
    q.bindValue(":id", m_billingOrderId);
    if (!DbManager::instance().executeQuery(q) || q.numRowsAffected() == 0) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "取消失败", "状态更新失败，工单状态可能已变更");
        return;
    }

    // 将该工单关联的已安装实例恢复为已领出
    q.prepare("UPDATE t_part_instance SET status = '已领出', vehicle_id = NULL, "
              "updated_at = NOW() WHERE workorder_id = :wid AND status = '已安装'");
    q.bindValue(":wid", m_billingOrderId);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();

    // 同步更新维修历史状态
    {
        QSqlQuery mu(DbManager::instance().database());
        mu.prepare("UPDATE t_maintenance_history SET status='待提单' WHERE workorder_id=:oid");
        mu.bindValue(":oid", m_billingOrderId);
        DbManager::instance().executeQuery(mu);
    }

    m_btnConfirmBill->setEnabled(true);
    m_btnCancelBill->setEnabled(false);
    {
        QString txt = m_lblBillingInfo->text();
        txt.replace("已提单", "待提单");
        m_lblBillingInfo->setText(txt);
    }

    QMessageBox::information(this, "取消成功",
        "提单已撤销，工单状态恢复为「待提单」，备件已恢复为已领出状态。");
}

// ============================================================
// Tab 2: 采购入库（按批进货）— 批次清单模式
// ============================================================

// 模糊搜索已有备件并自动填充输入区（功能与旧版一致）
void WarehousePage::onPurchaseSearch()
{
    QString keyword = m_purPartSearch->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::information(this, "提示", "请输入备件编号或名称关键字进行搜索");
        return;
    }

    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT p.id, p.part_no, p.name, COALESCE(NULLIF(p.spec,''),''), COALESCE(p.supplier,'') "
              "FROM t_parts p WHERE p.part_no LIKE :kw OR p.name LIKE :kw "
              "ORDER BY p.name LIMIT 30");
    q.bindValue(":kw", "%" + keyword + "%");
    DbManager::instance().executeQuery(q);

    struct Match { QString no, name, spec, supplier; };
    QList<Match> rows;
    while (q.next())
        rows << Match{q.value(1).toString(), q.value(2).toString(),
                      q.value(3).toString(), q.value(4).toString()};

    if (rows.isEmpty()) {
        QMessageBox::information(this, "未找到",
            QString("未找到包含「%1」的已有备件。\n可直接在下方手动填写，新备件将在入库时自动建档。").arg(keyword));
        return;
    }

    // 唯一匹配 → 直接填充
    if (rows.size() == 1) {
        m_purPartNo->setText(rows[0].no);
        m_purPartName->setText(rows[0].name);
        m_purSpec->setText(rows[0].spec);
        m_purPartNo->setFocus();
        return;
    }

    // 多结果 → 弹窗选择
    QDialog dlg(this);
    dlg.setWindowTitle(QString("选择已有备件 — %1").arg(keyword));
    dlg.resize(560, 340);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);

    QTableWidget *tbl = new QTableWidget;
    tbl->setColumnCount(4);
    tbl->setHorizontalHeaderLabels({"备件编号", "备件名称", "规格型号", "供应商"});
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setSelectionMode(QAbstractItemView::SingleSelection);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->verticalHeader()->setVisible(false);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tbl->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");
    tbl->setAlternatingRowColors(true);

    tbl->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++) {
        tbl->setItem(i, 0, new QTableWidgetItem(rows[i].no));
        tbl->setItem(i, 1, new QTableWidgetItem(rows[i].name));
        tbl->setItem(i, 2, new QTableWidgetItem(rows[i].spec));
        tbl->setItem(i, 3, new QTableWidgetItem(rows[i].supplier));
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
        m_purPartNo->setText(rows[r].no);
        m_purPartName->setText(rows[r].name);
        m_purSpec->setText(rows[r].spec);
        m_purPartNo->setFocus();
    }
}

// 将当前输入区的备件加入本批清单
void WarehousePage::onPurchaseAddItem()
{
    QString partNo = m_purPartNo->text().trimmed();
    QString partName = m_purPartName->text().trimmed();
    if (partNo.isEmpty() || partName.isEmpty()) {
        QMessageBox::warning(this, "提示", "备件编号和名称为必填项");
        return;
    }

    PurchaseItem it;
    it.partNo = partNo;
    it.partName = partName;
    it.spec = m_purSpec->text().trimmed();
    it.supplier = m_purSupplier->text().trimmed();
    it.cost = m_purCost->value();
    it.price = m_purPrice->value();
    it.qty = m_purQty->value();

    m_purchaseList << it;
    refreshPurchaseList();

    // 清空输入区，方便连续录入
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purCost->setValue(0); m_purPrice->setValue(0);
    m_purQty->setValue(1);
    m_purPartNo->setFocus();
}

// 从清单移除选中项
void WarehousePage::onPurchaseRemoveItem()
{
    QModelIndexList sel = m_purTable->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先在清单中选中要移除的行");
        return;
    }
    QList<int> rows;
    for (const QModelIndex &idx : sel) rows << idx.row();
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) {
        if (r >= 0 && r < m_purchaseList.size())
            m_purchaseList.removeAt(r);
    }
    refreshPurchaseList();
}

// 刷新清单表格显示
void WarehousePage::refreshPurchaseList()
{
    m_purModel->removeRows(0, m_purModel->rowCount());
    double total = 0;
    int totalQty = 0;
    for (const PurchaseItem &it : m_purchaseList) {
        double sub = it.cost * it.qty;
        total += sub;
        totalQty += it.qty;
        QList<QStandardItem*> row;
        row << new QStandardItem(it.partNo)
            << new QStandardItem(it.partName)
            << new QStandardItem(it.spec)
            << new QStandardItem(it.supplier)
            << new QStandardItem(QString::number(it.cost, 'f', 2))
            << new QStandardItem(QString::number(it.price, 'f', 2))
            << new QStandardItem(QString::number(it.qty))
            << new QStandardItem(QString::number(sub, 'f', 2));
        m_purModel->appendRow(row);
    }
    m_lblPurTotal->setText(QString("共 %1 项 / %2 件 | 合计金额: ¥%3")
                           .arg(m_purchaseList.size()).arg(totalQty).arg(total, 0, 'f', 2));
}

// 确认入库：弹窗确认后批量入库
void WarehousePage::onPurchaseConfirm()
{
    if (m_purchaseList.isEmpty()) {
        QMessageBox::warning(this, "提示", "清单为空，请先填写备件信息并点击「加入清单」");
        return;
    }

    // 确认弹窗
    QDialog dlg(this);
    dlg.setWindowTitle("确认本批入库");
    dlg.resize(680, 440);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);

    QLabel *tip = new QLabel(QString("以下 %1 种备件将全部入库，请核对无误：").arg(m_purchaseList.size()));
    tip->setStyleSheet("font-weight:bold;");
    dl->addWidget(tip);

    QTableWidget *tbl = new QTableWidget;
    tbl->setColumnCount(7);
    tbl->setHorizontalHeaderLabels({"备件编号", "备件名称", "规格型号", "数量", "进货价", "小计", "供应商"});
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->verticalHeader()->setVisible(false);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->setAlternatingRowColors(true);
    tbl->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");

    tbl->setRowCount(m_purchaseList.size());
    double total = 0;
    for (int i = 0; i < m_purchaseList.size(); i++) {
        const PurchaseItem &it = m_purchaseList[i];
        double sub = it.cost * it.qty;
        total += sub;
        tbl->setItem(i, 0, new QTableWidgetItem(it.partNo));
        tbl->setItem(i, 1, new QTableWidgetItem(it.partName));
        tbl->setItem(i, 2, new QTableWidgetItem(it.spec));
        tbl->setItem(i, 3, new QTableWidgetItem(QString::number(it.qty)));
        tbl->setItem(i, 4, new QTableWidgetItem(QString("¥%1").arg(it.cost, 0, 'f', 2)));
        tbl->setItem(i, 5, new QTableWidgetItem(QString("¥%1").arg(sub, 0, 'f', 2)));
        tbl->setItem(i, 6, new QTableWidgetItem(it.supplier));
    }
    dl->addWidget(tbl, 1);

    QHBoxLayout *bb = new QHBoxLayout;
    QLabel *lblTotal = new QLabel(QString("合计金额: ¥%1").arg(total, 0, 'f', 2));
    lblTotal->setStyleSheet("font-size:16px;font-weight:bold;color:#e74c3c;");
    bb->addWidget(lblTotal);
    bb->addStretch();
    QPushButton *ok = new QPushButton("确认入库");
    ok->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-weight:bold;}QPushButton:hover{background:#219a52;}");
    QPushButton *ca = new QPushButton("返回修改");
    ca->setStyleSheet(S_BTNGH);
    bb->addWidget(ca);
    bb->addWidget(ok);
    dl->addLayout(bb);

    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(ca, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    // 批量执行入库（单事务，任一失败整体回滚）
    DbManager::instance().beginTransaction();
    bool allOk = true;
    for (const PurchaseItem &it : m_purchaseList) {
        if (!doPurchaseInbound(it)) { allOk = false; break; }
    }
    if (!allOk) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "入库失败", "本批入库已整体回滚，请检查错误信息后重试");
        return;
    }
    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "入库成功",
        QString("本批共 %1 种备件已全部入库").arg(m_purchaseList.size()));
    m_purchaseList.clear();
    refreshPurchaseList();
}

// 单条备件入库（在调用方事务内执行）
bool WarehousePage::doPurchaseInbound(const PurchaseItem &it)
{
    QSqlQuery q(DbManager::instance().database());

    // 检查是否已有该备件编号
    int catalogId;
    q.prepare("SELECT id FROM t_parts WHERE part_no = :no");
    q.bindValue(":no", it.partNo);
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
        u.bindValue(":name", it.partName);
        u.bindValue(":spec", it.spec.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.spec);
        u.bindValue(":sup", it.supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.supplier);
        u.bindValue(":cost", it.cost > 0 ? it.cost : QVariant(QMetaType::fromType<double>()));
        u.bindValue(":price", it.price > 0 ? it.price : QVariant(QMetaType::fromType<double>()));
        u.bindValue(":id", catalogId);
        DbManager::instance().executeQuery(u);
    } else {
        // 新建备件目录
        q.prepare("INSERT INTO t_parts (part_no, name, spec, stock, purchase_price, sale_price, supplier) "
                  "VALUES (:no, :name, :spec, 0, :cost, :price, :sup)");
        q.bindValue(":no", it.partNo);
        q.bindValue(":name", it.partName);
        q.bindValue(":spec", it.spec.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.spec);
        q.bindValue(":cost", it.cost > 0 ? it.cost : QVariant(QMetaType::fromType<double>()));
        q.bindValue(":price", it.price > 0 ? it.price : QVariant(QMetaType::fromType<double>()));
        q.bindValue(":sup", it.supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.supplier);
        if (!DbManager::instance().executeQuery(q)) {
            QMessageBox::warning(this, "入库失败",
                QString("备件「%1」建档案失败: %2").arg(it.partNo, q.lastError().text()));
            return false;
        }
        catalogId = q.lastInsertId().toInt();
    }

    // 创建 qty 个实例
    for (int i = 0; i < it.qty; i++) {
        QString sn = generateInstanceSN(it.partNo, catalogId);
        QSqlQuery ins(DbManager::instance().database());
        ins.prepare("INSERT INTO t_part_instance (part_id, instance_sn, status, "
                    "unit_purchase_price, unit_sale_price, remark) "
                    "VALUES (:pid, :sn, '在库', :cost, :price, :rmk)");
        ins.bindValue(":pid", catalogId);
        ins.bindValue(":sn", sn);
        ins.bindValue(":cost", it.cost > 0 ? it.cost : QVariant(QMetaType::fromType<double>()));
        ins.bindValue(":price", it.price > 0 ? it.price : QVariant(QMetaType::fromType<double>()));
        ins.bindValue(":rmk", it.supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.supplier);
        DbManager::instance().executeQuery(ins);

        int instId = ins.lastInsertId().toInt();

        // 记录流水
        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
                    "operation_type, operator_id) "
                    "VALUES (:pid, :iid, 1, :price, :total, '采购入库', :op)");
        log.bindValue(":pid", catalogId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", it.cost);
        log.bindValue(":total", it.cost);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 记录采购批次
    q.prepare("INSERT INTO t_part_purchase (part_id, supplier, quantity, unit_cost, total_cost, operator_id) "
              "VALUES (:pid, :sup, :qty, :cost, :total, :op)");
    q.bindValue(":pid", catalogId);
    q.bindValue(":sup", it.supplier.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : it.supplier);
    q.bindValue(":qty", it.qty);
    q.bindValue(":cost", it.cost);
    q.bindValue(":total", it.qty * it.cost);
    q.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    // 更新库存缓存
    q.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    q.bindValue(":pid", catalogId);
    q.bindValue(":pid2", catalogId);
    DbManager::instance().executeQuery(q);

    return true;
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
    QSqlQuery q(DbManager::instance().database());

    // 构建 WHERE 条件
    QStringList conditions;
    if (!keyword.isEmpty()) {
        conditions << QString("(p.part_no LIKE '%%1%' OR p.name LIKE '%%1%' OR p.supplier LIKE '%%1%' OR p.spec LIKE '%%1%')")
                          .arg(keyword.replace("'", "''"));
    }
    // 若工单已锁定，仅搜索该工单绑定的已领出实例
    if (m_retLockedWorkOrderId > 0) {
        conditions << QString("i.workorder_id = %1").arg(m_retLockedWorkOrderId);
    }

    QString whereClause = conditions.isEmpty() ? "" : "WHERE " + conditions.join(" AND ");

    // 与备件领取 onPartsSearch 同风格，但仅 JOIN 已领出实例
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COUNT(i.id) AS '可退数量', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "INNER JOIN t_part_instance i ON i.part_id = p.id AND i.status = '已领出' "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(whereClause);

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

    // 获取可退库的实例（仅已领出）
    QList<int> instanceIds;
    QSqlQuery q(DbManager::instance().database());

    // 如果指定了工单号，从该工单查找
    if (!orderNo.isEmpty()) {
        q.prepare("SELECT i.id FROM t_part_instance i "
                  "JOIN t_workorder w ON w.id = i.workorder_id "
                  "WHERE i.part_id = :pid AND i.status = '已领出' "
                  "AND w.order_no LIKE :ono "
                  "LIMIT :lim");
        q.bindValue(":ono", "%" + orderNo + "%");
    } else {
        q.prepare("SELECT id FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '已领出' "
                  "LIMIT :lim");
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

    // 同步更新维修历史：备件摘要
    if (!orderNo.isEmpty()) {
        QSqlQuery woq(DbManager::instance().database());
        woq.prepare("SELECT id FROM t_workorder WHERE order_no=:no");
        woq.bindValue(":no", orderNo);
        DbManager::instance().executeQuery(woq);
        if (woq.next()) {
            int woid = woq.value(0).toInt();
            QStringList psList;
            QSqlQuery psq(DbManager::instance().database());
            psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                        "WHERE workorder_id=:oid AND item_type='材料' GROUP BY part_name");
            psq.bindValue(":oid", woid);
            DbManager::instance().executeQuery(psq);
            while (psq.next())
                psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
            QSqlQuery mu(DbManager::instance().database());
            mu.prepare("UPDATE t_maintenance_history SET parts_summary=:ps WHERE workorder_id=:oid");
            mu.bindValue(":ps", psList.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : psList.join(", "));
            mu.bindValue(":oid", woid);
            DbManager::instance().executeQuery(mu);
        }
    }

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
    QSqlQuery q(DbManager::instance().database());

    // 与备件领取 onPartsSearch 同风格，但仅 JOIN 在库实例
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COUNT(i.id) AS '在库数量', "
        "COALESCE(p.purchase_price, 0) AS '进货价', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "INNER JOIN t_part_instance i ON i.part_id = p.id AND i.status = '在库' "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.purchase_price, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(!keyword.isEmpty()
             ? QString("WHERE p.part_no LIKE '%%1%' OR p.name LIKE '%%1%' OR p.supplier LIKE '%%1%' OR p.spec LIKE '%%1%'")
               .arg(keyword.replace("'", "''"))
             : "");

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

// ============================================================
// Tab 切换自动刷新 — 每次切换到某个Tab时自动刷新该页数据
// ============================================================
void WarehousePage::onTabChanged(int index)
{
    switch (index) {
    case 0: // 备件领取
        onPartsSearch();
        break;
    case 1: // 材料结算/提单 — 若已加载工单则刷新明细，否则清空
        if (m_billingOrderId > 0 && !m_billingOrderNo->text().trimmed().isEmpty()) {
            onBillingSearchOrder();
        } else {
            m_billingModel->setQuery(QSqlQuery());
            m_lblBillingInfo->setText("请搜索工单");
            m_lblBillingTotal->setText("材料费合计: ¥0.00");
            m_btnConfirmBill->setEnabled(false);
            m_btnCancelBill->setEnabled(false);
        }
        break;
    case 2: // 采购入库（按批）— 刷新本批清单
        refreshPurchaseList();
        break;
    case 3: // 库存查询
        onStockSearch();
        break;
    case 4: // 备件退库
        onReturnSearch();
        break;
    case 5: // 采购退货
        onPurchaseReturnSearch();
        break;
    }
}

void WarehousePage::loadTechCombos() {}
void WarehousePage::loadPartCombos() {}
