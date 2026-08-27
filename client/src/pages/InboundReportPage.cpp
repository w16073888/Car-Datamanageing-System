#include "InboundReportPage.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"
#include "utils/XlsxExporter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

InboundReportPage::InboundReportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
InboundReportPage::~InboundReportPage() {}

void InboundReportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    // 标题行：标题 + 右侧导出按钮
    QHBoxLayout *titleRow = new QHBoxLayout;
    QLabel *title = new QLabel("入库报表");
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

    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &InboundReportPage::onDateRangeChanged);
    connect(m_btnExport, &QPushButton::clicked, this, &InboundReportPage::onExport);
    refreshData();
}

void InboundReportPage::onExport()
{
    if (m_model->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前没有可导出的入库记录");
        return;
    }

    QString defaultName = QString("入库报表_%1_%2.xlsx")
        .arg(m_dateRange->startDate().toString("yyyyMMdd"))
        .arg(m_dateRange->endDate().toString("yyyyMMdd"));
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出Excel", defaultName, "Excel 文件 (*.xlsx)");
    if (filePath.isEmpty())
        return;

    QString err = XlsxExporter::writeModel(filePath, m_model, "入库报表");
    if (err.isEmpty())
        QMessageBox::information(this, "导出成功", QString("入库报表已导出到：\n%1").arg(filePath));
    else
        QMessageBox::warning(this, "导出失败", err);
}

void InboundReportPage::refreshData()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    RemoteQuery query;
    query.prepare(
        "SELECT p.part_no, p.name, p.spec, p.supplier, p.applicable_model, "
        "  il.quantity, il.unit_price, il.created_at "
        "FROM t_inventory_log il "
        "LEFT JOIN t_parts p ON p.id = il.part_id "
        "WHERE il.operation_type = '采购入库' "
        "AND DATE(il.created_at) BETWEEN :start AND :end "
        "ORDER BY il.created_at DESC, il.id DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    query.exec();

    // 相邻且 备件编号/名称/型号/供应商 完全一致的条目 → 合并，数量叠加。
    // 排序带 il.id 二级键：同批同秒写入的流水按插入顺序排列，同种备件才连续。
    // 结果按 created_at DESC，组内第一条（最新）的进货价与入库时间作为合并行取值。
    QStringList cols = {"备件编号", "备件名称", "型号", "供应商", "适用车型", "入库数量", "进货价", "入库时间"};
    QList<QVariantList> merged;

    int i = 0;
    while (i < query.rowCount()) {
        query.seek(i);
        const QString pn = query.value(0).toString();
        const QString nm = query.value(1).toString();
        const QString sp = query.value(2).toString();
        const QString su = query.value(3).toString();
        const QString am = query.value(4).toString();
        int qty = query.value(5).toInt();
        double price = query.value(6).toDouble();
        const QString time = query.value(7).toString();

        int j = i + 1;
        while (j < query.rowCount()) {
            query.seek(j);
            if (query.value(0).toString() == pn
                && query.value(1).toString() == nm
                && query.value(2).toString() == sp
                && query.value(3).toString() == su
                && query.value(4).toString() == am) {
                qty += query.value(5).toInt();
                j++;
            } else {
                break;
            }
        }

        merged.append(QVariantList{pn, nm, sp, su, am, qty, price, time});
        i = j;
    }

    m_model->setRows(cols, merged);
    m_resultCount->setText(QString("共 %1 条入库记录（已合并 %2 条相邻同备件条目）")
                           .arg(merged.size()).arg(query.rowCount() - merged.size()));
}

void InboundReportPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshData();
}
