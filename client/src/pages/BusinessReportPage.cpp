#include "BusinessReportPage.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"
#include "dialogs/WorkOrderDetailDialog.h"
#include "utils/XlsxExporter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTabWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

BusinessReportPage::BusinessReportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
BusinessReportPage::~BusinessReportPage() {}

void BusinessReportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    // 标题行：标题 + 右侧导出按钮
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *title = new QLabel("业务流水");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    titleRow->addWidget(title);
    titleRow->addStretch();

    m_btnExport = new QPushButton("导出Excel");
    m_btnExport->setStyleSheet(
        "QPushButton { padding: 6px 14px; border-radius: 4px;"
        "  background-color: #27ae60; color: white; }"
        "QPushButton:hover { background-color: #229954; }");
    titleRow->addWidget(m_btnExport);
    mainLayout->addLayout(titleRow);

    // ==================== 时间范围（两个查询共用） ====================
    m_dateRange = new DateRangeWidget;
    mainLayout->addWidget(m_dateRange);

    // ==================== 页签：结算工单明细 / 报修工单明细 ====================
    m_tabWidget = new QTabWidget;
    // 与库房工作台页签样式保持一致
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #dcdde1; border-top: none; }"
        "QTabBar::tab { padding: 8px 16px; border: 1px solid #dcdde1;"
        "  border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px;"
        "  font-size: 12px; }"
        "QTabBar::tab:selected { background: #3498db; color: white; }"
        "QTabBar::tab:!selected { background: #ecf0f1; color: #2c3e50; }");

    // ---- 页签 1：结算工单明细 ----
    {
        QWidget *page = new QWidget;
        QVBoxLayout *l = new QVBoxLayout(page);
        l->setContentsMargins(6, 6, 6, 6);

        m_settleCount = new QLabel;
        m_settleCount->setStyleSheet("color: #7f8c8d; font-size: 13px;");
        l->addWidget(m_settleCount);

        m_settleTable = new QTableView;
        m_settleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_settleTable->setAlternatingRowColors(true);
        m_settleTable->horizontalHeader()->setStretchLastSection(true);
        m_settleTable->verticalHeader()->setVisible(false);
        m_settleTable->setStyleSheet(
            "QTableView { border: 1px solid #dcdde1; }"
            "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; font-weight: bold; }");
        l->addWidget(m_settleTable, 1);

        m_settleModel = new RemoteModel(this);
        m_settleTable->setModel(m_settleModel);

        connect(m_settleTable, &QTableView::doubleClicked, this, &BusinessReportPage::onSettleDoubleClicked);

        m_tabWidget->addTab(page, "结算工单明细");
    }

    // ---- 页签 2：报修工单明细 ----
    {
        QWidget *page = new QWidget;
        QVBoxLayout *l = new QVBoxLayout(page);
        l->setContentsMargins(6, 6, 6, 6);

        m_repairCount = new QLabel;
        m_repairCount->setStyleSheet("color: #7f8c8d; font-size: 13px;");
        l->addWidget(m_repairCount);

        m_repairTable = new QTableView;
        m_repairTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_repairTable->setAlternatingRowColors(true);
        m_repairTable->horizontalHeader()->setStretchLastSection(true);
        m_repairTable->verticalHeader()->setVisible(false);
        m_repairTable->setStyleSheet(
            "QTableView { border: 1px solid #dcdde1; }"
            "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; font-weight: bold; }");
        l->addWidget(m_repairTable, 1);

        m_repairModel = new RemoteModel(this);
        m_repairTable->setModel(m_repairModel);

        connect(m_repairTable, &QTableView::doubleClicked, this, &BusinessReportPage::onRepairDoubleClicked);

        m_tabWidget->addTab(page, "报修工单明细");
    }

    mainLayout->addWidget(m_tabWidget, 1);

    // 时间范围变化 → 两个页签都刷新；页签切换 → 刷新当前页
    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &BusinessReportPage::onDateRangeChanged);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &BusinessReportPage::onTabChanged);
    connect(m_btnExport, &QPushButton::clicked, this, &BusinessReportPage::onExport);

    refreshData();
}

