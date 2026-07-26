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

#define S_BTN1 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#3498db;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN1H S_BTN1 "QPushButton:hover{background:#2980b9;}"
#define S_BTN2 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN2H S_BTN2 "QPushButton:hover{background:#219a52;}"
#define S_BTNG "QPushButton{padding:6px 14px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;font-size:12px;}"
#define S_BTNGH S_BTNG "QPushButton:hover{background:#d5dbdb;}"

WarehousePage::WarehousePage(QWidget *parent)
    : QWidget(parent)
    , m_issuePartId(0), m_billingOrderId(0), m_purPartId(0), m_retPartId(0), m_purRetPartId(0)
{
    setupUI();
}

WarehousePage::~WarehousePage() {}

void WarehousePage::refreshData()
{
    // Clear all tabs
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

    // 工单号 + 领取人
    QHBoxLayout *issueTop = new QHBoxLayout;
    issueTop->addWidget(new QLabel("工单号:"));
    m_issueOrderNo = new QLineEdit;
    m_issueOrderNo->setPlaceholderText("输入工单号");
    issueTop->addWidget(m_issueOrderNo, 1);
    issueTop->addWidget(new QLabel("领取人:"));
    m_issueRecipient = new QLineEdit;
    m_issueRecipient->setPlaceholderText("输入领取人姓名");
    issueTop->addWidget(m_issueRecipient, 1);
    issueLayout->addLayout(issueTop);

    // 备件搜索
    QHBoxLayout *issueSearch = new QHBoxLayout;
    issueSearch->addWidget(new QLabel("备件搜索:"));
    m_issuePartSearch = new QLineEdit;
    m_issuePartSearch->setPlaceholderText("备件编号/名称模糊搜索");
    issueSearch->addWidget(m_issuePartSearch, 1);
    m_btnIssueSearch = new QPushButton("搜索");
    m_btnIssueSearch->setStyleSheet(S_BTN1H);
    issueSearch->addWidget(m_btnIssueSearch);
    issueLayout->addLayout(issueSearch);

    // 备件列表
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

    // 出库操作
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
    billTop->addWidget(new QLabel("工单号:"));
    m_billingOrderNo = new QLineEdit;
    m_billingOrderNo->setPlaceholderText("输入工单号");
    billTop->addWidget(m_billingOrderNo, 1);
    m_btnBillingSearch = new QPushButton("搜索工单");
    m_btnBillingSearch->setStyleSheet(S_BTN1H);
    billTop->addWidget(m_btnBillingSearch);
    billLayout->addLayout(billTop);

    m_lblBillingInfo = new QLabel("请搜索工单");
    m_lblBillingInfo->setStyleSheet("padding:8px;background:#f8f9fa;border-radius:4px;font-size:13px;");
    billLayout->addWidget(m_lblBillingInfo);

    // 备件使用明细
    m_billingTable = new QTableView;
    m_billingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_billingTable->setAlternatingRowColors(true);
    m_billingTable->horizontalHeader()->setStretchLastSection(true);
    m_billingTable->verticalHeader()->setVisible(false);
    m_billingTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    billLayout->addWidget(m_billingTable, 1);
    m_billingModel = new QSqlQueryModel(this);
    m_billingTable->setModel(m_billingModel);

    // 底部汇总+提单按钮
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

    // 搜索已有备件
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

    // 入库操作区
    QGroupBox *purOp = new QGroupBox("采购入库");
    QGridLayout *purGrid = new QGridLayout(purOp);
    purGrid->setSpacing(4);

    m_purPartNo = new QLineEdit; m_purPartNo->setPlaceholderText("*必填");
    m_purPartName = new QLineEdit; m_purPartName->setPlaceholderText("*必填");
    m_purSpec = new QLineEdit;
    m_purSupplier = new QLineEdit;
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
    // Tab 5: 采购退货
    // ============================================================
    m_tabPurRet = new QWidget;
    QVBoxLayout *purRetLayout = new QVBoxLayout(m_tabPurRet);
    purRetLayout->setContentsMargins(10, 8, 10, 8);

    QHBoxLayout *purRetTop = new QHBoxLayout;
    purRetTop->addWidget(new QLabel("搜索备件:"));
    m_purRetPartSearch = new QLineEdit;
    m_purRetPartSearch->setPlaceholderText("编号/名称");
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

    // ============================================================
    // Add tab widget to main layout
    // ============================================================
    mainLayout->addWidget(m_tabWidget, 1);

    // ============================================================
    // Signals
    // ============================================================
    connect(m_btnIssueSearch, &QPushButton::clicked, this, &WarehousePage::onPartsSearch);
    connect(m_btnIssue, &QPushButton::clicked, this, &WarehousePage::onPartsIssue);
    connect(m_issueTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_issuePartId = m_issueModel->data(m_issueModel->index(row, 0)).toInt();
        QString name = m_issueModel->data(m_issueModel->index(row, 2)).toString();
        int stock = m_issueModel->data(m_issueModel->index(row, 3)).toInt();
        m_lblIssuePartInfo->setText(QString("已选: %1 | 库存: %2").arg(name).arg(stock));
        m_spinIssueQty->setMaximum(stock);
    });

    connect(m_btnBillingSearch, &QPushButton::clicked, this, &WarehousePage::onBillingSearchOrder);
    connect(m_btnConfirmBill, &QPushButton::clicked, this, &WarehousePage::onCompareAndBill);

    connect(m_btnPurSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseSearch);
    connect(m_purTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_purPartId = m_purModel->data(m_purModel->index(row, 0)).toInt();
        m_purPartNo->setText(m_purModel->data(m_purModel->index(row, 1)).toString());
        m_purPartName->setText(m_purModel->data(m_purModel->index(row, 2)).toString());
        m_purSpec->setText(m_purModel->data(m_purModel->index(row, 3)).toString());
    });
    connect(m_btnPurConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseConfirm);

    connect(m_btnStockSearch, &QPushButton::clicked, this, &WarehousePage::onStockSearch);

    connect(m_btnRetSearch, &QPushButton::clicked, this, &WarehousePage::onReturnSearch);
    connect(m_retTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_retPartId = m_retModel->data(m_retModel->index(row, 0)).toInt();
        int stock = m_retModel->data(m_retModel->index(row, 3)).toInt();
        m_retQty->setMaximum(stock);
    });
    connect(m_btnRetConfirm, &QPushButton::clicked, this, &WarehousePage::onReturnConfirm);

    connect(m_btnPurRetSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnSearch);
    connect(m_purRetTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_purRetPartId = m_purRetModel->data(m_purRetModel->index(row, 0)).toInt();
        int stock = m_purRetModel->data(m_purRetModel->index(row, 3)).toInt();
        m_purRetQty->setMaximum(stock);
    });
    connect(m_btnPurRetConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnConfirm);
}

