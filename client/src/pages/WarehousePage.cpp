#include "WarehousePage.h"
#include "database/Session.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"
#include "remote/RemoteDb.h"
#include "remote/SqlUtil.h"

#include <QAbstractSpinBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QSqlRecord>
#include <QDialog>
#include <QTableWidget>
#include <QJsonArray>
#include <QJsonValue>
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
        "COALESCE(p.supplier,'') AS '供应商', "
        "COALESCE(p.applicable_model,'') AS '适用车型' "
        "%1 "
        "FROM t_parts p "
        "JOIN t_part_instance i ON i.part_id = p.id "
        "%2 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.applicable_model %3 "
        "%4")
        .arg(extraCols.isEmpty() ? "" : ", " + extraCols,
             whereClause.isEmpty() ? "" : "WHERE " + whereClause,
             groupBy.isEmpty() ? "" : ", " + groupBy,
             orderBy.isEmpty() ? "ORDER BY p.name LIMIT 500" : orderBy);
    return sql;
}

QString WarehousePage::generateInstanceSN(const QString &partNo, int partId) const
{
    RemoteQuery q;
    q.prepare("SELECT COUNT(*) FROM t_part_instance WHERE part_id = :pid");
    q.bindValue(":pid", partId);
    q.exec();
    int cnt = 0;
    if (q.next()) cnt = q.value(0).toInt();
    return QString("%1-%2").arg(partNo).arg(cnt + 1, 4, 10, QChar('0'));
}

