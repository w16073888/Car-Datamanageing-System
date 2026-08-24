#include "SettlementEditPage.h"
#include "database/Session.h"
#include "core/AppSettings.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteDb.h"
#include "remote/SqlUtil.h"
#include "utils/PrintUtil.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QDate>
#include <QComboBox>
#include <QButtonGroup>
#include <QScrollArea>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QFileDialog>
#include <QSignalBlocker>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

// ============================================================
// 人民币大写转换 / 文本金额格式化 / 打印PDF管线统一走 utils/PrintUtil.h
// ============================================================

// ============================================================
// LaborTypeDelegate：类别下拉（机电/钣金/喷漆）
// ============================================================
LaborTypeDelegate::LaborTypeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *LaborTypeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    QComboBox *combo = new QComboBox(parent);
    combo->addItems({"机电", "钣金", "喷漆"});
    return combo;
}

void LaborTypeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    if (!combo) return;
    const int idx = combo->findText(index.data().toString());
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void LaborTypeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    QComboBox *combo = qobject_cast<QComboBox*>(editor);
    if (!combo) return;
    model->setData(index, combo->currentText());
}

// ============================================================
// MoneyDelegate：单元格存原始数值，显示时加 ¥
// ============================================================
MoneyDelegate::MoneyDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QString MoneyDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    bool ok = false;
    const double v = value.toDouble(&ok);
    if (ok) return QString("¥%1").arg(v, 0, 'f', 2);
    return QStyledItemDelegate::displayText(value, locale);
}

// ============================================================
// 结算修改页
// ============================================================
SettlementEditPage::SettlementEditPage(QWidget *parent)
    : QWidget(parent)
    , m_currentOrderId(0)
    , m_hasOverlay(false)
{
    setupUI();
}

void SettlementEditPage::refreshData()
{
    m_searchOrder->clear();
    clearDisplay();
}

void SettlementEditPage::clearDisplay()
{
    m_lblVehicleInfo->setText("请搜索工单");
    m_laborTable->setRowCount(0);
    m_partsTable->setRowCount(0);
    for (int c = 1; c < 12; c += 2) {
        QTableWidgetItem *val = m_summaryTable->item(0, c);
        if (val) val->setText("¥0.00");
    }
    m_currentOrderId = 0;
    m_currentOrderNo.clear();
    m_hasOverlay = false;
    m_btnAddLabor->setVisible(false);
    m_btnAddPart->setVisible(false);
    m_btnSaveEdit->setVisible(false);
    m_btnPrint->setVisible(false);
    m_btnSavePdf->setVisible(false);
}

void SettlementEditPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("结算修改");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // ==================== 1. 查询工单（仅已结算） ====================
    QGroupBox *searchGroup = new QGroupBox("查询已结算工单");
    QHBoxLayout *searchLayout = new QHBoxLayout(searchGroup);
    searchLayout->addWidget(new QLabel("工单号/车牌:"));
    m_searchOrder = new QLineEdit;
    m_searchOrder->setPlaceholderText("输入工单号或车牌号，回车搜索（仅显示已结算工单）");
    searchLayout->addWidget(m_searchOrder, 1);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->addWidget(searchGroup);

    // ==================== 2. 工单信息 ====================
    QGroupBox *infoGroup = new QGroupBox("工单信息");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    m_lblVehicleInfo = new QLabel("请搜索工单");
    m_lblVehicleInfo->setStyleSheet(
        "padding:12px;background:#f0f3f5;border-radius:4px;font-size:16px;"
        "border:1px solid #dcdde1;");
    m_lblVehicleInfo->setWordWrap(true);
    m_lblVehicleInfo->setMinimumHeight(40);
    infoLayout->addWidget(m_lblVehicleInfo);

    // ---- 工时费明细表（单列一行一条，便于增删） ----
    QHBoxLayout *laborHeadRow = new QHBoxLayout;
    QLabel *laborLabel = new QLabel("▸ 工时费明细（可修改费用/主修人/维修内容，可新增/删除）");
    laborLabel->setStyleSheet("font-weight:bold;font-size:15px;color:#2c3e50;margin-top:6px;");
    laborHeadRow->addWidget(laborLabel, 1);
    m_btnAddLabor = new QPushButton("＋新增工时");
    m_btnAddLabor->setStyleSheet(
        "QPushButton{padding:4px 14px;border:none;border-radius:3px;"
        "background:#3498db;color:#fff;font-weight:bold;}"
        "QPushButton:hover{background:#2980b9;}");
    laborHeadRow->addWidget(m_btnAddLabor);
    infoLayout->addLayout(laborHeadRow);

    m_laborTable = new QTableWidget(0, 5);
    m_laborTable->setHorizontalHeaderLabels({"类别","主修人","维修内容","费用","操作"});
    m_laborTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_laborTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_laborTable->verticalHeader()->setVisible(false);
    m_laborTable->horizontalHeader()->setStretchLastSection(true);
    m_laborTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_laborTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_laborTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_laborTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_laborTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:13px;}"
        "QTableWidget::item{padding:2px 4px;font-size:13px;}");
    m_laborTable->setItemDelegateForColumn(0, new LaborTypeDelegate(this));
    m_laborTable->setItemDelegateForColumn(3, new MoneyDelegate(this));
    infoLayout->addWidget(m_laborTable);

    // ---- 材料明细表 ----
    QHBoxLayout *partsHeadRow = new QHBoxLayout;
    QLabel *partsLabel = new QLabel("▸ 材料明细（可修改数量/售价，可新增/删除；纯显示，不扣库存）");
    partsLabel->setStyleSheet("font-weight:bold;font-size:15px;color:#2c3e50;margin-top:6px;");
    partsHeadRow->addWidget(partsLabel, 1);
    m_btnAddPart = new QPushButton("＋新增材料");
    m_btnAddPart->setStyleSheet(
        "QPushButton{padding:4px 14px;border:none;border-radius:3px;"
        "background:#27ae60;color:#fff;font-weight:bold;}"
        "QPushButton:hover{background:#219a52;}");
    partsHeadRow->addWidget(m_btnAddPart);
    infoLayout->addLayout(partsHeadRow);

    m_partsTable = new QTableWidget(0, 5);
    m_partsTable->setHorizontalHeaderLabels({"材料名称","数量","售价","小计","操作"});
    m_partsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partsTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
                                  | QAbstractItemView::EditKeyPressed);
    m_partsTable->verticalHeader()->setVisible(false);
    m_partsTable->horizontalHeader()->setStretchLastSection(true);
    m_partsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_partsTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_partsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_partsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_partsTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:13px;}"
        "QTableWidget::item{padding:2px 4px;font-size:13px;}");
    m_partsTable->setItemDelegateForColumn(2, new MoneyDelegate(this));
    m_partsTable->setItemDelegateForColumn(3, new MoneyDelegate(this));
    infoLayout->addWidget(m_partsTable);

    // ---- 费用总计表 ----
    QLabel *summaryLabel = new QLabel("▸ 费用总计");
    summaryLabel->setStyleSheet("font-weight:bold;font-size:15px;color:#2c3e50;margin-top:6px;");
    infoLayout->addWidget(summaryLabel);
    m_summaryTable = new QTableWidget(1, 12);
    m_summaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->horizontalHeader()->setVisible(false);
    m_summaryTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_summaryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_summaryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    for (int c = 0; c < 12; c++)
        m_summaryTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    m_summaryTable->setMaximumHeight(50);
    m_summaryTable->setStyleSheet(
        "QTableWidget{background:#f8f9fa;border:1px solid #dcdde1;font-size:13px;}"
        "QTableWidget::item{padding:3px 6px;}");
    struct { int col; QString label; bool highlight; } summaryFields[] = {
        {0,  "工时费合计", false},
        {2,  "材料费合计", false},
        {4,  "优惠",       false},
        {6,  "其他费",     false},
        {8,  "管理费",     false},
        {10, "应收合计",   true},
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

    // ---- 费用编辑区 ----
    QHBoxLayout *editFeeRow = new QHBoxLayout;
    editFeeRow->setSpacing(8);
    editFeeRow->addWidget(new QLabel("优惠:"));
    m_editDiscount = new QDoubleSpinBox;
    m_editDiscount->setRange(0, 999999.99);
    m_editDiscount->setPrefix("¥ ");
    m_editDiscount->setDecimals(2);
    editFeeRow->addWidget(m_editDiscount);
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

    // ==================== 3. 打印按钮 ====================
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    m_btnPrint = new QPushButton("打印结算单");
    m_btnPrint->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#2980b9;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#1f6fa5;}");
    m_btnPrint->setMinimumHeight(40);
    m_btnPrint->setVisible(false);
    m_btnSavePdf = new QPushButton("保存到PDF");
    m_btnSavePdf->setStyleSheet(
        "QPushButton{padding:10px 24px;border:none;border-radius:4px;"
        "background:#8e44ad;color:#fff;font-size:14px;font-weight:bold;}"
        "QPushButton:hover{background:#7d3c98;}");
    m_btnSavePdf->setMinimumHeight(40);
    m_btnSavePdf->setVisible(false);
    btnLayout->addWidget(m_btnPrint);
    btnLayout->addWidget(m_btnSavePdf);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // ==================== 行删除按钮组 ====================
    m_laborBtnGroup = new QButtonGroup(this);
    connect(m_laborBtnGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *b) {
        QPushButton *btn = qobject_cast<QPushButton*>(b);
        if (!btn) return;
        for (int r = 0; r < m_laborTable->rowCount(); r++) {
            if (m_laborTable->cellWidget(r, 4) == btn) {
                m_laborTable->removeRow(r);
                break;
            }
        }
        updateSummary();
    });
    m_partBtnGroup = new QButtonGroup(this);
    connect(m_partBtnGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, [this](QAbstractButton *b) {
        QPushButton *btn = qobject_cast<QPushButton*>(b);
        if (!btn) return;
        for (int r = 0; r < m_partsTable->rowCount(); r++) {
            if (m_partsTable->cellWidget(r, 4) == btn) {
                m_partsTable->removeRow(r);
                break;
            }
        }
        updateSummary();
    });

    // ==================== 信号 ====================
    m_orderCompleter = new SearchCompleter(this);
    m_orderCompleter->setEdit(m_searchOrder);
    connect(m_orderCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx >= 0 && idx < m_orderRows.size()) {
            QSignalBlocker blocker(m_searchOrder);
            m_searchOrder->setText(m_orderRows[idx][0]);
            loadOrderInfo(m_orderRows[idx][0]);
        }
    });
    connect(m_searchOrder, &QLineEdit::textChanged, this, [this](const QString &) {
        onOrderSearch();
    });
    connect(m_btnAddLabor, &QPushButton::clicked, this, &SettlementEditPage::onAddLabor);
    connect(m_btnAddPart, &QPushButton::clicked, this, &SettlementEditPage::onAddPart);
    connect(m_btnSaveEdit, &QPushButton::clicked, this, &SettlementEditPage::onSaveEdit);
    connect(m_btnPrint, &QPushButton::clicked, this, &SettlementEditPage::onPrintSettlement);
    connect(m_btnSavePdf, &QPushButton::clicked, this, &SettlementEditPage::onSaveToPdf);
    connect(m_editOtherFee, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SettlementEditPage::onFeeEditChanged);
    connect(m_editMgmtFee, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SettlementEditPage::onFeeEditChanged);
    connect(m_editDiscount, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &SettlementEditPage::onFeeEditChanged);
    connect(m_laborTable, &QTableWidget::cellChanged,
            this, &SettlementEditPage::onLaborCellChanged);
    connect(m_partsTable, &QTableWidget::cellChanged,
            this, &SettlementEditPage::onPartsCellChanged);
}

