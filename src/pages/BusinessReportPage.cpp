#include "BusinessReportPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>

BusinessReportPage::BusinessReportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
BusinessReportPage::~BusinessReportPage() {}

void BusinessReportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("业务流水");
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

    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &BusinessReportPage::onDateRangeChanged);

    refreshData();
}

void BusinessReportPage::refreshData()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT w.order_no AS '工单号', v.plate_number AS '车牌号', "
        "  w.repair_content AS '维修内容', e.name AS '责任人', "
        "  w.labor_fee AS '工时费', w.total_amount AS '材料费', "
        "  w.total_amount AS '总金额', w.status AS '状态', w.created_at AS '创建时间' "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_employee e ON e.id = w.technician_id "
        "WHERE DATE(w.created_at) BETWEEN :start AND :end "
        "ORDER BY w.created_at DESC LIMIT 500");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条业务流水").arg(m_model->rowCount()));
}

void BusinessReportPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshData();
}

void BusinessReportPage::onRefresh()
{
    refreshData();
}