// ============================================================
// Tab 0: 备件领取 (Stage 2)
// ============================================================

void WarehousePage::onPartsSearch()
{
    QString keyword = m_issuePartSearch->text().trimmed();
    QSqlQuery q(DbManager::instance().database());
    if (keyword.isEmpty()) {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                  "FROM t_parts WHERE stock > 0 ORDER BY name LIMIT 200");
    } else {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                  "FROM t_parts WHERE (part_no LIKE :kw OR name LIKE :kw2) AND stock > 0 "
                  "ORDER BY name LIMIT 200");
        q.bindValue(":kw", "%" + keyword + "%");
        q.bindValue(":kw2", "%" + keyword + "%");
    }
    DbManager::instance().executeQuery(q);
    m_issueModel->setQuery(std::move(q));
    m_issuePartId = 0;
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

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    // 检查库存
    q.prepare("SELECT stock, sale_price, name FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_issuePartId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) { DbManager::instance().rollbackTransaction(); return; }
    int stock = q.value(0).toInt();
    double salePrice = q.value(1).toDouble();
    QString partName = q.value(2).toString();
    if (qty > stock) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "库存不足", QString("当前库存 %1，出库数量不能超过 %1").arg(stock));
        return;
    }

    // 扣减库存
    q.prepare("UPDATE t_parts SET stock = stock - :qty WHERE id = :id");
    q.bindValue(":qty", qty);
    q.bindValue(":id", m_issuePartId);
    if (!DbManager::instance().executeQuery(q)) { DbManager::instance().rollbackTransaction(); return; }

    // 记录流水
    q.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
              "operation_type, ref_order_no, operator_id, recipient) "
              "VALUES (:pid, :qty, :price, :total, '维修出库', :ref, :op, :rec)");
    q.bindValue(":pid", m_issuePartId);
    q.bindValue(":qty", -qty);
    q.bindValue(":price", salePrice);
    q.bindValue(":total", -qty * salePrice);
    q.bindValue(":ref", orderNo);
    q.bindValue(":op", Session::instance().userId());
    q.bindValue(":rec", recipient);
    DbManager::instance().executeQuery(q);

    // 写入工单备件明细（t_workorder_item）
    q.prepare("SELECT id FROM t_workorder WHERE order_no = :no");
    q.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(q);
    if (q.next()) {
        int woid = q.value(0).toInt();
        QSqlQuery q2(DbManager::instance().database());
        q2.prepare("INSERT INTO t_workorder_item (workorder_id, part_id, part_name, "
                    "quantity, unit_price, item_type) "
                    "VALUES (:oid, :pid, :name, :qty, :price, '材料')");
        q2.bindValue(":oid", woid);
        q2.bindValue(":pid", m_issuePartId);
        q2.bindValue(":name", partName);
        q2.bindValue(":qty", qty);
        q2.bindValue(":price", salePrice);
        DbManager::instance().executeQuery(q2);
    }

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "出库成功",
        QString("备件「%1」x %2 已出库\n工单: %3\n领取人: %4")
        .arg(partName).arg(qty).arg(orderNo).arg(recipient));
    onPartsSearch();
}