// ============================================================
// 查询已结算工单
// ============================================================
void SettlementEditPage::onOrderSearch()
{
    QString text = m_searchOrder->text().trimmed();
    if (text.isEmpty()) {
        m_orderRows.clear();
        m_orderCompleter->hideDropdown();
        return;
    }

    RemoteQuery q;
    q.prepare(QString(
        "SELECT w.id, w.order_no, w.status, COALESCE(v.plate_number,'') AS plate, "
        "w.repair_content, w.created_at "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE w.status = '已结算' "
        "AND %1 "
        "ORDER BY w.id DESC LIMIT 30").arg(SqlUtil::likeConds(
            {"w.order_no", "v.plate_number"}, ":kw")));
    q.bindValue(":kw", SqlUtil::likePattern(text));
    q.exec();

    m_orderRows.clear();
    while (q.next()) {
        QStringList row;
        row << q.value(1).toString()  // order_no
            << q.value(2).toString()  // status
            << q.value(3).toString()  // plate
            << q.value(4).toString()  // repair_content
            << q.value(5).toDateTime().toString("yyyy-MM-dd HH:mm");
        m_orderRows << row;
    }

    if (m_orderRows.isEmpty()) {
        m_orderCompleter->hideDropdown();
        return;
    }

    QList<QVariant> ids;
    for (int i = 0; i < m_orderRows.size(); ++i)
        ids << i;
    m_orderCompleter->setResults(m_orderRows, ids);
    m_orderCompleter->showDropdown();
}

