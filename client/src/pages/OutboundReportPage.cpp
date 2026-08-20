#include "OutboundReportPage.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"
#include "utils/XlsxExporter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

OutboundReportPage::OutboundReportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
OutboundReportPage::~OutboundReportPage() {}

void OutboundReportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    // 标题行：标题 + 右侧导出按钮
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *title = new QLabel("出库报表");
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

    m_dateRange = new DateRangeWidget;
    mainLayout->addWidget(m_dateRange);

    m_resultCount = new QLabel;
    m_resultCount->setStyleSheet("color: #7f8c8d; font-size: 13px;");
    mainLayout->addWidget(m_resultCount);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; font-weight: bold; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new RemoteModel(this);
    m_tableView->setModel(m_model);

    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &OutboundReportPage::onDateRangeChanged);
    connect(m_btnExport, &QPushButton::clicked, this, &OutboundReportPage::onExport);
    refreshData();
}

void OutboundReportPage::refreshData()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    RemoteQuery query;
    query.prepare(
        "SELECT p.part_no AS '备件编号', p.name AS '备件名称', "
        "  ABS(il.quantity) AS '出库数量', il.unit_price AS '售价', "
        "  il.recipient AS '领取人', "
        "  COALESCE(v.plate_number, '') AS '绑定车辆', il.ref_order_no AS '关联工单', "
        "  il.created_at AS '出库时间' "
        "FROM t_inventory_log il "
        "LEFT JOIN t_parts p ON p.id = il.part_id "
        "LEFT JOIN t_workorder w ON w.order_no = il.ref_order_no "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE il.operation_type = '维修出库' "
        "AND DATE(il.created_at) BETWEEN :start AND :end "
        "ORDER BY il.created_at DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    query.exec();
    m_model->setQuery(query);
    m_tableView->resizeColumnsToContents();
    // 关联工单列宽改为自动宽度（内容+表头）的 2 倍
    const int woCol = m_model->fieldIndex("关联工单");
    if (woCol >= 0)
        m_tableView->setColumnWidth(woCol, m_tableView->columnWidth(woCol) * 2);
    m_resultCount->setText(QString("共 %1 条出库记录").arg(m_model->rowCount()));
}

void OutboundReportPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshData();
}

void OutboundReportPage::onExport()
{
    if (m_model->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的出库记录");
        return;
    }

    QString defaultName = QString("出库报表_%1_%2.xlsx")
        .arg(m_dateRange->startDate().toString("yyyyMMdd"))
        .arg(m_dateRange->endDate().toString("yyyyMMdd"));
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出Excel", defaultName, "Excel 文件 (*.xlsx)");
    if (filePath.isEmpty())
        return;

    QString err = XlsxExporter::writeModel(filePath, m_model, "出库报表");
    if (err.isEmpty())
        QMessageBox::information(this, "导出成功", QString("出库报表已导出到：\n%1").arg(filePath));
    else
        QMessageBox::warning(this, "导出失败", err);
}