// ============================================================
// Tab 1: 材料结算/提单 (Stage 3)
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

    // 加载该工单的备件使用明细
    QSqlQuery q2(DbManager::instance().database());
    q2.prepare("SELECT part_name AS '备件名称', quantity AS '数量', "
               "unit_price AS '单价', subtotal AS '小计' "
               "FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料'");
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

    // 已派工 或 维修中 状态的工单可以提单
    if (status == "已派工" || status == "维修中") {
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
            QString("当前状态为「%1」，需要「已派工」或「维修中」才能提单").arg(status));
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

    // 获取材料费总额
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
              "WHERE workorder_id = :oid AND item_type = '材料'");
    q.bindValue(":oid", m_billingOrderId);
    DbManager::instance().executeQuery(q);
    double matTotal = q.next() ? q.value(0).toDouble() : 0;

    DbManager::instance().beginTransaction();

    // 更新工单状态为"已提单"，同时更新材料费
    // 允许从 已派工 或 维修中 状态提单
    q.prepare("UPDATE t_workorder SET status = '已提单', material_fee = :mat "
              "WHERE id = :id AND status IN ('已派工','维修中')");
    q.bindValue(":mat", matTotal);
    q.bindValue(":id", m_billingOrderId);
    if (!DbManager::instance().executeQuery(q) || q.numRowsAffected() == 0) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "提单失败", "状态更新失败，请确认工单状态为「已派工」或「维修中」");
        return;
    }

    // 记录交易历史
    q.prepare("SELECT vehicle_id FROM t_workorder WHERE id = :id");
    q.bindValue(":id", m_billingOrderId);
    DbManager::instance().executeQuery(q);
    if (q.next()) {
        int vid = q.value(0).toInt();
        QSqlQuery txn(DbManager::instance().database());
        txn.prepare("INSERT INTO t_vehicle_transaction (vehicle_id, workorder_id, "
                     "transaction_type, description, operator_id) "
                     "VALUES (:vid, :woid, '其他', :desc, :op)");
        txn.bindValue(":vid", vid);
        txn.bindValue(":woid", m_billingOrderId);
        txn.bindValue(":desc", QString("材料审核提单完成，材料费合计 ¥%1").arg(matTotal, 0, 'f', 2));
        txn.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(txn);
    }

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "提单成功",
        QString("工单材料审核已通过，已设置为「已提单」状态\n材料费合计: ¥%1\n前台可进行结算操作")
        .arg(matTotal, 0, 'f', 2));
    m_btnConfirmBill->setEnabled(false);
    // 更新显示文本中的状态
    {
        QString txt = m_lblBillingInfo->text();
        txt.replace("已派工", "已提单").replace("维修中", "已提单");
        m_lblBillingInfo->setText(txt);
    }
}

// ============================================================
// Tab 2: 采购入库
// ============================================================