QList<int> WarehousePage::getInStockInstanceIds(int partId, int count) const
{
    QList<int> ids;
    RemoteQuery q;
    q.prepare("SELECT id FROM t_part_instance "
              "WHERE part_id = :pid AND status = '在库' "
              "ORDER BY id ASC LIMIT :lim");
    q.bindValue(":pid", partId);
    q.bindValue(":lim", count);
    q.exec();
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

QList<int> WarehousePage::getCheckedOutInstanceIds(int partId, int count) const
{
    QList<int> ids;
    RemoteQuery q;
    q.prepare("SELECT id FROM t_part_instance "
              "WHERE part_id = :pid AND status = '已领出' "
              "ORDER BY id ASC LIMIT :lim");
    q.bindValue(":pid", partId);
    q.bindValue(":lim", count);
    q.exec();
    while (q.next()) ids << q.value(0).toInt();
    return ids;
}

bool WarehousePage::updateInstanceStatus(const QList<int> &instanceIds, const QString &newStatus,
                                          int vehicleId, int workorderId,
                                          const QString &recipient)
{
    if (instanceIds.isEmpty()) return true;

    RemoteQuery q;
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

    return q.exec();
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
    m_purSupplier->clear(); m_purApplicableModel->clear();
    m_purCost->setValue(0); m_purPrice->setValue(0);
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
    m_issueModel = new RemoteModel(this);
    m_issueTable->setModel(m_issueModel);
    m_issueTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 备件领取：只读

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

    m_billingTable = new QTableWidget(0, 5);
    m_billingTable->setHorizontalHeaderLabels({"备件名称", "数量", "单价", "小计", "成本"});
    m_billingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_billingTable->setAlternatingRowColors(true);
    m_billingTable->horizontalHeader()->setStretchLastSection(true);
    m_billingTable->verticalHeader()->setVisible(false);
    m_billingTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:5px;}");
    billLayout->addWidget(m_billingTable, 1);
    m_billingTable->setEditTriggers(QAbstractItemView::DoubleClicked);   // 单价列可双击编辑

    QHBoxLayout *billBottom = new QHBoxLayout;
    m_lblBillingTotal = new QLabel("材料费合计: ¥0.00 | 总成本: ¥0.00");
    m_lblBillingTotal->setStyleSheet("font-size:15px;font-weight:bold;color:#e74c3c;");
    billBottom->addWidget(m_lblBillingTotal);
    billBottom->addStretch();
    m_btnConfirmBill = new QPushButton("确认提单");
    m_btnConfirmBill->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#8e44ad;color:#fff;font-weight:bold;font-size:14px;}QPushButton:hover{background:#7d3c98;}");
    m_btnConfirmBill->setVisible(false);   // 仅锁定工单且状态为「待提单」时显示
    billBottom->addWidget(m_btnConfirmBill);

    m_btnCancelBill = new QPushButton("取消提单");
    m_btnCancelBill->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#c0392b;color:#fff;font-weight:bold;font-size:14px;}QPushButton:hover{background:#a93226;}");
    m_btnCancelBill->setVisible(false);    // 仅锁定工单且状态为「已提单」时显示
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
    m_purTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:2px 4px;font-size:11px;}");
    purLayout->addWidget(m_purTable, 1);
    // 本批入库清单：表头+内容字号调小、行紧凑
    {
        QFont tf = m_purTable->font();
        tf.setPixelSize(11);
        m_purTable->setFont(tf);
        m_purTable->verticalHeader()->setDefaultSectionSize(20);
    }
    m_purModel = new QStandardItemModel(this);
    m_purModel->setHorizontalHeaderLabels(
        {"备件编号","备件名称","规格型号","供应商","适用车型","进货价","销售价","数量","小计(¥)"});
    m_purTable->setModel(m_purModel);
    m_purTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 采购入库清单：只读
    // 本批入库清单：各列宽度调窄
    {
        const int purWidths[] = {72, 88, 62, 68, 88, 56, 56, 42, 66};
        for (int c = 0; c < 9; ++c)
            m_purTable->setColumnWidth(c, purWidths[c]);
    }

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
    m_purSupplier = new QLineEdit; m_purSupplier->setPlaceholderText("*必填");
    m_purApplicableModel = new QLineEdit; m_purApplicableModel->setPlaceholderText("选填，如:丰田凯美瑞/本田雅阁");
    m_purCost = new QDoubleSpinBox; m_purCost->setRange(0, 999999.99); m_purCost->setPrefix("¥ "); m_purCost->setDecimals(2);
    m_purCost->setSpecialValueText("0");   // 默认显示 0，不再显示 0.00
    m_purPrice = new QDoubleSpinBox; m_purPrice->setRange(0, 999999.99); m_purPrice->setPrefix("¥ "); m_purPrice->setDecimals(2);
    m_purPrice->setSpecialValueText("0");  // 默认显示 0，不再显示 0.00
    m_purQty = new QSpinBox; m_purQty->setRange(1, 99999);

    // 采购入库输入区样式：字体调大、高度刚好容纳字体、内边距为 0
    {
        const QList<QWidget*> purInputs = {
            m_purPartSearch, m_purPartNo, m_purPartName,
            m_purSpec, m_purSupplier, m_purApplicableModel, m_purCost, m_purPrice, m_purQty
        };
        for (QWidget *w : purInputs) {
            QFont f = w->font();
            f.setPixelSize(18);
            w->setFont(f);
            w->setFixedHeight(QFontMetrics(f).height() + 6);
            w->setStyleSheet("padding: 0px;");
        }
    }

    // 输入区标签：字体与输入框一致设为 18px
    auto makePurLabel = [](const QString &txt) {
        QLabel *lbl = new QLabel(txt);
        QFont f = lbl->font();
        f.setPixelSize(18);
        lbl->setFont(f);
        return lbl;
    };
    m_btnPurAddItem = new QPushButton("加入清单");
    m_btnPurAddItem->setStyleSheet("QPushButton{padding:8px 20px;border:none;border-radius:3px;background:#3498db;color:#fff;font-size:14px;font-weight:bold;}QPushButton:hover{background:#2980b9;}");
    m_btnPurConfirm = new QPushButton("确认入库");
    m_btnPurConfirm->setStyleSheet("QPushButton{padding:9px 20px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-weight:bold;font-size:15px;}QPushButton:hover{background:#219a52;}");

    // 布局（6 列）：
    //  第0行: 模糊搜索 + 输入框
    //  第1行: 备件编号 / 备件名称 / 规格型号
    //  第2行: 适用车型 / 供应商
    //  第3行: 进货价 / 销售价 / 数量
    //  第4行: 三个按钮（搜索 / 加入清单 / 确认入库）一起放最下面
    purGrid->addWidget(makePurLabel("模糊搜索:"), 0, 0);
    purGrid->addWidget(m_purPartSearch, 0, 1, 1, 5);
    purGrid->addWidget(makePurLabel("备件编号*:"), 1, 0); purGrid->addWidget(m_purPartNo, 1, 1);
    purGrid->addWidget(makePurLabel("备件名称*:"), 1, 2); purGrid->addWidget(m_purPartName, 1, 3);
    purGrid->addWidget(makePurLabel("规格型号:"), 1, 4); purGrid->addWidget(m_purSpec, 1, 5);
    purGrid->addWidget(makePurLabel("适用车型(选填):"), 2, 0); purGrid->addWidget(m_purApplicableModel, 2, 1, 1, 2);
    purGrid->addWidget(makePurLabel("供应商*:"), 2, 3); purGrid->addWidget(m_purSupplier, 2, 4, 1, 2);
    purGrid->addWidget(makePurLabel("进货价:"), 3, 0); purGrid->addWidget(m_purCost, 3, 1);
    purGrid->addWidget(makePurLabel("销售价:"), 3, 2); purGrid->addWidget(m_purPrice, 3, 3);
    purGrid->addWidget(makePurLabel("数量:"), 3, 4); purGrid->addWidget(m_purQty, 3, 5);
    purGrid->addWidget(m_btnPurSearch, 4, 0, 1, 2);
    purGrid->addWidget(m_btnPurAddItem, 4, 2, 1, 2);
    purGrid->addWidget(m_btnPurConfirm, 4, 4, 1, 2);
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
    m_stockModel = new RemoteModel(this);
    m_stockTable->setModel(m_stockModel);
    m_stockTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 库存查询：只读

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
    m_retModel = new RemoteModel(this);
    m_retTable->setModel(m_retModel);
    m_retTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 备件退库：只读

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
    m_retStatusBar = new QLabel("未锁定工单，请先搜索并锁定工单");
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
    m_purRetModel = new RemoteModel(this);
    m_purRetTable->setModel(m_purRetModel);
    m_purRetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);  // 采购退货：只读

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
    connect(m_issuePartSearch, &QLineEdit::returnPressed, this, &WarehousePage::onPartsSearch);
    connect(m_btnIssueSearch, &QPushButton::clicked, this, &WarehousePage::onPartsSearch);
    // 备件领取备件模糊搜索：每次修改输入即触发搜索并刷新结果
    connect(m_issuePartSearch, &QLineEdit::textChanged, this, &WarehousePage::onPartsSearch);
    connect(m_btnIssue, &QPushButton::clicked, this, &WarehousePage::onPartsIssue);
    connect(m_issueTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        // 合并视图中 column 0 = catalog_id
        m_issuePartId = m_issueModel->data(m_issueModel->index(row, 0)).toInt();
        QString name = m_issueModel->data(m_issueModel->index(row, 2)).toString();
        int stock = m_issueModel->data(m_issueModel->index(row, 6)).toInt(); // 在库数量
        m_lblIssuePartInfo->setText(QString("已选: %1 | 在库: %2").arg(name).arg(stock));
        m_spinIssueQty->setMaximum(stock > 0 ? stock : 1);
    });

    // 工单多结果下拉：备件领取 / 结算提单 各自挂靠一个输入框
    m_issueWoCompleter = new SearchCompleter(this);
    m_issueOrderNo->installEventFilter(this);   // 先装本页过滤，再让 completer 后装（下拉打开时 completer 优先处理回车/方向键）
    m_issueWoCompleter->setEdit(m_issueOrderNo);
    connect(m_issueWoCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx < 0 || idx >= m_woRows.size()) return;
        m_issueOrderNo->blockSignals(true);
        m_issueOrderNo->setText(m_woRows[idx][0]);   // 填入工单号
        m_issueOrderNo->blockSignals(false);
        updateIssueOrderStatus();
    });
    // 输入/删除即时搜索刷新下拉（放在 completer setEdit 之后：先收起过期下拉再重新搜索）
    connect(m_issueOrderNo, &QLineEdit::textChanged, this, &WarehousePage::onIssueOrderSearchTextChanged);

    m_billingWoCompleter = new SearchCompleter(this);
    m_billingOrderNo->installEventFilter(this);  // 同 m_issueOrderNo：completer 后装、优先
    m_billingWoCompleter->setEdit(m_billingOrderNo);
    connect(m_billingWoCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx < 0 || idx >= m_woRows.size()) return;
        m_billingOrderNo->blockSignals(true);
        m_billingOrderNo->setText(m_woRows[idx][0]);
        m_billingOrderNo->blockSignals(false);
        onBillingSearchOrder();   // 自动触发票据搜索加载明细
    });

    // 清空工单号时释放锁定：隐藏提单按钮并清空明细
    connect(m_billingOrderNo, &QLineEdit::textChanged, this, [this](const QString &txt) {
        if (txt.trimmed().isEmpty()) {
            m_billingOrderId = 0;
            m_billingTable->setRowCount(0);
            m_lblBillingInfo->setText("请搜索工单");
            m_lblBillingTotal->setText("材料费合计: ¥0.00 | 总成本: ¥0.00");
            updateBillButtons(0, QString());
        }
    });
    // 输入/删除即时搜索刷新下拉（放在 completer setEdit 之后：先收起过期下拉再重新搜索）
    connect(m_billingOrderNo, &QLineEdit::textChanged, this, &WarehousePage::onBillingOrderSearchTextChanged);
    connect(m_btnConfirmBill, &QPushButton::clicked, this, &WarehousePage::onCompareAndBill);
    connect(m_btnCancelBill, &QPushButton::clicked, this, &WarehousePage::onCancelBill);

    // 材料结算表格：单价列编辑后重算小计与合计（数量/材料费总额不可改）
    connect(m_billingTable, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (col == 2) {
            QTableWidgetItem *priceItem = m_billingTable->item(row, 2);
            QTableWidgetItem *qtyItem   = m_billingTable->item(row, 1);
            QTableWidgetItem *subItem   = m_billingTable->item(row, 3);
            if (priceItem && qtyItem && subItem) {
                double price = priceItem->text().remove("¥").toDouble();
                int    qty   = qtyItem->text().toInt();
                subItem->setText(QString("¥%1").arg(qty * price, 0, 'f', 2));
            }
        }
        double total = 0, cost = 0;
        for (int r = 0; r < m_billingTable->rowCount(); r++) {
            QTableWidgetItem *si = m_billingTable->item(r, 3);
            if (si) total += si->text().remove("¥").toDouble();
            QTableWidgetItem *ci = m_billingTable->item(r, 4);
            if (ci) cost += ci->text().remove("¥").toDouble();
        }
        m_lblBillingTotal->setText(QString("材料费合计: ¥%1 | 总成本: ¥%2")
                                   .arg(total, 0, 'f', 2).arg(cost, 0, 'f', 2));
    });

    connect(m_btnPurSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseSearch);

    // 采购入库备件多结果下拉：选中即回填 编号/名称/规格
    m_purCompleter = new SearchCompleter(this);
    m_purPartSearch->installEventFilter(this);   // 同 m_issueOrderNo：completer 后装、优先
    m_purCompleter->setEdit(m_purPartSearch);
    connect(m_purCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx < 0 || idx >= m_purRows.size()) return;
        m_purPartNo->setText(m_purRows[idx][0]);
        m_purPartName->setText(m_purRows[idx][1]);
        m_purSpec->setText(m_purRows[idx][2]);
        m_purSupplier->setText(m_purRows[idx][3]);
        m_purApplicableModel->setText(m_purRows[idx][4]);
        m_purPartNo->setFocus();
    });
    // 输入/删除即时搜索刷新下拉（放在 completer setEdit 之后：先收起过期下拉再重新搜索）
    connect(m_purPartSearch, &QLineEdit::textChanged, this, &WarehousePage::onPurchaseSearch);
    // 进货价填写结束 → 自动按 40% 加价率填入销售价
    connect(m_purCost, &QDoubleSpinBox::editingFinished, this, [this]() {
        m_purPrice->setValue(qRound64(m_purCost->value() * 1.4 * 100) / 100.0);
    });
    connect(m_btnPurAddItem, &QPushButton::clicked, this, &WarehousePage::onPurchaseAddItem);
    connect(m_btnPurRemoveItem, &QPushButton::clicked, this, &WarehousePage::onPurchaseRemoveItem);
    connect(m_btnPurConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseConfirm);

    connect(m_btnStockSearch, &QPushButton::clicked, this, &WarehousePage::onStockSearch);
    // 库存查询模糊搜索：每次修改输入即触发搜索并刷新结果
    connect(m_stockKeyword, &QLineEdit::textChanged, this, &WarehousePage::onStockSearch);

    // 备件退库 — 工单搜索 + 状态栏更新 + 锁定工单ID
    // 工单/车牌输入完成：回车、Tab 或失焦自动锁定工单
    m_retWoCompleter = new SearchCompleter(this);
    m_retOrderNo->installEventFilter(this);      // 同 m_issueOrderNo：completer 后装、优先
    m_retWoCompleter->setEdit(m_retOrderNo);
    connect(m_retWoCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx < 0 || idx >= m_woRows.size()) return;
        m_retOrderNo->blockSignals(true);
        m_retOrderNo->setText(m_woRows[idx][0]);   // 填入工单号
        m_retOrderNo->blockSignals(false);
        updateReturnOrderStatus();
        onReturnSearch();   // 锁定工单后立即刷新备件列表（只显示该工单绑定的备件）
    });
    // 输入/删除即时搜索刷新下拉（放在 completer setEdit 之后：先收起过期下拉再重新搜索）
    connect(m_retOrderNo, &QLineEdit::textChanged, this, [this](const QString &) {
        showWorkOrderSearchPopup(m_retOrderNo, m_retWoCompleter);
    });
    // 清空工单号时重置锁定并清空备件列表
    connect(m_retOrderNo, &QLineEdit::textChanged, this, [this](const QString &txt) {
        if (txt.trimmed().isEmpty()) {
            m_retLockedWorkOrderId = 0;
            m_retStatusBar->setText("未锁定工单，请先搜索并锁定工单");
            onReturnSearch();   // 清空列表
        }
    });
    connect(m_btnRetSearch, &QPushButton::clicked, this, &WarehousePage::onReturnSearch);
    // 备件退库模糊搜索：每次修改输入即触发搜索并刷新结果（原 editingFinished 改为逐键触发）
    connect(m_retPartSearch, &QLineEdit::textChanged, this, &WarehousePage::onReturnSearch);
    connect(m_retTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_retPartId = m_retModel->data(m_retModel->index(row, 0)).toInt();
        int available = m_retModel->data(m_retModel->index(row, 6)).toInt(); // 可退库数量
        m_retQty->setMaximum(available > 0 ? available : 1);
    });
    connect(m_btnRetConfirm, &QPushButton::clicked, this, &WarehousePage::onReturnConfirm);

    connect(m_btnPurRetSearch, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnSearch);
    // 采购退货模糊搜索：每次修改输入即触发搜索并刷新结果
    connect(m_purRetPartSearch, &QLineEdit::textChanged, this, &WarehousePage::onPurchaseReturnSearch);
    connect(m_purRetTable, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_purRetPartId = m_purRetModel->data(m_purRetModel->index(row, 0)).toInt();
        int inStock = m_purRetModel->data(m_purRetModel->index(row, 6)).toInt(); // 在库数量
        m_purRetQty->setMaximum(inStock > 0 ? inStock : 1);
    });
    connect(m_btnPurRetConfirm, &QPushButton::clicked, this, &WarehousePage::onPurchaseReturnConfirm);

    // ============================================================
    // 键盘导航事件过滤
    //   - Tab 栏：默认先选择 tab（左右切换，回车/鼠标进入该 tab）
    //   - 每个 tab：输入焦点按「从左到右、再从上到下」跳转（回车/Tab 前进）
    //   - 搜索框 + 结果列表（需求3）：搜索框回车有结果跳列表第一行（无结果不跳）；
    //     列表回车确认选中备件并跳下一输入区
    // ============================================================
    m_tabWidget->tabBar()->setFocusPolicy(Qt::StrongFocus);
    m_tabWidget->tabBar()->installEventFilter(this);

    // Tab 0 备件领取（m_issueOrderNo 已在上方 completer 前安装）
    m_issueRecipient->installEventFilter(this);
    m_issuePartSearch->installEventFilter(this);
    m_issueTable->installEventFilter(this);
    installNavFilter(m_spinIssueQty);
    m_btnIssue->installEventFilter(this);

    // Tab 1 材料结算（m_billingOrderNo 已在上方 completer 前安装）
    m_billingTable->installEventFilter(this);
    m_btnConfirmBill->installEventFilter(this);
    m_btnCancelBill->installEventFilter(this);

    // Tab 2 采购入库（m_purPartSearch 已在上方 completer 前安装）
    m_purPartNo->installEventFilter(this);
    m_purPartName->installEventFilter(this);
    m_purSpec->installEventFilter(this);
    m_purSupplier->installEventFilter(this);
    m_purApplicableModel->installEventFilter(this);
    installNavFilter(m_purCost);
    installNavFilter(m_purPrice);
    installNavFilter(m_purQty);
    m_btnPurAddItem->installEventFilter(this);
    m_btnPurConfirm->installEventFilter(this);

    // Tab 3 库存查询
    m_stockKeyword->installEventFilter(this);
    m_stockTable->installEventFilter(this);

    // Tab 4 备件退库（m_retOrderNo 已在上方 completer 前安装）
    m_retPartSearch->installEventFilter(this);
    m_retTable->installEventFilter(this);
    installNavFilter(m_retQty);
    m_btnRetConfirm->installEventFilter(this);

    // Tab 5 采购退货
    m_purRetPartSearch->installEventFilter(this);
    m_purRetTable->installEventFilter(this);
    installNavFilter(m_purRetQty);
    m_btnPurRetConfirm->installEventFilter(this);
}