// ============================================================
// 加载已结算工单详细信息
// ============================================================
void SettlementEditPage::loadOrderInfo(const QString &orderNo)
{
    m_currentOrderNo = orderNo;
    RemoteQuery q;

    q.prepare(
        "SELECT w.id, w.order_no, w.status, w.labor_fee, w.material_fee, "
        "  w.other_fee, w.management_fee, w.discount, w.total_amount, "
        "  w.repair_content, w.created_at, w.mileage, w.repair_date, "
        "  v.plate_number, v.vin, v.model, v.engine_number, "
        "  COALESCE(v.owner_name,''), COALESCE(v.owner_phone,''), COALESCE(v.owner_address,''), "
        "  COALESCE(e.name,'') "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_employee e ON e.id = w.customer_service_id "
        "WHERE w.order_no = :no");
    q.bindValue(":no", orderNo);
    q.exec();

    if (!q.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }
    if (q.value(2).toString() != "已结算") {
        QMessageBox::warning(this, "提示", "仅支持对「已结算」状态的工单进行结算修改");
        clearDisplay();
        return;
    }

    m_currentOrderId = q.value(0).toInt();
    double storedOther = q.value(5).toDouble();
    double storedMgmt  = q.value(6).toDouble();
    double storedDisc  = q.value(7).toDouble();
    QString repairContent = q.value(9).toString();
    QString createdAt = q.value(10).toDateTime().toString("yyyy-MM-dd HH:mm");
    m_mileage = q.value(11).toInt();
    QString repairDate = q.value(12).toDate().toString("yyyy-MM-dd");
    m_entryDate = !repairDate.isEmpty() ? repairDate : createdAt.mid(0, 10);
    m_plate = q.value(13).toString();
    m_vin = q.value(14).toString();
    m_model = q.value(15).toString();
    m_engine = q.value(16).toString();
    m_ownerName = q.value(17).toString();
    m_ownerPhone = q.value(18).toString();
    QString svcAdvisor = q.value(20).toString();

    // 1. 是否已有修改快照（overlay）
    m_hasOverlay = false;
    {
        RemoteQuery cq;
        cq.prepare("SELECT COUNT(*) FROM t_settlement_edit WHERE workorder_id=:oid");
        cq.bindValue(":oid", m_currentOrderId);
        if (cq.exec() && cq.next()) m_hasOverlay = cq.value(0).toInt() > 0;
    }

    double otherFee = storedOther, mgmtFee = storedMgmt, discount = storedDisc;
    if (m_hasOverlay) {
        RemoteQuery sq;
        sq.prepare("SELECT labor_fee, material_fee, other_fee, management_fee, discount, total_amount "
                   "FROM t_settlement_edit WHERE workorder_id=:oid");
        sq.bindValue(":oid", m_currentOrderId);
        if (sq.exec() && sq.next()) {
            otherFee = sq.value(2).toDouble();
            mgmtFee  = sq.value(3).toDouble();
            discount = sq.value(4).toDouble();
        }
    }

    // 2. 车辆 + 车主信息
    QString vehicleHtml = QString(
        "<table width='100%%' cellspacing='4' style='font-size:15px;'>"
        "<tr>"
        "<td><b>工单号:</b> %1</td>"
        "<td><b>状态:</b> <span style='color:#e67e22;font-weight:bold;'>%2</span></td>"
        "<td><b>车牌号:</b> %3</td>"
        "<td><b>车型:</b> %4</td>"
        "<td><b>VIN码:</b> %5</td>"
        "<td><b>发动机号:</b> %6</td>"
        "</tr>"
        "<tr>"
        "<td><b>车主:</b> %7</td>"
        "<td><b>电话:</b> %8</td>"
        "<td><b>服务顾问:</b> %9</td>"
        "<td><b>创建:</b> %11</td>"
        "<td colspan='2'><b>报修内容:</b> %10</td>"
        "</tr>"
        "</table>")
        .arg(orderNo, "已结算", m_plate, m_model, m_vin, m_engine,
             m_ownerName, m_ownerPhone, svcAdvisor, repairContent, createdAt);
    if (m_hasOverlay) {
        vehicleHtml += QString("<div style='margin-top:4px;font-size:13px;color:#e67e22;font-weight:bold;'>"
                               "（当前显示为结算修改后的快照，原结算数据未变动）</div>");
    }
    m_lblVehicleInfo->setText(vehicleHtml);

    // 3. 工时明细
    m_laborTable->setRowCount(0);
    m_laborTable->blockSignals(true);
    {
        const QString tbl = m_hasOverlay ? "t_settlement_edit_repair" : "t_workorder_repair_item";
        RemoteQuery lq;
        lq.prepare(QString("SELECT item_type, repair_person, repair_content, fee "
                           "FROM %1 WHERE workorder_id=:oid ORDER BY item_type, id").arg(tbl));
        lq.bindValue(":oid", m_currentOrderId);
        lq.exec();
        while (lq.next()) {
            const int row = m_laborTable->rowCount();
            m_laborTable->insertRow(row);
            setLaborRow(row, lq.value(0).toString(), lq.value(1).toString(),
                        lq.value(2).toString().trimmed(), lq.value(3).toDouble());
        }
    }
    m_laborTable->blockSignals(false);

    // 4. 材料明细
    m_partsTable->setRowCount(0);
    m_partsTable->blockSignals(true);
    {
        if (m_hasOverlay) {
            RemoteQuery pq;
            pq.prepare("SELECT part_id, part_name, quantity, unit_price, subtotal "
                       "FROM t_settlement_edit_item WHERE workorder_id=:oid ORDER BY id");
            pq.bindValue(":oid", m_currentOrderId);
            pq.exec();
            while (pq.next()) {
                const int row = m_partsTable->rowCount();
                m_partsTable->insertRow(row);
                setPartRow(row, pq.value(0).toInt(), pq.value(1).toString(),
                           pq.value(2).toDouble(), pq.value(3).toDouble());
            }
        } else {
            RemoteQuery pq;
            pq.prepare(
                "SELECT COALESCE(MAX(wi.part_id),0), wi.part_name, SUM(wi.quantity), "
                "  wi.unit_price, SUM(wi.subtotal) "
                "FROM t_workorder_item wi "
                "WHERE wi.workorder_id=:oid AND wi.item_type='材料' "
                "GROUP BY wi.part_name, wi.unit_price ORDER BY wi.part_name");
            pq.bindValue(":oid", m_currentOrderId);
            pq.exec();
            while (pq.next()) {
                const int row = m_partsTable->rowCount();
                m_partsTable->insertRow(row);
                setPartRow(row, pq.value(0).toInt(), pq.value(1).toString(),
                           pq.value(2).toDouble(), pq.value(3).toDouble());
            }
        }
    }
    m_partsTable->blockSignals(false);

    // 5. 费用编辑控件 + 汇总
    m_editOtherFee->blockSignals(true);
    m_editOtherFee->setValue(otherFee);
    m_editOtherFee->blockSignals(false);
    m_editMgmtFee->blockSignals(true);
    m_editMgmtFee->setValue(mgmtFee);
    m_editMgmtFee->blockSignals(false);
    m_editDiscount->blockSignals(true);
    m_editDiscount->setValue(discount);
    m_editDiscount->blockSignals(false);

    updateSummary();

    // 6. 显示操作按钮
    m_btnAddLabor->setVisible(true);
    m_btnAddPart->setVisible(true);
    m_btnSaveEdit->setVisible(true);
    m_btnPrint->setVisible(true);
    m_btnSavePdf->setVisible(true);
}

