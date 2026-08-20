#include "InboundReportPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>

InboundReportPage::InboundReportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
InboundReportPage::~InboundReportPage() {}

void InboundReportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("入库报表");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

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

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &InboundReportPage::onDateRangeChanged);
    refreshData();
}

void InboundReportPage::refreshData()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT p.part_no AS '备件编号', p.name AS '备件名称', "
        "  il.quantity AS '入库数量', il.unit_price AS '进货价', "
        "  p.supplier AS '供应商', il.created_at AS '入库时间' "
        "FROM t_inventory_log il "
        "LEFT JOIN t_parts p ON p.id = il.part_id "
        "WHERE il.operation_type = '采购入库' "
        "AND DATE(il.created_at) BETWEEN :start AND :end "
        "ORDER BY il.created_at DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条入库记录").arg(m_model->rowCount()));
}

void InboundReportPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshData();
}