void BusinessReportPage::onExport()
{
    const bool isSettle = (m_tabWidget->currentIndex() == 0);
    RemoteModel *model = isSettle ? m_settleModel : m_repairModel;
    const QString sheetName = isSettle ? "结算工单明细" : "报修工单明细";

    if (model->rowCount() == 0) {
        QMessageBox::information(this, "提示", QString("当前页签「%1」没有可导出的数据").arg(sheetName));
        return;
    }

    QString defaultName = QString("%1_%2_%3.xlsx")
        .arg(sheetName)
        .arg(m_dateRange->startDate().toString("yyyyMMdd"))
        .arg(m_dateRange->endDate().toString("yyyyMMdd"));
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出Excel", defaultName, "Excel 文件 (*.xlsx)");
    if (filePath.isEmpty())
        return;

    QString err = XlsxExporter::writeModel(filePath, model, sheetName);
    if (err.isEmpty())
        QMessageBox::information(this, "导出成功", QString("%1已导出到：\n%2").arg(sheetName, filePath));
    else
        QMessageBox::warning(this, "导出失败", err);
}

void BusinessReportPage::refreshData()
{
    refreshSettle();
    refreshRepair();
}

void BusinessReportPage::refreshActive()
{
    if (m_tabWidget->currentIndex() == 0)
        refreshSettle();
    else
        refreshRepair();
}

void BusinessReportPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshData();
}

void BusinessReportPage::onTabChanged(int index)
{
    Q_UNUSED(index)
    refreshActive();
}

// ============================================================
// 结算工单明细查询
// ============================================================
void BusinessReportPage::refreshSettle()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    // TODO: 结算工单明细需要显示的列待补充，以下为占位查询（按结算时间过滤）
    RemoteQuery query;
    query.prepare(
        "SELECT w.order_no AS '工单号', v.owner_name AS '车主', v.owner_phone AS '车主电话', "
        "  v.plate_number AS '车牌号', "
        "  s.total_amount AS '结算金额', s.settled_at AS '结算时间', w.status AS '状态' "
        "FROM t_settlement s "
        "LEFT JOIN t_workorder w ON w.id = s.workorder_id "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE DATE(s.settled_at) BETWEEN :start AND :end "
        "ORDER BY s.settled_at DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    query.exec();
    m_settleModel->setQuery(query);

    // 统计查询范围内结算总金额
    RemoteQuery sumQ;
    sumQ.prepare("SELECT COALESCE(SUM(total_amount),0) FROM t_settlement "
                 "WHERE DATE(settled_at) BETWEEN :start AND :end");
    sumQ.bindValue(":start", start.toString("yyyy-MM-dd"));
    sumQ.bindValue(":end", end.toString("yyyy-MM-dd"));
    sumQ.exec();
    double settleTotal = 0;
    if (sumQ.next())
        settleTotal = sumQ.value(0).toDouble();

    m_settleCount->setText(QString("共 %1 条结算工单明细 | 结算总金额: ¥%2")
                           .arg(m_settleModel->rowCount()).arg(settleTotal, 0, 'f', 2));
}

// ============================================================
// 报修工单明细查询
// ============================================================
void BusinessReportPage::refreshRepair()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    // TODO: 报修工单明细需要显示的列待补充，以下为占位查询（按报修日期过滤）
    RemoteQuery query;
    query.prepare(
        "SELECT w.order_no AS '工单号', v.owner_name AS '车主', v.owner_phone AS '车主电话', "
        "  v.plate_number AS '车牌号', "
        "  w.repair_content AS '报修内容', w.repair_date AS '报修日期', w.status AS '状态' "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE DATE(w.repair_date) BETWEEN :start AND :end "
        "ORDER BY w.repair_date DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    query.exec();
    m_repairModel->setQuery(query);
    m_repairCount->setText(QString("共 %1 条报修工单明细").arg(m_repairModel->rowCount()));
}

// ============================================================
// 双击行 → 打开工单详情弹窗（含打印/PDF）
// ============================================================
void BusinessReportPage::onSettleDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString orderNo = m_settleModel->data(m_settleModel->index(index.row(), 0)).toString().trimmed();
    if (orderNo.isEmpty()) return;
    WorkOrderDetailDialog dlg(orderNo, this);
    dlg.exec();
}

void BusinessReportPage::onRepairDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString orderNo = m_repairModel->data(m_repairModel->index(index.row(), 0)).toString().trimmed();
    if (orderNo.isEmpty()) return;
    WorkOrderDetailDialog dlg(orderNo, this);
    dlg.exec();
}