void WarehousePage::onPurchaseSearch()
{
    QString keyword = m_purPartSearch->text().trimmed();
    QSqlQuery q(DbManager::instance().database());
    if (keyword.isEmpty()) {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "spec AS '规格', stock AS '库存', purchase_price AS '进货价', "
                  "sale_price AS '销售价', supplier AS '供应商' "
                  "FROM t_parts ORDER BY name LIMIT 200");
    } else {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "spec AS '规格', stock AS '库存', purchase_price AS '进货价', "
                  "sale_price AS '销售价', supplier AS '供应商' "
                  "FROM t_parts WHERE part_no LIKE :kw OR name LIKE :kw2 "
                  "ORDER BY name LIMIT 200");
        q.bindValue(":kw", "%" + keyword + "%");
        q.bindValue(":kw2", "%" + keyword + "%");
    }
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

    // 检查是否已有该备件编号
    q.prepare("SELECT id, stock FROM t_parts WHERE part_no = :no");
    q.bindValue(":no", partNo);
    DbManager::instance().executeQuery(q);

    if (q.next()) {
        // 已有备件 → 增加库存
        int existingId = q.value(0).toInt();
        q.prepare("UPDATE t_parts SET stock = stock + :qty, "
                  "purchase_price = :cost, sale_price = :price, supplier = :sup "
                  "WHERE id = :id");
        q.bindValue(":qty", qty);
        q.bindValue(":cost", cost);
        q.bindValue(":price", price);
        q.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        q.bindValue(":id", existingId);
        DbManager::instance().executeQuery(q);

        // 记录采购流水
        QSqlQuery q2(DbManager::instance().database());
        q2.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
                   "operation_type, operator_id) "
                   "VALUES (:pid, :qty, :price, :total, '采购入库', :op)");
        q2.bindValue(":pid", existingId);
        q2.bindValue(":qty", qty);
        q2.bindValue(":price", cost);
        q2.bindValue(":total", qty * cost);
        q2.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(q2);

        // 记录采购详情
        QSqlQuery q3(DbManager::instance().database());
        q3.prepare("INSERT INTO t_part_purchase (part_id, supplier, quantity, unit_cost, total_cost, operator_id) "
                   "VALUES (:pid, :sup, :qty, :cost, :total, :op)");
        q3.bindValue(":pid", existingId);
        q3.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        q3.bindValue(":qty", qty);
        q3.bindValue(":cost", cost);
        q3.bindValue(":total", qty * cost);
        q3.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(q3);
    } else {
        // 新建备件
        q.prepare("INSERT INTO t_parts (part_no, name, spec, stock, purchase_price, sale_price, supplier) "
                  "VALUES (:no, :name, :spec, :qty, :cost, :price, :sup)");
        q.bindValue(":no", partNo);
        q.bindValue(":name", partName);
        q.bindValue(":spec", spec.isEmpty() ? QVariant(QString()) : spec);
        q.bindValue(":qty", qty);
        q.bindValue(":cost", cost);
        q.bindValue(":price", price);
        q.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        DbManager::instance().executeQuery(q);
        int newId = q.lastInsertId().toInt();

        // 记录采购流水
        QSqlQuery q2(DbManager::instance().database());
        q2.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
                   "operation_type, operator_id) "
                   "VALUES (:pid, :qty, :price, :total, '采购入库', :op)");
        q2.bindValue(":pid", newId);
        q2.bindValue(":qty", qty);
        q2.bindValue(":price", cost);
        q2.bindValue(":total", qty * cost);
        q2.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(q2);

        // 记录采购详情
        QSqlQuery q3(DbManager::instance().database());
        q3.prepare("INSERT INTO t_part_purchase (part_id, supplier, quantity, unit_cost, total_cost, operator_id) "
                   "VALUES (:pid, :sup, :qty, :cost, :total, :op)");
        q3.bindValue(":pid", newId);
        q3.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        q3.bindValue(":qty", qty);
        q3.bindValue(":cost", cost);
        q3.bindValue(":total", qty * cost);
        q3.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(q3);
    }

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "入库成功",
        QString("备件「%1」x %2 已入库").arg(partName).arg(qty));
    onPurchaseSearch();
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purCost->setValue(0); m_purPrice->setValue(0);
    m_purQty->setValue(1);
}

// ============================================================
// Tab 3: 库存查询
// ============================================================

void WarehousePage::onStockSearch()
{
    QString keyword = m_stockKeyword->text().trimmed();
    QSqlQuery q(DbManager::instance().database());
    if (keyword.isEmpty()) {
        q.prepare("SELECT part_no AS '备件编号', name AS '备件名称', "
                  "spec AS '规格型号', stock AS '库存量', "
                  "purchase_price AS '进货价', sale_price AS '销售价', "
                  "supplier AS '供应商' "
                  "FROM t_parts ORDER BY name LIMIT 500");
    } else {
        q.prepare("SELECT part_no AS '备件编号', name AS '备件名称', "
                  "spec AS '规格型号', stock AS '库存量', "
                  "purchase_price AS '进货价', sale_price AS '销售价', "
                  "supplier AS '供应商' "
                  "FROM t_parts WHERE part_no LIKE :kw OR name LIKE :kw2 "
                  "OR spec LIKE :kw3 OR supplier LIKE :kw4 "
                  "ORDER BY name LIMIT 500");
        q.bindValue(":kw", "%" + keyword + "%");
        q.bindValue(":kw2", "%" + keyword + "%");
        q.bindValue(":kw3", "%" + keyword + "%");
        q.bindValue(":kw4", "%" + keyword + "%");
    }
    DbManager::instance().executeQuery(q);
    m_stockModel->setQuery(std::move(q));
}