// ============================================================
// 工单搜索弹窗（共用）— 按工单号/车牌模糊搜索，仅"已派工"工单
// ============================================================

void WarehousePage::showWorkOrderSearchPopup(QLineEdit *targetField, SearchCompleter *completer,
                                             const QString &statusFilter)
{
    QString text = targetField->text().trimmed();
    if (text.isEmpty()) {
        completer->hideDropdown();
        return;
    }

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

    RemoteQuery q;
    q.prepare(QString(
        "SELECT w.id, w.order_no, w.status, COALESCE(v.plate_number,'') AS plate, "
        "w.repair_content, w.created_at "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE %2 "
        "AND %1 "
        "ORDER BY w.id DESC LIMIT 30").arg(SqlUtil::likeConds(
            {"w.order_no", "v.plate_number"}, ":kw"), statusWhere));
    q.bindValue(":kw", SqlUtil::likePattern(text));
    q.exec();

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

    if (rows.isEmpty()) {
        completer->hideDropdown();
        return;                       // 无匹配 → 收起下拉，不弹"未找到"
    }

    // 无论 1 条还是多条，都在输入框下方自动展开下拉，仅在用户主动选择时回填
    m_woRows = rows;
    QList<QVariant> ids;
    for (int i = 0; i < rows.size(); ++i)
        ids << i;                     // 用行号作为 id，选中后按 m_woRows 回填
    completer->setResults(rows, ids);
    completer->showDropdown();
}

// ============================================================
// updateIssueOrderStatus — 备件领取：刷新锁定工单的状态栏（车牌号）
// ============================================================
void WarehousePage::updateIssueOrderStatus()
{
    QString orderNo = m_issueOrderNo->text().trimmed();
    RemoteQuery q;
    q.prepare("SELECT v.plate_number FROM t_workorder w "
              "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
              "WHERE w.order_no = :no");
    q.bindValue(":no", orderNo);
    q.exec();
    if (q.next() && !q.value(0).toString().isEmpty()) {
        m_issueStatusBar->setText(
            QString("当前工单: %1 | 车牌号: %2").arg(orderNo, q.value(0).toString()));
    } else {
        m_issueStatusBar->setText(QString("当前工单: %1").arg(orderNo));
    }
    m_issueStatusBar->setVisible(true);
}

// ============================================================
// updateReturnOrderStatus — 备件退库：锁定工单ID + 刷新状态栏
// ============================================================
void WarehousePage::updateReturnOrderStatus()
{
    QString orderNo = m_retOrderNo->text().trimmed();
    RemoteQuery q;
    q.prepare("SELECT w.id, v.plate_number FROM t_workorder w "
              "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
              "WHERE w.order_no = :no");
    q.bindValue(":no", orderNo);
    q.exec();
    if (q.next()) {
        m_retLockedWorkOrderId = q.value(0).toInt();
        QString plate = q.value(1).toString();
        if (!plate.isEmpty())
            m_retStatusBar->setText(QString("当前工单: %1 | 车牌号: %2 | 仅搜索本工单已领出备件").arg(orderNo, plate));
        else
            m_retStatusBar->setText(QString("当前工单: %1 | 仅搜索本工单已领出备件").arg(orderNo));
    }
}

// ============================================================
// 备件领取 — 工单搜索（textChanged 触发）
// ============================================================
void WarehousePage::onIssueOrderSearchTextChanged(const QString &)
{
    // 实时搜索只刷新下拉；无匹配自动收起（不弹"未找到"），选中后由 selected 回调更新状态栏
    showWorkOrderSearchPopup(m_issueOrderNo, m_issueWoCompleter);
}

// ============================================================
// 材料结算/提单 — 工单搜索（textChanged 触发）
// ============================================================
void WarehousePage::onBillingOrderSearchTextChanged(const QString &)
{
    // 实时搜索只刷新下拉；无匹配自动收起（不弹"未找到"），选中后由 selected 回调加载明细
    showWorkOrderSearchPopup(m_billingOrderNo, m_billingWoCompleter, "待提单,已提单");
}

// ============================================================
// Tab 0: 备件领取 (Stage 2) — 合并显示 + 实例级出库
// ============================================================