void SettlementEditPage::setLaborRow(int row, const QString &typ, const QString &person,
                                     const QString &content, double fee)
{
    m_laborTable->setItem(row, 0, new QTableWidgetItem(typ));
    m_laborTable->setItem(row, 1, new QTableWidgetItem(person));
    m_laborTable->setItem(row, 2, new QTableWidgetItem(content));
    m_laborTable->setItem(row, 3, new QTableWidgetItem(QString::number(fee, 'f', 2)));

    QPushButton *btn = new QPushButton("删除");
    btn->setStyleSheet(
        "QPushButton{padding:2px 10px;border:none;border-radius:3px;"
        "background:#e74c3c;color:#fff;font-size:12px;}"
        "QPushButton:hover{background:#c0392b;}");
    m_laborBtnGroup->addButton(btn);
    m_laborTable->setCellWidget(row, 4, btn);
}

void SettlementEditPage::setPartRow(int row, int partId, const QString &name,
                                    double qty, double price)
{
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    nameItem->setData(Qt::UserRole, partId);   // 保留原 part_id（新增行=0）
    m_partsTable->setItem(row, 0, nameItem);
    m_partsTable->setItem(row, 1, new QTableWidgetItem(QString::number(qty, 'f', 3)));
    m_partsTable->setItem(row, 2, new QTableWidgetItem(QString::number(price, 'f', 2)));

    QTableWidgetItem *subItem = new QTableWidgetItem(QString::number(qty * price, 'f', 2));
    subItem->setFlags(subItem->flags() & ~Qt::ItemIsEditable);   // 小计只读
    m_partsTable->setItem(row, 3, subItem);

    QPushButton *btn = new QPushButton("删除");
    btn->setStyleSheet(
        "QPushButton{padding:2px 10px;border:none;border-radius:3px;"
        "background:#e74c3c;color:#fff;font-size:12px;}"
        "QPushButton:hover{background:#c0392b;}");
    m_partBtnGroup->addButton(btn);
    m_partsTable->setCellWidget(row, 4, btn);
}

// ============================================================
// 刷新费用总计（工时+材料+其他+管理−优惠）
// ============================================================
void SettlementEditPage::updateSummary()
{
    double laborTotal = 0;
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        QTableWidgetItem *feeItem = m_laborTable->item(r, 3);
        if (feeItem) laborTotal += feeItem->text().toDouble();
    }
    double partsTotal = 0;
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        QTableWidgetItem *subItem = m_partsTable->item(r, 3);
        if (subItem) partsTotal += subItem->text().toDouble();
    }

    double otherFee = m_editOtherFee->value();
    double mgmtFee  = m_editMgmtFee->value();
    double discount = m_editDiscount->value();
    double grandTotal = laborTotal + partsTotal + otherFee + mgmtFee - discount;

    struct { int c; double v; } feeMap[] = {
        {1, laborTotal}, {3, partsTotal}, {5, discount}, {7, otherFee},
        {9, mgmtFee}, {11, grandTotal},
    };
    for (const auto &f : feeMap) {
        QTableWidgetItem *val = m_summaryTable->item(0, f.c);
        if (val) val->setText(QString("¥%1").arg(f.v, 0, 'f', 2));
    }
}

void SettlementEditPage::onFeeEditChanged()
{
    updateSummary();
}

void SettlementEditPage::onLaborCellChanged(int, int)
{
    updateSummary();
}

void SettlementEditPage::onPartsCellChanged(int row, int column)
{
    // 数量/售价改动 → 重算小计
    if (column == 1 || column == 2) {
        QTableWidgetItem *qtyItem  = m_partsTable->item(row, 1);
        QTableWidgetItem *prcItem  = m_partsTable->item(row, 2);
        QTableWidgetItem *subItem  = m_partsTable->item(row, 3);
        if (qtyItem && prcItem && subItem) {
            QSignalBlocker blocker(m_partsTable);
            subItem->setText(QString::number(qtyItem->text().toDouble() * prcItem->text().toDouble(), 'f', 2));
        }
    }
    updateSummary();
}

void SettlementEditPage::onAddLabor()
{
    if (m_currentOrderId == 0) return;
    const int row = m_laborTable->rowCount();
    m_laborTable->insertRow(row);
    setLaborRow(row, "机电", "", "", 0);
    updateSummary();
}

void SettlementEditPage::onAddPart()
{
    if (m_currentOrderId == 0) return;
    const int row = m_partsTable->rowCount();
    m_partsTable->insertRow(row);
    setPartRow(row, 0, "", 1, 0);
    updateSummary();
}

