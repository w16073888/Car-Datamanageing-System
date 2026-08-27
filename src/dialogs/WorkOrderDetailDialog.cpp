#include "WorkOrderDetailDialog.h"
#include "pages/QuotePage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QFileDialog>
#include <QColor>
#include <QFont>

WorkOrderDetailDialog::WorkOrderDetailDialog(const QString &orderNo, QWidget *parent)
    : QDialog(parent)
    , m_orderId(0)
    , m_orderNo(orderNo)
{
    setWindowTitle(QString("工单详情 - %1").arg(orderNo));
    resize(980, 640);
    setupUI();
    loadDetail(orderNo);
}

WorkOrderDetailDialog::~WorkOrderDetailDialog() {}

void WorkOrderDetailDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);

    // ---- 工单信息 ----
    m_lblInfo = new QLabel("加载中...");
    m_lblInfo->setWordWrap(true);
    m_lblInfo->setMinimumHeight(40);
    m_lblInfo->setStyleSheet(
        "padding:10px;background:#f0f3f5;border-radius:4px;font-size:13px;border:1px solid #dcdde1;");
    scrollLayout->addWidget(m_lblInfo);

    // ---- 工时费明细 ----
    QLabel *laborLabel = new QLabel("▸ 工时费明细");
    laborLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    scrollLayout->addWidget(laborLabel);
    m_laborTable = new QTableWidget(0, 9);
    m_laborTable->setHorizontalHeaderLabels({"类别","主修人","维修内容","费用","","类别","主修人","维修内容","费用"});
    m_laborTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_laborTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_laborTable->verticalHeader()->setVisible(false);
    m_laborTable->horizontalHeader()->setStretchLastSection(true);
    m_laborTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_laborTable->horizontalHeader()->resizeSection(4, 6);
    m_laborTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_laborTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    m_laborTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);
    m_laborTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    scrollLayout->addWidget(m_laborTable);

    // ---- 材料明细（含成本列，与工单查询一致） ----
    QLabel *partsLabel = new QLabel("▸ 材料明细");
    partsLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    scrollLayout->addWidget(partsLabel);
    m_partsTable = new QTableWidget(0, 13);
    m_partsTable->setHorizontalHeaderLabels({"材料名称","数量","成本","总成本","单价","总价","","材料名称","数量","成本","总成本","单价","总价"});
    m_partsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_partsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_partsTable->verticalHeader()->setVisible(false);
    m_partsTable->horizontalHeader()->setStretchLastSection(true);
    m_partsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    m_partsTable->horizontalHeader()->resizeSection(6, 6);
    m_partsTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    m_partsTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    m_partsTable->horizontalHeader()->setSectionResizeMode(12, QHeaderView::Stretch);
    m_partsTable->setStyleSheet(
        "QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    scrollLayout->addWidget(m_partsTable);

    // ---- 费用总计 ----
    QLabel *summaryLabel = new QLabel("▸ 费用总计");
    summaryLabel->setStyleSheet("font-weight:bold;font-size:12px;color:#2c3e50;margin-top:4px;");
    scrollLayout->addWidget(summaryLabel);
    m_summaryTable = new QTableWidget(1, 14);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->horizontalHeader()->setVisible(false);
    for (int c = 0; c < 14; c++)
        m_summaryTable->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
    m_summaryTable->setMaximumHeight(50);
    m_summaryTable->setStyleSheet(
        "QTableWidget{background:#f8f9fa;border:1px solid #dcdde1;font-size:11px;}"
        "QTableWidget::item{padding:2px 4px;}");
    scrollLayout->addWidget(m_summaryTable);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // ---- 底部按钮 ----
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *btnPrint = new QPushButton("打印");
    QPushButton *btnPdf = new QPushButton("保存到PDF");
    QPushButton *btnClose = new QPushButton("关闭");
    for (QPushButton *b : {btnPrint, btnPdf}) {
        b->setStyleSheet("padding:8px 18px;border:none;border-radius:3px;background:#2980b9;color:#fff;font-weight:bold;");
        b->setMinimumHeight(34);
    }
    btnClose->setStyleSheet("padding:8px 18px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;");
    btnClose->setMinimumHeight(34);
    btnRow->addStretch();
    btnRow->addWidget(btnPrint);
    btnRow->addWidget(btnPdf);
    btnRow->addWidget(btnClose);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    connect(btnPrint, &QPushButton::clicked, this, &WorkOrderDetailDialog::onPrint);
    connect(btnPdf, &QPushButton::clicked, this, &WorkOrderDetailDialog::onSavePdf);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
}