void WarehousePage::onPartsSearch()
{
    QString keyword = m_issuePartSearch->text().trimmed();
    RemoteQuery q;

    // LEFT JOIN: 显示所有备件(含库存为0的)，按模糊关键字搜索
    QString likeWhere;
    if (!keyword.isEmpty())
        likeWhere = "WHERE " + SqlUtil::likeConds(
            {"p.part_no", "p.name", "p.supplier", "p.spec"}, ":kw");
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COALESCE(p.applicable_model,'') AS '适用车型', "
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '在库数量', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "LEFT JOIN t_part_instance i ON i.part_id = p.id "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.applicable_model, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(likeWhere);

    q.prepare(sql);
    if (!keyword.isEmpty())
        q.bindValue(":kw", SqlUtil::likePattern(keyword));
    q.exec();
    m_issueModel->setQuery(q);
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
    RemoteQuery q;
    q.prepare("SELECT part_no, name, "
              "COALESCE(NULLIF(sale_price, 0), purchase_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_issuePartId);
    q.exec();
    if (!q.next()) return;
    QString partNo = q.value(0).toString();
    QString partName = q.value(1).toString();
    double salePrice = q.value(2).toDouble();

    // 查找工单ID
    q.prepare("SELECT id, vehicle_id FROM t_workorder WHERE order_no = :no");
    q.bindValue(":no", orderNo);
    q.exec();
    int woid = 0;
    if (q.next()) woid = q.value(0).toInt();

    // 未绑定工单不能出库：工单号必须真实存在于 t_workorder，否则实例无法绑定工单
    if (woid == 0) {
        QMessageBox::warning(this, "无法出库",
            QString("工单「%1」不存在，未绑定工单时不能出库").arg(orderNo));
        return;
    }

    // 领料出库在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    {
        // 更新实例状态为已领出（IN 子句，替代原 updateInstanceStatus 辅助函数）
        QStringList phs;
        for (int i = 0; i < instanceIds.size(); i++)
            phs << QString(":id%1").arg(i);
        QJsonObject params;
        params[":st"] = "已领出";
        if (woid > 0) params[":wid"] = woid;
        if (!recipient.isEmpty()) params[":rec"] = recipient;
        for (int i = 0; i < instanceIds.size(); i++)
            params[QString(":id%1").arg(i)] = instanceIds[i];
        steps.append(RemoteDb::step(
            "UPDATE t_part_instance SET status = :st, updated_at = NOW() "
            + QString(woid > 0 ? ", workorder_id = :wid " : "")
            + QString(!recipient.isEmpty() ? ", recipient = :rec " : "")
            + "WHERE id IN (" + phs.join(",") + ")",
            params));
    }

    // 更新目录表库存缓存
    steps.append(RemoteDb::step(
        "UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
        "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2",
        QJsonObject{ { ":pid", m_issuePartId }, { ":pid2", m_issuePartId } }));

    // 记录流水(每个实例一条)
    for (int instId : instanceIds) {
        steps.append(RemoteDb::step(
            "INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
            "operation_type, ref_order_no, operator_id, recipient) "
            "VALUES (:pid, :iid, -1, :price, :total, '维修出库', :ref, :op, :rec)",
            QJsonObject{
                { ":pid", m_issuePartId }, { ":iid", instId },
                { ":price", salePrice }, { ":total", -salePrice },
                { ":ref", orderNo }, { ":op", Session::instance().userId() },
                { ":rec", recipient },
            }));
    }

    // 写入工单备件明细
    if (woid > 0) {
        for (int instId : instanceIds) {
            steps.append(RemoteDb::step(
                "INSERT INTO t_workorder_item (workorder_id, part_id, part_instance_id, part_name, "
                "quantity, unit_price, item_type) "
                "VALUES (:oid, :pid, :iid, :name, 1, :price, '材料')",
                QJsonObject{
                    { ":oid", woid }, { ":pid", m_issuePartId }, { ":iid", instId },
                    { ":name", partName }, { ":price", salePrice },
                }));
        }
    }

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "出库失败", txn.value("error").toString());
        return;
    }

    // 同步更新维修历史：备件摘要
    if (woid > 0) {
        QStringList psList;
        RemoteQuery psq;
        psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                    "WHERE workorder_id = :oid AND item_type = '材料' "
                    "GROUP BY part_name");
        psq.bindValue(":oid", woid);
        psq.exec();
        while (psq.next())
            psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
        RemoteQuery mu;
        mu.prepare("UPDATE t_maintenance_history SET parts_summary=:ps WHERE workorder_id=:oid");
        mu.bindValue(":ps", psList.isEmpty() ? QString() : psList.join(", "));
        mu.bindValue(":oid", woid);
        mu.exec();
    }

    QMessageBox::information(this, "出库成功",
        QString("备件「%1」x %2 已出库\n工单: %3\n领取人: %4")
        .arg(partName).arg(qty).arg(orderNo).arg(recipient));
    onPartsSearch();
}

// ============================================================
// Tab 1: 材料结算/提单 (Stage 3) — 实例状态流转到已安装并绑定车辆
// ============================================================

// 按「锁定工单 + 工单状态」控制 确认提单/取消提单 按钮显示
//   确认提单: 仅锁定工单且状态为「待提单」时显示
//   取消提单: 仅锁定工单且状态为「已提单」(等待结算) 时显示
void WarehousePage::updateBillButtons(int workorderId, const QString &status)
{
    const bool locked = (workorderId > 0);
    m_btnConfirmBill->setVisible(locked && status == "待提单");
    m_btnCancelBill->setVisible(locked && status == "已提单");
}

void WarehousePage::onBillingSearchOrder()
{
    QString orderNo = m_billingOrderNo->text().trimmed();
    if (orderNo.isEmpty()) return;

    RemoteQuery q;
    q.prepare("SELECT w.id, w.order_no, w.status, v.plate_number, "
              "COALESCE(w.material_fee,0) "
              "FROM t_workorder w LEFT JOIN t_vehicle v ON v.id=w.vehicle_id "
              "WHERE " + SqlUtil::likeCond("w.order_no", ":no")
              + " ORDER BY w.id DESC LIMIT 1");
    q.bindValue(":no", SqlUtil::likePattern(orderNo));
    q.exec();

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

    // 加载该工单的备件使用明细（合并显示）；单价列可编辑（待提单=确认提单前）
    // 成本列：按绑定实例的进货价计算（t_part_instance.unit_purchase_price，
    //   无实例进货价时回退到 t_parts.purchase_price），代表该备件的实际进货成本
    RemoteQuery q2;
    q2.prepare("SELECT wi.part_name AS '备件名称', COUNT(*) AS '数量', "
               "wi.unit_price AS '单价', SUM(wi.subtotal) AS '小计', "
               "SUM(COALESCE(pi.unit_purchase_price, p.purchase_price, 0) * wi.quantity) AS '成本' "
               "FROM t_workorder_item wi "
               "LEFT JOIN t_part_instance pi ON pi.id = wi.part_instance_id "
               "LEFT JOIN t_parts p ON p.id = wi.part_id "
               "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
               "GROUP BY wi.part_name, wi.unit_price");
    q2.bindValue(":oid", m_billingOrderId);
    q2.exec();

    m_billingTable->setRowCount(0);
    const bool canEditPrice = (status == "待提单");   // 仅待提单（确认提单前）可改单价
    int r = 0;
    while (q2.next()) {
        m_billingTable->insertRow(r);
        QString name  = q2.value(0).toString();
        int     qty   = q2.value(1).toInt();
        double  price = q2.value(2).toDouble();
        double  sub   = q2.value(3).toDouble();
        double  cost  = q2.value(4).toDouble();

        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setData(Qt::UserRole, name);   // 存备件名称，保存时据此定位
        m_billingTable->setItem(r, 0, nameItem);

        QTableWidgetItem *qtyItem = new QTableWidgetItem(QString::number(qty));
        qtyItem->setFlags(qtyItem->flags() & ~Qt::ItemIsEditable);
        m_billingTable->setItem(r, 1, qtyItem);

        QTableWidgetItem *priceItem = new QTableWidgetItem(QString("¥%1").arg(price, 0, 'f', 2));
        priceItem->setData(Qt::UserRole + 1, price);   // 原始单价
        if (!canEditPrice) priceItem->setFlags(priceItem->flags() & ~Qt::ItemIsEditable);
        m_billingTable->setItem(r, 2, priceItem);

        QTableWidgetItem *subItem = new QTableWidgetItem(QString("¥%1").arg(sub, 0, 'f', 2));
        subItem->setFlags(subItem->flags() & ~Qt::ItemIsEditable);
        m_billingTable->setItem(r, 3, subItem);

        QTableWidgetItem *costItem = new QTableWidgetItem(QString("¥%1").arg(cost, 0, 'f', 2));
        costItem->setFlags(costItem->flags() & ~Qt::ItemIsEditable);
        m_billingTable->setItem(r, 4, costItem);
        r++;
    }

    // 汇总材料费与总成本
    RemoteQuery q3;
    q3.prepare("SELECT COALESCE(SUM(wi.subtotal),0), "
               "COALESCE(SUM(COALESCE(pi.unit_purchase_price, p.purchase_price, 0) * wi.quantity),0) "
               "FROM t_workorder_item wi "
               "LEFT JOIN t_part_instance pi ON pi.id = wi.part_instance_id "
               "LEFT JOIN t_parts p ON p.id = wi.part_id "
               "WHERE wi.workorder_id = :oid AND wi.item_type = '材料'");
    q3.bindValue(":oid", m_billingOrderId);
    q3.exec();
    double matTotal = 0, costTotal = 0;
    if (q3.next()) {
        matTotal  = q3.value(0).toDouble();
        costTotal = q3.value(1).toDouble();
    }
    m_lblBillingTotal->setText(QString("材料费合计: ¥%1 | 总成本: ¥%2")
                               .arg(matTotal, 0, 'f', 2).arg(costTotal, 0, 'f', 2));

    // 确认提单/取消提单按钮：仅锁定工单且状态匹配时显示
    updateBillButtons(m_billingOrderId, status);

    if (status == "已派工") {
        QMessageBox::information(this, "提示", "该工单尚未通知提单，请先由前台通知提单后再操作");
    } else if (status == "已结算") {
        QMessageBox::information(this, "提示", "该工单已结算，无法操作");
    } else if (status != "待提单" && status != "已提单") {
        QMessageBox::warning(this, "状态错误",
            QString("当前状态为「%1」，需要「待提单」或「已提单」才能操作").arg(status));
    }
}