// ============================================================
// 保存修改：单事务写入 overlay 表（绝不碰原表）
// ============================================================
void SettlementEditPage::onSaveEdit()
{
    if (m_currentOrderId == 0) return;

    if (QMessageBox::question(this, "确认保存",
            QString("确认保存结算修改？\n\n工单号: %1\n\n"
                    "修改结果仅在本「结算修改」入口显示，原始结算数据不会变动，"
                    "其他入口（工单查询/统计/流水）仍以原结算为准。")
            .arg(m_currentOrderNo),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // 收集工时条目（跳过完全空行）
    struct LRow { QString typ, person, content; double fee; };
    QList<LRow> labors;
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        QTableWidgetItem *t = m_laborTable->item(r, 0);
        QTableWidgetItem *p = m_laborTable->item(r, 1);
        QTableWidgetItem *c = m_laborTable->item(r, 2);
        QTableWidgetItem *f = m_laborTable->item(r, 3);
        if (!t || !f) continue;
        QString typ     = t->text().trimmed();
        QString person  = p ? p->text().trimmed() : QString();
        QString content = c ? c->text().trimmed() : QString();
        double  fee     = f->text().toDouble();
        if (content.isEmpty() && person.isEmpty() && fee < 0.005) continue;
        if (typ.isEmpty()) typ = "机电";
        labors.append({typ, person, content, fee});
    }

    // 收集材料条目（跳过空名称行）
    struct PRow { int partId; QString name; double qty, price; };
    QList<PRow> parts;
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        QTableWidgetItem *n = m_partsTable->item(r, 0);
        QTableWidgetItem *q = m_partsTable->item(r, 1);
        QTableWidgetItem *p = m_partsTable->item(r, 2);
        if (!n || !q || !p) continue;
        const QString name = n->text().trimmed();
        if (name.isEmpty()) continue;
        parts.append({n->data(Qt::UserRole).toInt(), name, q->text().toDouble(), p->text().toDouble()});
    }

    double laborTotal = 0;
    for (const auto &L : labors) laborTotal += L.fee;
    double partsTotal = 0;
    for (const auto &P : parts) partsTotal += P.qty * P.price;
    double otherFee = m_editOtherFee->value();
    double mgmtFee  = m_editMgmtFee->value();
    double discount = m_editDiscount->value();
    double grandTotal = laborTotal + partsTotal + otherFee + mgmtFee - discount;

    const QVariant operatorId = Session::instance().isLoggedIn()
        ? QVariant(Session::instance().userId()) : QVariant();

    QJsonArray steps;
    // 1. 费用快照（upsert）
    steps.append(RemoteDb::step(
        "INSERT INTO t_settlement_edit "
        "(workorder_id, labor_fee, material_fee, other_fee, management_fee, discount, total_amount, operator_id) "
        "VALUES (:oid, :labor, :mat, :other, :mgmt, :disc, :total, :by) "
        "ON DUPLICATE KEY UPDATE "
        "labor_fee=VALUES(labor_fee), material_fee=VALUES(material_fee), "
        "other_fee=VALUES(other_fee), management_fee=VALUES(management_fee), "
        "discount=VALUES(discount), total_amount=VALUES(total_amount), "
        "operator_id=VALUES(operator_id), updated_at=NOW()",
        QJsonObject{
            {":oid", m_currentOrderId},
            {":labor", laborTotal},
            {":mat", partsTotal},
            {":other", otherFee},
            {":mgmt", mgmtFee},
            {":disc", discount},
            {":total", grandTotal},
            {":by", RemoteDb::v(operatorId)}
        }));

    // 2. 工时条目（删后重插）
    steps.append(RemoteDb::step(
        "DELETE FROM t_settlement_edit_repair WHERE workorder_id=:oid",
        QJsonObject{ {":oid", m_currentOrderId} }));
    for (const auto &L : labors) {
        steps.append(RemoteDb::step(
            "INSERT INTO t_settlement_edit_repair "
            "(workorder_id, item_type, repair_person, repair_content, fee) "
            "VALUES (:oid, :typ, :person, :content, :fee)",
            QJsonObject{
                {":oid", m_currentOrderId},
                {":typ", L.typ},
                {":person", RemoteDb::v(L.person.isEmpty() ? QVariant() : QVariant(L.person))},
                {":content", RemoteDb::v(L.content.isEmpty() ? QVariant() : QVariant(L.content))},
                {":fee", L.fee}
            }));
    }

    // 3. 材料条目（删后重插）
    steps.append(RemoteDb::step(
        "DELETE FROM t_settlement_edit_item WHERE workorder_id=:oid",
        QJsonObject{ {":oid", m_currentOrderId} }));
    for (const auto &P : parts) {
        steps.append(RemoteDb::step(
            "INSERT INTO t_settlement_edit_item "
            "(workorder_id, part_id, part_name, quantity, unit_price, subtotal) "
            "VALUES (:oid, :pid, :name, :qty, :price, :sub)",
            QJsonObject{
                {":oid", m_currentOrderId},
                {":pid", RemoteDb::v(P.partId > 0 ? QVariant(P.partId) : QVariant())},
                {":name", P.name},
                {":qty", P.qty},
                {":price", P.price},
                {":sub", P.qty * P.price}
            }));
    }

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "保存失败", txn.value("error").toString());
        return;
    }

    m_hasOverlay = true;
    QMessageBox::information(this, "成功",
        QString("结算修改已保存（仅本入口生效）。\n工单号: %1\n应收合计: ¥%2")
        .arg(m_currentOrderNo).arg(grandTotal, 0, 'f', 2));
    loadOrderInfo(m_currentOrderNo);
}