void WorkOrderDetailDialog::loadDetail(const QString &orderNo)
{
    m_orderNo = orderNo;
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
        m_lblInfo->setText(QString("未找到工单：%1").arg(orderNo));
        return;
    }

    m_orderId = q.value(0).toInt();
    m_status = q.value(2).toString();
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
    QString svcAdvisor = q.value(18).toString();

    // 车辆 + 车主信息（两行，格式与工单查询一致）
    QString vehicleHtml = QString(
        "<table width='100%%' cellspacing='2' style='font-size:12px;'>"
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
        "<td colspan='2'><b>报修内容:</b> %10</td>"
        "<td><b>创建:</b> %11</td>"
        "</tr>"
        "</table>")
        .arg(orderNo, m_status, plate, model, vin, engine,
             ownerName, ownerPhone, svcAdvisor, repairContent, createdAt);
    m_lblInfo->setText(vehicleHtml);

    // 2. 工时费明细表 — 先收集全部条目，再按"先左列后右列"填充
    QSqlQuery lq(DbManager::instance().database());
    lq.prepare("SELECT item_type, repair_person, repair_content, fee "
               "FROM t_workorder_repair_item "
               "WHERE workorder_id = :oid ORDER BY item_type, id");
    lq.bindValue(":oid", m_orderId);
    DbManager::instance().executeQuery(lq);

    struct LaborItem { QString typ, person, content; double fee; };
    QList<LaborItem> laborItems;
    double laborFromItems = 0;
    while (lq.next()) {
        LaborItem it;
        it.typ     = lq.value(0).toString();
        it.person  = lq.value(1).toString();
        it.content = lq.value(2).toString().trimmed();
        it.fee     = lq.value(3).toDouble();
        laborFromItems += it.fee;
        laborItems.append(it);
    }
    double displayLabor = (laborFee > 0) ? laborFee : laborFromItems;

    int laborRowCount = (laborItems.size() + 1) / 2;
    m_laborTable->setRowCount(laborRowCount);
    for (int i = 0; i < laborItems.size(); i++) {
        int row = (i < laborRowCount) ? i : (i - laborRowCount);
        int colBase = (i < laborRowCount) ? 0 : 5;
        const LaborItem &it = laborItems[i];
        m_laborTable->setItem(row, colBase + 0, new QTableWidgetItem(it.typ));
        m_laborTable->setItem(row, colBase + 1, new QTableWidgetItem(it.person.isEmpty() ? "-" : it.person));
        m_laborTable->setItem(row, colBase + 2, new QTableWidgetItem(it.content.isEmpty() ? "-" : it.content));
        m_laborTable->setItem(row, colBase + 3, new QTableWidgetItem(QString("¥%1").arg(it.fee, 0, 'f', 2)));
    }

    // 3. 材料明细表（含成本列，与工单查询一致）
    QSqlQuery pq(DbManager::instance().database());
    pq.prepare(
        "SELECT wi.part_name, COUNT(*) AS qty, wi.unit_price, SUM(wi.subtotal) AS subtotal, "
        "  COALESCE(MAX(p.purchase_price), 0) AS cost "
        "FROM t_workorder_item wi "
        "LEFT JOIN t_parts p ON p.id = wi.part_id "
        "WHERE wi.workorder_id = :oid AND wi.item_type = '材料' "
        "GROUP BY wi.part_name, wi.unit_price "
        "ORDER BY wi.part_name");
    pq.bindValue(":oid", m_orderId);
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
        m_partsTable->setItem(row, colBase + 0, new QTableWidgetItem(it.name));
        m_partsTable->setItem(row, colBase + 1, new QTableWidgetItem(QString::number(it.qty)));
        m_partsTable->setItem(row, colBase + 2, new QTableWidgetItem(
            it.cost > 0 ? QString("¥%1").arg(it.cost, 0, 'f', 2) : "-"));
        m_partsTable->setItem(row, colBase + 3, new QTableWidgetItem(
            it.cost > 0 ? QString("¥%1").arg(it.cost * it.qty, 0, 'f', 2) : "-"));
        m_partsTable->setItem(row, colBase + 4, new QTableWidgetItem(QString("¥%1").arg(it.price, 0, 'f', 2)));
        m_partsTable->setItem(row, colBase + 5, new QTableWidgetItem(QString("¥%1").arg(it.sub, 0, 'f', 2)));
    }

    // 4. 费用总计表
    double grandTotal = displayLabor + partsTotal + otherFee + mgmtFee;
    double unpaid = grandTotal - deposit;
    struct { int c; QString label; bool hl; } summaryFields[] = {
        {0,  "工时费合计", false},
        {2,  "材料费合计", false},
        {4,  "其他费",     false},
        {6,  "管理费",     false},
        {8,  "订金",       false},
        {10, "应收合计",   true},
        {12, "应付尾款",   true},
    };
    for (const auto &f : summaryFields) {
        QTableWidgetItem *lbl = new QTableWidgetItem(f.label);
        lbl->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (f.hl) {
            lbl->setForeground(QColor("#e74c3c"));
            QFont fo = lbl->font(); fo.setBold(true); lbl->setFont(fo);
        }
        m_summaryTable->setItem(0, f.c, lbl);

        double val = 0;
        switch (f.c) {
            case 0:  val = displayLabor; break;
            case 2:  val = partsTotal; break;
            case 4:  val = otherFee; break;
            case 6:  val = mgmtFee; break;
            case 8:  val = deposit; break;
            case 10: val = grandTotal; break;
            case 12: val = unpaid; break;
        }
        QTableWidgetItem *v = new QTableWidgetItem(QString("¥%1").arg(val, 0, 'f', 2));
        v->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (f.hl) {
            v->setForeground(QColor("#e74c3c"));
            QFont fo = v->font(); fo.setBold(true); v->setFont(fo);
        }
        m_summaryTable->setItem(0, f.c + 1, v);
    }
}