// ============================================================
// 保存材料单价编辑：
//   把表格中修改过的"单价"按 (工单 + 备件名称 + 原始单价) 定位写回
//   t_workorder_item.unit_price，再按明细重算材料费写回
//   t_workorder.material_fee。数量与材料费总额不可改。
// ============================================================
void WarehousePage::saveBillingPriceEdits()
{
    if (m_billingOrderId == 0) return;

    bool changed = false;
    for (int r = 0; r < m_billingTable->rowCount(); r++) {
        QTableWidgetItem *nameItem  = m_billingTable->item(r, 0);
        QTableWidgetItem *priceItem = m_billingTable->item(r, 2);
        if (!nameItem || !priceItem) continue;
        const QString partName = nameItem->data(Qt::UserRole).toString();
        if (partName.isEmpty()) continue;
        const double origPrice = priceItem->data(Qt::UserRole + 1).toDouble();
        const double newPrice  = priceItem->text().remove("¥").toDouble();
        if (qAbs(origPrice - newPrice) < 0.005) continue;   // 单价未修改

        RemoteQuery q;
        q.prepare("UPDATE t_workorder_item SET unit_price = :p "
                  "WHERE workorder_id = :oid AND part_name = :n "
                  "AND item_type = '材料' AND unit_price = :old");
        q.bindValue(":p", newPrice);
        q.bindValue(":oid", m_billingOrderId);
        q.bindValue(":n", partName);
        q.bindValue(":old", origPrice);
        q.exec();
        changed = true;

        // ---- 结算联动：该工单绑定的备件售价自动更新为编辑后的单价 ----
        // 1) 定位该备件的目录ID（按工单+备件名，取任一明细行的 part_id）
        q.prepare("SELECT part_id FROM t_workorder_item "
                  "WHERE workorder_id = :oid AND part_name = :n "
                  "AND item_type = '材料' LIMIT 1");
        q.bindValue(":oid", m_billingOrderId);
        q.bindValue(":n", partName);
        q.exec();
        if (!q.next()) continue;
        const int partId = q.value(0).toInt();

        // 2) 绑定实例的售价同步（t_part_instance.unit_sale_price）
        q.prepare("UPDATE t_part_instance SET unit_sale_price = :p, updated_at = NOW() "
                  "WHERE workorder_id = :oid AND part_id = :pid "
                  "AND status IN ('已领出','已安装')");
        q.bindValue(":p", newPrice);
        q.bindValue(":oid", m_billingOrderId);
        q.bindValue(":pid", partId);
        q.exec();

        // 3) 出库流水售价同步（出库报表读取 t_inventory_log.unit_price / total_price）
        const QString orderNo = m_billingOrderNo->text().trimmed();
        if (!orderNo.isEmpty()) {
            q.prepare("UPDATE t_inventory_log SET unit_price = :p, total_price = :t "
                      "WHERE part_id = :pid AND ref_order_no = :ono "
                      "AND operation_type = '维修出库'");
            q.bindValue(":p", newPrice);
            q.bindValue(":t", -newPrice);   // 出库流水 total = -单价（数量为 -1）
            q.bindValue(":pid", partId);
            q.bindValue(":ono", orderNo);
            q.exec();
        }
    }
    if (!changed) return;

    // 重算材料费并写回 t_workorder.material_fee
    RemoteQuery q;
    q.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
              "WHERE workorder_id = :oid AND item_type = '材料'");
    q.bindValue(":oid", m_billingOrderId);
    q.exec();
    double mat = q.next() ? q.value(0).toDouble() : 0;
    q.prepare("UPDATE t_workorder SET material_fee = :mat WHERE id = :id");
    q.bindValue(":mat", mat);
    q.bindValue(":id", m_billingOrderId);
    q.exec();
}

void WarehousePage::onCompareAndBill()
{
    if (m_billingOrderId == 0) return;

    // 检查当前工单状态
    {
        RemoteQuery cq;
        cq.prepare("SELECT status FROM t_workorder WHERE id = :id");
        cq.bindValue(":id", m_billingOrderId);
        cq.exec();
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

    // 直接把材料单价编辑保存到数据库，再按新单价重算材料费
    saveBillingPriceEdits();

    // 获取材料费总额及工单车辆信息
    RemoteQuery q;
    q.prepare("SELECT COALESCE(SUM(subtotal),0) FROM t_workorder_item "
              "WHERE workorder_id = :oid AND item_type = '材料'");
    q.bindValue(":oid", m_billingOrderId);
    q.exec();
    double matTotal = q.next() ? q.value(0).toDouble() : 0;

    q.prepare("SELECT vehicle_id FROM t_workorder WHERE id = :id");
    q.bindValue(":id", m_billingOrderId);
    q.exec();
    int vehicleId = q.next() ? q.value(0).toInt() : -1;

    // 先取已领出实例（事务外查询，事务内使用）
    RemoteQuery instQ;
    instQ.prepare("SELECT id, part_id FROM t_part_instance "
                  "WHERE workorder_id = :wid AND status = '已领出'");
    instQ.bindValue(":wid", m_billingOrderId);
    instQ.exec();
    QList<QPair<int,int>> instances; // (instance_id, part_id)
    while (instQ.next())
        instances << QPair<int,int>(instQ.value(0).toInt(), instQ.value(1).toInt());

    // 提单操作在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    steps.append(RemoteDb::step(
        "UPDATE t_workorder SET status = '已提单', material_fee = :mat "
        "WHERE id = :id AND status IN ('已派工','待提单')",
        QJsonObject{ { ":mat", matTotal }, { ":id", m_billingOrderId } }));

    for (const auto &pair : instances) {
        // 已领出实例 → 已安装 并绑定车辆
        steps.append(RemoteDb::step(
            "UPDATE t_part_instance SET status = '已安装', vehicle_id = :vid, "
            "updated_at = NOW() WHERE id = :iid",
            QJsonObject{
                { ":vid", vehicleId > 0 ? QJsonValue(vehicleId) : QJsonValue(QJsonValue::Null) },
                { ":iid", pair.first },
            }));
        // 记录材料结算流水
        steps.append(RemoteDb::step(
            "INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, "
            "operation_type, ref_order_no, operator_id, remark) "
            "VALUES (:pid, :iid, 1, '材料结算', :ref, :op, '材料审核提单')",
            QJsonObject{
                { ":pid", pair.second }, { ":iid", pair.first },
                { ":ref", m_billingOrderNo->text().trimmed() },
                { ":op", Session::instance().userId() },
            }));
    }
    // 更新库存缓存
    for (const auto &pair : instances) {
        steps.append(RemoteDb::step(
            "UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
            "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2",
            QJsonObject{ { ":pid", pair.second }, { ":pid2", pair.second } }));
    }
    // 记录交易历史
    if (vehicleId > 0) {
        steps.append(RemoteDb::step(
            "INSERT INTO t_vehicle_transaction (vehicle_id, workorder_id, "
            "transaction_type, description, operator_id) "
            "VALUES (:vid, :woid, '提单', :desc, :op)",
            QJsonObject{
                { ":vid", vehicleId }, { ":woid", m_billingOrderId },
                { ":desc", QString("材料审核提单完成，材料费合计 ¥%1").arg(matTotal, 0, 'f', 2) },
                { ":op", Session::instance().userId() },
            }));
    }

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "提单失败", txn.value("error").toString());
        return;
    }

    // 同步更新维修历史：状态 + 材料费 + 备件摘要
    {
        QStringList psList;
        RemoteQuery psq;
        psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                    "WHERE workorder_id = :oid AND item_type = '材料' "
                    "GROUP BY part_name");
        psq.bindValue(":oid", m_billingOrderId);
        psq.exec();
        while (psq.next())
            psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
        QString ps = psList.join(", ");

        RemoteQuery mu;
        mu.prepare("UPDATE t_maintenance_history SET status='已提单', material_fee=:mat, "
                   "parts_summary=:ps WHERE workorder_id=:oid");
        mu.bindValue(":mat", matTotal);
        mu.bindValue(":ps", ps.isEmpty() ? QString() : ps);
        mu.bindValue(":oid", m_billingOrderId);
        mu.exec();
    }

    QMessageBox::information(this, "提单成功",
        QString("工单材料审核已通过，已设置为「已提单」状态\n"
                "备件已绑定到车辆，材料费合计: ¥%1\n前台可进行结算操作")
        .arg(matTotal, 0, 'f', 2));
    updateBillButtons(m_billingOrderId, "已提单");
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
    RemoteQuery cq;
    cq.prepare("SELECT status FROM t_workorder WHERE id = :id");
    cq.bindValue(":id", m_billingOrderId);
    cq.exec();
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

    // 取消提单在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    steps.append(RemoteDb::step(
        "UPDATE t_workorder SET status = '待提单' WHERE id = :id AND status = '已提单'",
        QJsonObject{ { ":id", m_billingOrderId } }));
    // 将该工单关联的已安装实例恢复为已领出
    steps.append(RemoteDb::step(
        "UPDATE t_part_instance SET status = '已领出', vehicle_id = NULL, "
        "updated_at = NOW() WHERE workorder_id = :wid AND status = '已安装'",
        QJsonObject{ { ":wid", m_billingOrderId } }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "取消失败", txn.value("error").toString());
        return;
    }

    // 同步更新维修历史状态
    {
        RemoteQuery mu;
        mu.prepare("UPDATE t_maintenance_history SET status='待提单' WHERE workorder_id=:oid");
        mu.bindValue(":oid", m_billingOrderId);
        mu.exec();
    }

    updateBillButtons(m_billingOrderId, "待提单");
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