// ============================================================
// 构建「修改后结算单」分区块（读取页面当前实况，不读原表）
// 方案B：分区块 + QPainter 拼版，纵向铺满一页
// ============================================================
QList<SettlementSection> SettlementEditPage::buildEditSettlementSections() const
{
    if (m_currentOrderId == 0) return QList<SettlementSection>();

    // ---- 工时行 ----
    QString laborRows;
    int laborSeq = 0;
    double laborTotal = 0;
    for (int r = 0; r < m_laborTable->rowCount(); r++) {
        QTableWidgetItem *t = m_laborTable->item(r, 0);
        QTableWidgetItem *p = m_laborTable->item(r, 1);
        QTableWidgetItem *c = m_laborTable->item(r, 2);
        QTableWidgetItem *f = m_laborTable->item(r, 3);
        if (!t || !f) continue;
        QString typ     = t->text().trimmed();
        QString person  = p ? p->text().trimmed() : QString();
        QString content = c ? c->text().trimmed() : QString();
        double  fee     = f->text().toDouble();
        if (content.isEmpty() && person.isEmpty() && fee < 0.005) continue;
        laborSeq++;
        laborTotal += fee;
        QString serviceItem = typ;
        if (!content.isEmpty()) serviceItem += ": " + content;
        laborRows += QString(
            "<tr><td class='c'>%1</td>"
            "<td>%2</td>"
            "<td class='c'>%3</td>"
            "<td class='amt'>%4</td></tr>")
            .arg(laborSeq)
            .arg(PrintUtil::esc(serviceItem))
            .arg(person.isEmpty() ? "" : PrintUtil::esc(person))
            .arg(PrintUtil::money(fee));
    }

    // ---- 材料行 ----
    QString partsRows;
    int partsSeq = 0;
    double partsTotal = 0;
    for (int r = 0; r < m_partsTable->rowCount(); r++) {
        QTableWidgetItem *n = m_partsTable->item(r, 0);
        QTableWidgetItem *q = m_partsTable->item(r, 1);
        QTableWidgetItem *p = m_partsTable->item(r, 2);
        if (!n || !q || !p) continue;
        const QString name = n->text().trimmed();
        if (name.isEmpty()) continue;
        double qty = q->text().toDouble();
        double price = p->text().toDouble();
        double sub = qty * price;
        partsSeq++;
        partsTotal += sub;
        partsRows += QString(
            "<tr><td class='c'>%1</td>"
            "<td>-</td><td>%2</td>"
            "<td class='c'>个</td>"
            "<td class='c'>%3</td>"
            "<td class='amt'>%4</td>"
            "<td class='amt'>%5</td></tr>")
            .arg(partsSeq)
            .arg(PrintUtil::esc(name))
            .arg(QString::number(qty))
            .arg(PrintUtil::money(price))
            .arg(PrintUtil::money(sub));
    }

    double otherFee = m_editOtherFee->value();
    double mgmtFee  = m_editMgmtFee->value();
    double discount = m_editDiscount->value();
    double grandTotal = laborTotal + partsTotal + otherFee + mgmtFee - discount;
    QString amountInWords = PrintUtil::chineseUpper(grandTotal);

    const QString settlementPerson = Session::instance().userName();
    const QString settlementDate   = QDate::currentDate().toString("yyyy-MM-dd");

    const QString address = AppSettings::officeAddress();

    auto esc = [](const QString &s) { return s.toHtmlEscaped(); };

    // 头部：标题（工号已移至底部区块）
    QString headHtml =
        "<div class='title'>成都科盟汽车服务有限责任公司维修结算单</div>";

    // 送修信息 3×3（有框）
    QString infoHtml =
        "<table class='form' width='100%'>"
        "<tr><td>送修单位：%OWNER%</td><td>联系人：%CONTACT%</td><td>联系电话：%PHONE%</td></tr>"
        "<tr><td>车牌号：%PLATE%</td><td>车型：%MODEL%</td><td>发动机号：%ENGINE%</td></tr>"
        "<tr><td>车身号：%VIN%</td><td>送修日期：%ENTRYDATE%</td><td>里程数：%MILEAGE% km</td></tr>"
        "</table>";
    infoHtml.replace("%OWNER%", esc(m_ownerName)).replace("%CONTACT%", esc(m_ownerName)).replace("%PHONE%", esc(m_ownerPhone));
    infoHtml.replace("%PLATE%", esc(m_plate)).replace("%MODEL%", esc(m_model)).replace("%ENGINE%", esc(m_engine));
    infoHtml.replace("%VIN%", esc(m_vin)).replace("%ENTRYDATE%", m_entryDate).replace("%MILEAGE%", QString::number(m_mileage));

    // 工时区（grow）：表头不画框，列宽 序号+主修人+工时费=50%，维修项目=50%
    QString laborRowsHtml = laborRows.isEmpty()
        ? QString("<tr><td class='c'>-</td><td>暂无工时费明细</td>"
                  "<td class='c'>-</td><td class='amt'>-</td></tr>")
        : laborRows;
    QString laborHtml =
        "<div class='area-title'>维修内容</div>"
        "<table class='plain' width='100%'>"
        "<tr class='hd'>"
        "<td width='16.7%'>序号</td>"
        "<td width='50%'>维修项目</td>"
        "<td width='16.7%'>主修人</td>"
        "<td width='16.6%'>工时费</td>"
        "</tr>"
        "%LABORROWS%"
        "<tr class='sub'><td colspan='3' class='r'>工时费小计</td><td class='amt'>%LABORTOTAL%</td></tr>"
        "</table>";
    laborHtml.replace("%LABORROWS%", laborRowsHtml).replace("%LABORTOTAL%", PrintUtil::money(laborTotal));

    // 材料区（grow）：无内部网格，仅顶部实线
    QString partsRowsHtml = partsRows.isEmpty()
        ? QString("<tr><td class='c'>-</td><td>-</td><td>暂无材料明细</td>"
                  "<td class='c'>-</td><td class='c'>-</td>"
                  "<td class='amt'>-</td><td class='amt'>-</td></tr>")
        : partsRows;
    QString partsHtml =
        "<div class='area-title'>材料清单</div>"
        "<table class='plain' width='100%'>"
        "<tr class='hd'>"
        "<td width='9%'>序号</td>"
        "<td width='15%'>配件件号</td>"
        "<td width='22%'>配件名称</td>"
        "<td width='9%'>单位</td>"
        "<td width='9%'>数量</td>"
        "<td width='18%'>单价</td>"
        "<td width='18%'>金额</td>"
        "</tr>"
        "%PARTSROWS%"
        "<tr class='sub'><td colspan='6' class='r'>材料费小计</td><td class='amt'>%PARTSTOTAL%</td></tr>"
        "</table>";
    partsHtml.replace("%PARTSROWS%", partsRowsHtml).replace("%PARTSTOTAL%", PrintUtil::money(partsTotal));

    // 汇总区（固定）：12列 3项/行各4列；应收款加大加粗；大写金额3/4 + 客户签字1/4
    QString summaryHtml =
        "<table class='form' width='100%'>"
        "<tr>"
        "<td colspan='4' class='lbl' width='33.3%'>材料费 %MFEE%</td>"
        "<td colspan='4' class='lbl' width='33.3%'>工时费 %LFEE%</td>"
        "<td colspan='4' class='lbl' width='33.4%'>管理费 %MGM%</td>"
        "</tr>"
        "<tr>"
        "<td colspan='4' class='lbl' width='33.3%'>总费用 %GRAND%</td>"
        "<td colspan='4' class='lbl' width='33.3%'>总优惠 %DISCOUNT%</td>"
        "<td colspan='4' class='recv' width='33.4%'>应收款 %RECV%</td>"
        "</tr>"
        "<tr>"
        "<td colspan='9' class='words' width='75%'>大写金额：%WORDS%</td>"
        "<td colspan='3' class='sign' width='25%'>客户签字：</td>"
        "</tr>"
        "</table>";
    summaryHtml.replace("%MFEE%", PrintUtil::money(partsTotal))
        .replace("%LFEE%", PrintUtil::money(laborTotal))
        .replace("%MGM%", PrintUtil::money(mgmtFee))
        .replace("%GRAND%", PrintUtil::money(grandTotal))
        .replace("%DISCOUNT%", PrintUtil::money(discount))
        .replace("%RECV%", PrintUtil::money(grandTotal))
        .replace("%WORDS%", amountInWords);

    // 底部区块（固定）：工号（从右上角移至此）+ 地址/服务电话同行 +
    // 结算人/结算日期/收款人签字 三个独立表格；原收款人签字行/出厂日期已并入。
    QString servicePhone = AppSettings::servicePhone();
    if (servicePhone.isEmpty()) servicePhone = "028-________";
    // 地址+服务电话、结算人/结算日期/收款人签字 两行：无表格框线，仅对齐
    QString footerHtml =
        "<div class='order-no'>工号：%ORDER%</div>"
        "<table class='main' width='100%'>"
        "<tr><td width='66.7%'>地址：%ADDRESS%</td><td width='33.3%'>服务电话：%PHONE%</td></tr>"
        "</table>"
        "<table class='main' width='100%'>"
        "<tr>"
        "<td width='33.3%'>结算人：%SETTLER%</td>"
        "<td width='33.3%'>结算日期：%SETTLEDATE%</td>"
        "<td width='33.4%'>收款人签字：%SIGN%</td>"
        "</tr>"
        "</table>";
    footerHtml.replace("%ORDER%", esc(m_currentOrderNo))
        .replace("%ADDRESS%", esc(address))
        .replace("%PHONE%", esc(servicePhone))
        .replace("%SETTLER%", esc(settlementPerson))
        .replace("%SETTLEDATE%", settlementDate)
        .replace("%SIGN%", QString("________________"));

    QList<SettlementSection> out;
    out << SettlementSection{PrintUtil::settlementWrap(headHtml),    false}
        << SettlementSection{PrintUtil::settlementWrap(infoHtml),    false}
        << SettlementSection{PrintUtil::settlementWrap(laborHtml),   true}
        << SettlementSection{PrintUtil::settlementWrap(partsHtml),   true}
        << SettlementSection{PrintUtil::settlementWrap(summaryHtml), false}
        << SettlementSection{PrintUtil::settlementWrap(footerHtml),  false};
    return out;
}

// ============================================================
// 保存到PDF（修改后结算单）
// ============================================================
void SettlementEditPage::onSaveToPdf()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先搜索并选择已结算工单");
        return;
    }
    QString defaultName = QString("结算修改单_%1.pdf").arg(m_currentOrderNo);
    QString filePath = QFileDialog::getSaveFileName(
        this, "保存结算修改单PDF", defaultName, "PDF 文件 (*.pdf)");
    if (filePath.isEmpty()) return;

    if (PrintUtil::renderSectionsToPdf(buildEditSettlementSections(), filePath, this)) {
        QMessageBox::information(this, "导出成功",
            QString("修改后结算单已保存到:\n%1").arg(filePath));
    }
}

// ============================================================
// 打印（修改后结算单）
// ============================================================
void SettlementEditPage::onPrintSettlement()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先搜索并选择已结算工单");
        return;
    }
    PrintUtil::printSectionsPreview(buildEditSettlementSections(), this, "打印修改后结算单");
}