// ============================================================
// Tab 4: 备件退库
// ============================================================

void WarehousePage::onReturnSearch()
{
    QString keyword = m_retPartSearch->text().trimmed();
    QSqlQuery q(DbManager::instance().database());
    if (keyword.isEmpty()) {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                  "FROM t_parts ORDER BY name LIMIT 200");
    } else {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                  "FROM t_parts WHERE part_no LIKE :kw OR name LIKE :kw2 "
                  "ORDER BY name LIMIT 200");
        q.bindValue(":kw", "%" + keyword + "%");
        q.bindValue(":kw2", "%" + keyword + "%");
    }
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

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    q.prepare("SELECT stock, purchase_price, name FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_retPartId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) { DbManager::instance().rollbackTransaction(); return; }
    int stock = q.value(0).toInt();
    double costPrice = q.value(1).toDouble();
    QString name = q.value(2).toString();
    if (qty > stock) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "库存不足", QString("当前库存 %1，退库数量不能超过 %1").arg(stock));
        return;
    }

    // 增加库存
    q.prepare("UPDATE t_parts SET stock = stock + :qty WHERE id = :id");
    q.bindValue(":qty", qty);
    q.bindValue(":id", m_retPartId);
    DbManager::instance().executeQuery(q);

    // 记录流水
    q.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
              "operation_type, ref_order_no, operator_id, remark) "
              "VALUES (:pid, :qty, :price, :total, '备件退库', :ref, :op, '备件退库')");
    q.bindValue(":pid", m_retPartId);
    q.bindValue(":qty", qty);
    q.bindValue(":price", costPrice);
    q.bindValue(":total", qty * costPrice);
    q.bindValue(":ref", orderNo.isEmpty() ? QVariant(QString()) : orderNo);
    q.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退库成功",
        QString("备件「%1」x %2 已退库").arg(name).arg(qty));
    onReturnSearch();
}

// ============================================================
// Tab 5: 采购退货
// ============================================================

void WarehousePage::onPurchaseReturnSearch()
{
    QString keyword = m_purRetPartSearch->text().trimmed();
    QSqlQuery q(DbManager::instance().database());
    if (keyword.isEmpty()) {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', purchase_price AS '进货价', supplier AS '供应商' "
                  "FROM t_parts ORDER BY name LIMIT 200");
    } else {
        q.prepare("SELECT id, part_no AS '编号', name AS '名称', "
                  "stock AS '库存', purchase_price AS '进货价', supplier AS '供应商' "
                  "FROM t_parts WHERE part_no LIKE :kw OR name LIKE :kw2 "
                  "ORDER BY name LIMIT 200");
        q.bindValue(":kw", "%" + keyword + "%");
        q.bindValue(":kw2", "%" + keyword + "%");
    }
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

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    q.prepare("SELECT stock, purchase_price, name FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_purRetPartId);
    DbManager::instance().executeQuery(q);
    if (!q.next()) { DbManager::instance().rollbackTransaction(); return; }
    int stock = q.value(0).toInt();
    double costPrice = q.value(1).toDouble();
    QString name = q.value(2).toString();
    if (qty > stock) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "库存不足", QString("当前库存 %1，退货数量不能超过 %1").arg(stock));
        return;
    }

    // 扣减库存
    q.prepare("UPDATE t_parts SET stock = stock - :qty WHERE id = :id");
    q.bindValue(":qty", qty);
    q.bindValue(":id", m_purRetPartId);
    DbManager::instance().executeQuery(q);

    // 记录流水
    q.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
              "operation_type, operator_id, remark) "
              "VALUES (:pid, :qty, :price, :total, '采购退货', :op, '采购退货')");
    q.bindValue(":pid", m_purRetPartId);
    q.bindValue(":qty", -qty);
    q.bindValue(":price", costPrice);
    q.bindValue(":total", -qty * costPrice);
    q.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退货成功",
        QString("备件「%1」x %2 已退货").arg(name).arg(qty));
    onPurchaseReturnSearch();
}