// ============================================================
// 打印 / 保存PDF（复用工单查询的结算单文档格式）
// ============================================================
void WorkOrderDetailDialog::onPrint()
{
    if (m_orderId == 0) {
        QMessageBox::warning(this, "提示", "工单不存在，无法打印");
        return;
    }

    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(QString("打印工单 - %1").arg(m_orderNo));
    connect(&preview, &QPrintPreviewDialog::paintRequested, [this](QPrinter *p) {
        QTextDocument doc;
        doc.setHtml(QuotePage::buildSettlementHtmlFor(m_orderId));
        doc.setPageSize(p->pageLayout().paintRectPixels(p->resolution()).size());
        doc.print(p);
    });
    preview.exec();
}

void WorkOrderDetailDialog::onSavePdf()
{
    if (m_orderId == 0) {
        QMessageBox::warning(this, "提示", "工单不存在，无法导出PDF");
        return;
    }

    QString defaultName = QString("工单_%1.pdf").arg(m_orderNo);
    QString filePath = QFileDialog::getSaveFileName(
        this, "保存工单PDF", defaultName, "PDF 文件 (*.pdf)");
    if (filePath.isEmpty()) return;

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QTextDocument doc;
    doc.setHtml(QuotePage::buildSettlementHtmlFor(m_orderId));
    doc.setPageSize(printer.pageLayout().paintRectPixels(printer.resolution()).size());
    doc.print(&printer);

    QMessageBox::information(this, "导出成功",
        QString("工单已保存到:\n%1").arg(filePath));
}