// 模糊搜索已有备件并刷新下拉（逐键触发；仅在用户主动选择时回填输入区）
void WarehousePage::onPurchaseSearch()
{
    QString keyword = m_purPartSearch->text().trimmed();
    if (keyword.isEmpty()) {
        m_purRows.clear();
        m_purCompleter->hideDropdown();
        return;
    }

    RemoteQuery q;
    q.prepare("SELECT p.id, p.part_no, p.name, COALESCE(NULLIF(p.spec,''),''), "
              "COALESCE(p.supplier,''), COALESCE(p.applicable_model,'') "
              "FROM t_parts p WHERE " + SqlUtil::likeConds({"p.part_no", "p.name"}, ":kw") + " "
              "ORDER BY p.name LIMIT 30");
    q.bindValue(":kw", SqlUtil::likePattern(keyword));
    q.exec();

    m_purRows.clear();
    while (q.next())
        m_purRows << QStringList{q.value(1).toString(), q.value(2).toString(),
                                 q.value(3).toString(), q.value(4).toString(), q.value(5).toString()};

    if (m_purRows.isEmpty()) {
        m_purCompleter->hideDropdown();
        return;                       // 无匹配 → 收起下拉，不弹"未找到"（可手动填写新备件）
    }

    // 无论 1 条还是多条，都在输入框下方自动展开下拉，仅在用户主动选择时回填
    QList<QVariant> ids;
    for (int i = 0; i < m_purRows.size(); ++i)
        ids << i;                     // 用行号作为 id，选中后按 m_purRows 回填
    m_purCompleter->setResults(m_purRows, ids);
    m_purCompleter->showDropdown();
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
    if (m_purSupplier->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "供应商为必填项");
        return;
    }

    PurchaseItem it;
    it.partNo = partNo;
    it.partName = partName;
    it.spec = m_purSpec->text().trimmed();
    it.supplier = m_purSupplier->text().trimmed();
    it.applicableModel = m_purApplicableModel->text().trimmed();
    it.cost = m_purCost->value();
    it.price = m_purPrice->value();
    it.qty = m_purQty->value();

    m_purchaseList << it;
    refreshPurchaseList();

    // 清空输入区，方便连续录入
    m_purPartNo->clear(); m_purPartName->clear(); m_purSpec->clear();
    m_purSupplier->clear(); m_purApplicableModel->clear();
    m_purCost->setValue(0); m_purPrice->setValue(0);
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
            << new QStandardItem(it.applicableModel.isEmpty() ? "-" : it.applicableModel)
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

    // 按 备件编号/名称/型号/供应商 分组合并同种备件：
    // 1) 让同一批的同种备件流水连续，报表按相邻合并才能生效；
    // 2) 避免同一备件录入多次导致重复建档。
    QList<PurchaseItem> grouped;
    for (const PurchaseItem &it : m_purchaseList) {
        bool found = false;
        for (PurchaseItem &g : grouped) {
            if (g.partNo == it.partNo && g.partName == it.partName
                && g.spec == it.spec && g.supplier == it.supplier
                && g.applicableModel == it.applicableModel) {
                g.qty += it.qty;
                found = true;
                break;
            }
        }
        if (!found)
            grouped.append(it);
    }

    // 确认弹窗
    QDialog dlg(this);
    dlg.setWindowTitle("确认本批入库");
    dlg.resize(680, 440);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);

    QLabel *tip = new QLabel(QString("以下 %1 种备件将全部入库，请核对无误：").arg(grouped.size()));
    tip->setStyleSheet("font-weight:bold;");
    dl->addWidget(tip);

    QTableWidget *tbl = new QTableWidget;
    tbl->setColumnCount(8);
    tbl->setHorizontalHeaderLabels({"备件编号", "备件名称", "规格型号", "适用车型", "数量", "进货价", "小计", "供应商"});
    tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
    tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tbl->verticalHeader()->setVisible(false);
    tbl->horizontalHeader()->setStretchLastSection(true);
    tbl->setAlternatingRowColors(true);
    tbl->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");

    tbl->setRowCount(grouped.size());
    double total = 0;
    for (int i = 0; i < grouped.size(); i++) {
        const PurchaseItem &it = grouped[i];
        double sub = it.cost * it.qty;
        total += sub;
        tbl->setItem(i, 0, new QTableWidgetItem(it.partNo));
        tbl->setItem(i, 1, new QTableWidgetItem(it.partName));
        tbl->setItem(i, 2, new QTableWidgetItem(it.spec));
        tbl->setItem(i, 3, new QTableWidgetItem(it.applicableModel.isEmpty() ? "-" : it.applicableModel));
        tbl->setItem(i, 4, new QTableWidgetItem(QString::number(it.qty)));
        tbl->setItem(i, 5, new QTableWidgetItem(QString("¥%1").arg(it.cost, 0, 'f', 2)));
        tbl->setItem(i, 6, new QTableWidgetItem(QString("¥%1").arg(sub, 0, 'f', 2)));
        tbl->setItem(i, 7, new QTableWidgetItem(it.supplier));
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

    // 批量执行入库（单事务，任一失败整体回滚；经 4s-server 事务命令）
    QJsonArray steps;
    int idx = 0;
    bool allOk = true;
    for (const PurchaseItem &it : grouped) {
        if (!buildPurchaseInboundSteps(it, steps, idx)) { allOk = false; break; }
        idx++;
    }
    QJsonObject txn = allOk ? RemoteDb::transaction(steps) : QJsonObject{ { "ok", false } };
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "入库失败",
            "本批入库已整体回滚：" + txn.value("error").toString());
        return;
    }

    QMessageBox::information(this, "入库成功",
        QString("本批共 %1 种备件已全部入库").arg(grouped.size()));
    m_purchaseList.clear();
    refreshPurchaseList();
}

// 构建单条备件入库的事务步骤（查询在事务外做，写入由 4s-server 事务命令原子执行）
bool WarehousePage::buildPurchaseInboundSteps(const PurchaseItem &it, QJsonArray &steps, int idx)
{
    RemoteQuery q;

    // 检查是否已有该备件编号（事务外查询）
    int catalogId = 0;
    q.prepare("SELECT id FROM t_parts WHERE part_no = :no");
    q.bindValue(":no", it.partNo);
    q.exec();

    QString catalogRef;
    if (q.next()) {
        catalogId = q.value(0).toInt();
        catalogRef = QString::number(catalogId);
        // 已有备件 → 更新目录信息
        steps.append(RemoteDb::step(
            "UPDATE t_parts SET name = :name, spec = :spec, supplier = :sup, "
            "applicable_model = COALESCE(NULLIF(:am, ''), applicable_model), "
            "purchase_price = COALESCE(:cost, purchase_price), "
            "sale_price = COALESCE(:price, sale_price) WHERE id = :id",
            QJsonObject{
                { ":name", it.partName },
                { ":spec", RemoteDb::v(it.spec.isEmpty() ? QString() : it.spec) },
                { ":sup", RemoteDb::v(it.supplier.isEmpty() ? QString() : it.supplier) },
                { ":am", RemoteDb::v(it.applicableModel.isEmpty() ? QString() : it.applicableModel) },
                { ":cost", it.cost > 0 ? QJsonValue(it.cost) : QJsonValue(QJsonValue::Null) },
                { ":price", it.price > 0 ? QJsonValue(it.price) : QJsonValue(QJsonValue::Null) },
                { ":id", catalogId },
            }));
    } else {
        // 新建备件目录（捕获新 id，供后续步骤引用）
        steps.append(RemoteDb::step(
            "INSERT INTO t_parts (part_no, name, spec, stock, purchase_price, sale_price, "
            "supplier, applicable_model) "
            "VALUES (:no, :name, :spec, 0, :cost, :price, :sup, :am)",
            QJsonObject{
                { ":no", it.partNo }, { ":name", it.partName },
                { ":spec", RemoteDb::v(it.spec.isEmpty() ? QString() : it.spec) },
                { ":cost", it.cost > 0 ? QJsonValue(it.cost) : QJsonValue(QJsonValue::Null) },
                { ":price", it.price > 0 ? QJsonValue(it.price) : QJsonValue(QJsonValue::Null) },
                { ":sup", RemoteDb::v(it.supplier.isEmpty() ? QString() : it.supplier) },
                { ":am", RemoteDb::v(it.applicableModel.isEmpty() ? QString() : it.applicableModel) },
            }, QString("catalog%1").arg(idx)));
        catalogRef = QString("@catalog%1").arg(idx);
    }

    // 实例编号基数（已有备件按当前实例数续号；新备件从 1 起）
    int base = 0;
    if (catalogId > 0) {
        RemoteQuery cq;
        cq.prepare("SELECT COUNT(*) FROM t_part_instance WHERE part_id = :pid");
        cq.bindValue(":pid", catalogId);
        cq.exec();
        if (cq.next()) base = cq.value(0).toInt();
    }

    // 创建 qty 个实例（序号在本批内递增，避免同一批重复 SN）
    for (int i = 0; i < it.qty; i++) {
        const QString sn = QString("%1-%2").arg(it.partNo).arg(base + i + 1, 4, 10, QChar('0'));
        const QString cap = QString("inst%1_%2").arg(idx).arg(i);
        steps.append(RemoteDb::step(
            "INSERT INTO t_part_instance (part_id, instance_sn, status, "
            "unit_purchase_price, unit_sale_price, remark) "
            "VALUES (:pid, :sn, '在库', :cost, :price, :rmk)",
            QJsonObject{
                { ":pid", catalogRef }, { ":sn", sn },
                { ":cost", it.cost > 0 ? QJsonValue(it.cost) : QJsonValue(QJsonValue::Null) },
                { ":price", it.price > 0 ? QJsonValue(it.price) : QJsonValue(QJsonValue::Null) },
                { ":rmk", RemoteDb::v(it.supplier.isEmpty() ? QString() : it.supplier) },
            }, cap));

        // 记录流水（引用本实例的 lastInsertId）
        steps.append(RemoteDb::step(
            "INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
            "operation_type, operator_id) "
            "VALUES (:pid, :iid, 1, :price, :total, '采购入库', :op)",
            QJsonObject{
                { ":pid", catalogRef }, { ":iid", "@" + cap },
                { ":price", it.cost }, { ":total", it.cost },
                { ":op", Session::instance().userId() },
            }));
    }

    // 记录采购批次
    steps.append(RemoteDb::step(
        "INSERT INTO t_part_purchase (part_id, supplier, quantity, unit_cost, total_cost, operator_id) "
        "VALUES (:pid, :sup, :qty, :cost, :total, :op)",
        QJsonObject{
            { ":pid", catalogRef },
            { ":sup", RemoteDb::v(it.supplier.isEmpty() ? QString() : it.supplier) },
            { ":qty", it.qty }, { ":cost", it.cost }, { ":total", it.qty * it.cost },
            { ":op", Session::instance().userId() },
        }));

    // 更新库存缓存
    steps.append(RemoteDb::step(
        "UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
        "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2",
        QJsonObject{ { ":pid", catalogRef }, { ":pid2", catalogRef } }));

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
    if (!keyword.isEmpty())
        where = SqlUtil::likeConds({"p.part_no", "p.name", "p.spec", "p.supplier"}, ":kw");

    QString sql = mergedSelectSQL(extraCols, where,
        "p.purchase_price, p.sale_price", "ORDER BY p.name LIMIT 500");
    RemoteQuery q;
    q.prepare(sql);
    if (!keyword.isEmpty())
        q.bindValue(":kw", SqlUtil::likePattern(keyword));
    q.exec();
    m_stockModel->setQuery(q);
}

// ============================================================
// Tab 4: 备件退库 — 将已领出/已安装实例退回在库
// ============================================================

void WarehousePage::onReturnSearch()
{
    // 未锁定工单：清空列表并提示，不显示任何备件（退库列表只显示锁定工单绑定的备件）
    if (m_retLockedWorkOrderId <= 0) {
        m_retModel->clear();
        m_retPartId = 0;
        return;
    }

    QString keyword = m_retPartSearch->text().trimmed();
    RemoteQuery q;

    // 构建 WHERE 条件：仅锁定工单绑定的已领出实例（必选），再叠加模糊关键字
    QStringList conditions;
    conditions << QString("i.workorder_id = %1").arg(m_retLockedWorkOrderId);
    if (!keyword.isEmpty()) {
        conditions << SqlUtil::likeConds({"p.part_no", "p.name", "p.supplier", "p.spec"}, ":kw");
    }

    QString whereClause = "WHERE " + conditions.join(" AND ");

    // 退库仅针对「已派工」工单：只显示绑定在已派工农单上的已领出实例，
    // 待提单/已提单/已结算工单的备件已进入流程，不允许退回。
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COALESCE(p.applicable_model,'') AS '适用车型', "
        "COUNT(i.id) AS '可退数量', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "INNER JOIN t_part_instance i ON i.part_id = p.id AND i.status = '已领出' "
        "AND i.workorder_id IS NOT NULL "
        "INNER JOIN t_workorder w ON w.id = i.workorder_id AND w.status = '已派工' "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.applicable_model, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(whereClause);

    q.prepare(sql);
    if (!keyword.isEmpty())
        q.bindValue(":kw", SqlUtil::likePattern(keyword));
    q.exec();
    m_retModel->setQuery(q);
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

    // 获取可退库的实例：仅已领出、且绑定在「已派工」工单上的实例
    QList<int> instanceIds;
    QList<int> affectedWorkOrders;   // 退库涉及的工单ID（用于重算备件摘要）
    RemoteQuery q;

    if (m_retLockedWorkOrderId > 0) {
        // 已锁定工单：精确匹配该工单的已领出实例（不用模糊 LIKE，避免退错相似单号的工单）
        q.prepare("SELECT id, workorder_id FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '已领出' "
                  "AND workorder_id = :wid LIMIT :lim");
        q.bindValue(":wid", m_retLockedWorkOrderId);
    } else {
        // 未锁定工单：仅退「已派工」工单绑定的已领出实例
        q.prepare("SELECT i.id, i.workorder_id FROM t_part_instance i "
                  "JOIN t_workorder w ON w.id = i.workorder_id AND w.status = '已派工' "
                  "WHERE i.part_id = :pid AND i.status = '已领出' "
                  "LIMIT :lim");
    }
    q.bindValue(":pid", m_retPartId);
    q.bindValue(":lim", qty);
    q.exec();
    while (q.next()) {
        instanceIds << q.value(0).toInt();
        int woid = q.value(1).toInt();
        if (woid > 0 && !affectedWorkOrders.contains(woid))
            affectedWorkOrders << woid;
    }

    if (instanceIds.size() < qty) {
        QMessageBox::warning(this, "退库失败",
            QString("可退库的备件仅 %1 件，退库数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取备件信息
    q.prepare("SELECT name, COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_retPartId);
    q.exec();
    QString partName = q.next() ? q.value(0).toString() : "未知";
    double costPrice = q.value(1).toDouble();

    // 退库操作在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    for (int instId : instanceIds) {
        // 更新实例状态为在库，清除绑定
        steps.append(RemoteDb::step(
            "UPDATE t_part_instance SET status = '在库', vehicle_id = NULL, "
            "workorder_id = NULL, recipient = NULL, updated_at = NOW() "
            "WHERE id = :iid",
            QJsonObject{ { ":iid", instId } }));

        // 记录流水
        steps.append(RemoteDb::step(
            "INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
            "operation_type, ref_order_no, operator_id, remark) "
            "VALUES (:pid, :iid, 1, :price, :total, '备件退库', :ref, :op, '备件退库')",
            QJsonObject{
                { ":pid", m_retPartId }, { ":iid", instId },
                { ":price", costPrice }, { ":total", costPrice },
                { ":ref", RemoteDb::v(orderNo.isEmpty() ? QString() : orderNo) },
                { ":op", Session::instance().userId() },
            }));
    }

    // 清除工单备件明细中已退库的实例行：否则材料费/备件摘要仍会把已退库备件计入统计
    if (!instanceIds.isEmpty()) {
        QStringList phs;
        QJsonObject delParams;
        for (int i = 0; i < instanceIds.size(); ++i) {
            phs << QString(":iid%1").arg(i);
            delParams[QString(":iid%1").arg(i)] = instanceIds[i];
        }
        steps.append(RemoteDb::step(
            "DELETE FROM t_workorder_item WHERE part_instance_id IN (" + phs.join(",") + ")",
            delParams));
    }

    // 更新库存缓存
    steps.append(RemoteDb::step(
        "UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
        "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2",
        QJsonObject{ { ":pid", m_retPartId }, { ":pid2", m_retPartId } }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "退库失败", txn.value("error").toString());
        return;
    }

    // 同步更新维修历史：备件摘要（对退库涉及的所有工单重算，剔除已退库备件）
    for (int woid : affectedWorkOrders) {
        QStringList psList;
        RemoteQuery psq;
        psq.prepare("SELECT part_name, COUNT(*) FROM t_workorder_item "
                    "WHERE workorder_id=:oid AND item_type='材料' GROUP BY part_name");
        psq.bindValue(":oid", woid);
        psq.exec();
        while (psq.next())
            psList << QString("%1x%2").arg(psq.value(0).toString()).arg(psq.value(1).toInt());
        RemoteQuery mu;
        mu.prepare("UPDATE t_maintenance_history SET parts_summary=:ps WHERE workorder_id=:oid");
        mu.bindValue(":ps", psList.isEmpty() ? QString() : psList.join(", "));
        mu.bindValue(":oid", woid);
        mu.exec();
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
    RemoteQuery q;

    // 与备件领取 onPartsSearch 同风格，但仅 JOIN 在库实例
    QString likeWhere;
    if (!keyword.isEmpty())
        likeWhere = "WHERE " + SqlUtil::likeConds(
            {"p.part_no", "p.name", "p.supplier", "p.spec"}, ":kw");
    QString sql = QString(
        "SELECT p.id AS catalog_id, p.part_no AS '备件编号', p.name AS '备件名称', "
        "COALESCE(NULLIF(p.spec,''), CONCAT('(无型号-', p.part_no, ')')) AS '规格型号', "
        "COALESCE(p.supplier,'') AS '供应商', "
        "COALESCE(p.applicable_model,'') AS '适用车型', "
        "COUNT(i.id) AS '在库数量', "
        "COALESCE(p.purchase_price, 0) AS '进货价', "
        "COALESCE(p.sale_price, (SELECT unit_sale_price FROM t_part_instance "
        " WHERE part_id=p.id AND unit_sale_price IS NOT NULL LIMIT 1)) AS '销售价' "
        "FROM t_parts p "
        "INNER JOIN t_part_instance i ON i.part_id = p.id AND i.status = '在库' "
        "%1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier, p.applicable_model, p.purchase_price, p.sale_price "
        "ORDER BY p.name LIMIT 200")
        .arg(likeWhere);

    q.prepare(sql);
    if (!keyword.isEmpty())
        q.bindValue(":kw", SqlUtil::likePattern(keyword));
    q.exec();
    m_purRetModel->setQuery(q);
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
    RemoteQuery q;
    q.prepare("SELECT name, COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    q.bindValue(":id", m_purRetPartId);
    q.exec();
    QString partName = q.next() ? q.value(0).toString() : "未知";
    double costPrice = q.value(1).toDouble();

    // 采购退货在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    for (int instId : instanceIds) {
        // 更新实例状态为已退货
        steps.append(RemoteDb::step(
            "UPDATE t_part_instance SET status = '已退货', "
            "workorder_id = NULL, vehicle_id = NULL, recipient = NULL, "
            "updated_at = NOW() WHERE id = :iid",
            QJsonObject{ { ":iid", instId } }));

        // 记录流水
        steps.append(RemoteDb::step(
            "INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, unit_price, total_price, "
            "operation_type, operator_id, remark) "
            "VALUES (:pid, :iid, -1, :price, :total, '采购退货', :op, '采购退货')",
            QJsonObject{
                { ":pid", m_purRetPartId }, { ":iid", instId },
                { ":price", costPrice }, { ":total", -costPrice },
                { ":op", Session::instance().userId() },
            }));
    }

    // 更新库存缓存
    steps.append(RemoteDb::step(
        "UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
        "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2",
        QJsonObject{ { ":pid", m_purRetPartId }, { ":pid2", m_purRetPartId } }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "退货失败", txn.value("error").toString());
        return;
    }
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
            m_billingTable->setRowCount(0);
            m_lblBillingInfo->setText("请搜索工单");
            m_lblBillingTotal->setText("材料费合计: ¥0.00 | 总成本: ¥0.00");
            updateBillButtons(0, QString());   // 未锁定工单 → 隐藏两个提单按钮
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

// ============================================================
// 键盘导航实现
// ============================================================

void WarehousePage::installNavFilter(QWidget *w)
{
    w->installEventFilter(this);
    // QSpinBox 内部有一个行编辑：按键实际发给它，需一并挂过滤
    if (auto *sb = qobject_cast<QAbstractSpinBox*>(w)) {
        if (QLineEdit *le = sb->findChild<QLineEdit*>())
            le->installEventFilter(this);
    }
}

QList<QWidget*> WarehousePage::chainForTab(int index) const
{
    switch (index) {
    case 0:  // 备件领取
        return { m_issueOrderNo, m_issueRecipient, m_issuePartSearch,
                 m_issueTable, m_spinIssueQty, m_btnIssue };
    case 1:  // 材料结算
        return { m_billingOrderNo, m_billingTable, m_btnConfirmBill, m_btnCancelBill };
    case 2:  // 采购入库（输入区：搜索→填写→加入清单→确认入库）
        return { m_purPartSearch, m_purPartNo, m_purPartName, m_purSpec,
                 m_purApplicableModel, m_purSupplier, m_purCost, m_purPrice, m_purQty,
                 m_btnPurAddItem, m_btnPurConfirm };
    case 3:  // 库存查询
        return { m_stockKeyword, m_stockTable };
    case 4:  // 备件退库
        return { m_retOrderNo, m_retPartSearch, m_retTable, m_retQty, m_btnRetConfirm };
    case 5:  // 采购退货
        return { m_purRetPartSearch, m_purRetTable, m_purRetQty, m_btnPurRetConfirm };
    default:
        return {};
    }
}

QWidget *WarehousePage::navWidgetOf(QObject *obj) const
{
    auto *w = qobject_cast<QWidget*>(obj);
    if (!w)
        return nullptr;
    // QSpinBox 内部行编辑收到按键 → 归一到自旋框本体（输入链中存的是自旋框）
    if (auto *sb = qobject_cast<QAbstractSpinBox*>(w->parentWidget()))
        return sb;
    return w;
}

bool WarehousePage::eventFilter(QObject *obj, QEvent *event)
{
    // ==================== Tab 栏：左右切换 / 回车进入 / 鼠标点击进入 ====================
    if (obj == m_tabWidget->tabBar()) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            const int key = ke->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Tab) {
                enterTab();        // 回车/Tab 进入当前 tab 的第一个输入区
                return true;
            }
            return false;          // 左右方向键 → 原生切换 tab（QTabBar 自带）
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            // 鼠标点击某个 tab → 切换后自动进入该 tab 内容
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (m_tabWidget->tabBar()->tabAt(me->pos()) >= 0)
                QTimer::singleShot(0, this, [this]() { enterTab(); });
        }
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        const int key = ke->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter)
            return handleEnter(obj);
        if (key == Qt::Key_Tab) {
            handleTab(obj);
            return true;
        }
        // 方向键/其它键：不拦截，保留原生行为（光标移动 / 数值调整 / 表格行移动）
        return false;
    }
    return QWidget::eventFilter(obj, event);
}

bool WarehousePage::handleEnter(QObject *obj)
{
    QWidget *w = navWidgetOf(obj);
    if (!w)
        return false;

    // 搜索框 + 结果列表配对（需求3）：有结果跳列表第一行，无结果不跳
    if (w == m_issuePartSearch || w == m_retPartSearch
        || w == m_purRetPartSearch || w == m_stockKeyword) {
        return searchToFirstRow(w);
    }
    // 结果列表：回车 = 确认选中备件 → 跳下一输入区
    if (w == m_issueTable || w == m_retTable || w == m_purRetTable) {
        return confirmRowAndNext(w);
    }
    // 只读展示表格（结算明细 / 本批清单 / 库存）：回车 → 下一输入区
    if (w == m_billingTable || w == m_purTable || w == m_stockTable) {
        navNext(w);
        return true;
    }
    // 按钮：回车 = 点击（确认出库/加入清单/确认入库/确认退库/确认退货/提单等）
    if (auto *btn = qobject_cast<QPushButton*>(w)) {
        btn->click();
        return true;
    }
    // 普通输入框（QLineEdit / QSpinBox）：回车 → 下一输入区
    navNext(w);
    return true;
}

bool WarehousePage::handleTab(QObject *obj)
{
    QWidget *w = navWidgetOf(obj);
    if (!w)
        return false;
    // 结果列表：Tab 与回车一致（确认选中备件再前进，避免未选中就离开导致选择丢失）
    if (w == m_issueTable || w == m_retTable || w == m_purRetTable) {
        return confirmRowAndNext(w);
    }
    navNext(w);
    return true;
}

bool WarehousePage::searchToFirstRow(QWidget *searchField)
{
    // 与 returnPressed 语义一致，先执行一次搜索（live 搜索一般已是最新）
    if (searchField == m_issuePartSearch)       onPartsSearch();
    else if (searchField == m_retPartSearch)    onReturnSearch();
    else if (searchField == m_purRetPartSearch) onPurchaseReturnSearch();
    else if (searchField == m_stockKeyword)     onStockSearch();

    QTableView *table = nullptr;
    if (searchField == m_issuePartSearch)       table = m_issueTable;
    else if (searchField == m_retPartSearch)    table = m_retTable;
    else if (searchField == m_purRetPartSearch) table = m_purRetTable;
    else if (searchField == m_stockKeyword)     table = m_stockTable;

    if (table && table->model() && table->model()->rowCount() > 0)
        focusListFirstRow(table);    // 有结果：跳列表第一行
    // 无结果：焦点留在搜索框，不跳转
    return true;
}

bool WarehousePage::confirmRowAndNext(QWidget *tableWidget)
{
    auto *t = qobject_cast<QTableView*>(tableWidget);
    if (!t)
        return true;
    if (!t->model() || t->model()->rowCount() <= 0)
        return true;                 // 空列表：回车无效，停留在列表
    QModelIndex cur = t->currentIndex();
    if (!cur.isValid()) {
        focusListFirstRow(t);        // 尚未选中行 → 先跳到第一行让用户用上下键选择
        return true;
    }
    // 复用 clicked 连接的选中逻辑（设置 partId / 数量上限 / 信息栏）
    emit t->clicked(cur);
    // 确认选中后跳下一输入区（数量）
    navNext(t);
    return true;
}

void WarehousePage::focusListFirstRow(QTableView *t)
{
    t->setFocus(Qt::OtherFocusReason);
    if (!t->model() || t->model()->rowCount() <= 0)
        return;
    QModelIndex first = t->model()->index(0, 0);
    t->setCurrentIndex(first);
    t->selectRow(0);
    t->scrollTo(first);
}

void WarehousePage::navNext(QWidget *w)
{
    const QList<QWidget*> chain = chainForTab(m_tabWidget->currentIndex());
    if (chain.isEmpty())
        return;
    int idx = chain.indexOf(w);
    if (idx < 0)
        idx = chain.size();          // 不在链中（理论上不会）→ 从第一个开始
    for (int i = 1; i <= chain.size(); ++i) {
        QWidget *n = chain.at((idx + i) % chain.size());
        if (n->isVisible()) {        // 跳过隐藏项（如未锁定工单时隐藏的提单按钮）
            focusWidget(n);
            return;
        }
    }
}

void WarehousePage::focusWidget(QWidget *w)
{
    if (!w)
        return;
    if (auto *e = qobject_cast<QLineEdit*>(w))
        e->selectAll();
    w->setFocus(Qt::OtherFocusReason);
}

void WarehousePage::enterTab()
{
    const QList<QWidget*> chain = chainForTab(m_tabWidget->currentIndex());
    for (QWidget *w : chain)
        if (w->isVisible()) {
            focusWidget(w);
            return;
        }
}

void WarehousePage::focusTabBar()
{
    m_tabWidget->tabBar()->setFocus(Qt::OtherFocusReason);
}

void WarehousePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 打开库房工作台：默认落在「备件领取」tab，焦点先放 tab 栏
    // （此时左右方向键可切换 tab，回车或鼠标点击进入该 tab 内容）
    QTimer::singleShot(0, this, [this]() {
        m_tabWidget->setCurrentIndex(0);
        focusTabBar();
    });
}

void WarehousePage::loadTechCombos() {}
void WarehousePage::loadPartCombos() {}
