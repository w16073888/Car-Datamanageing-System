#include "SettlementQueryPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>

SettlementQueryPage::SettlementQueryPage(QWidget *parent) : QWidget(parent) { setupUI(); }
SettlementQueryPage::~SettlementQueryPage() {}

void SettlementQueryPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("结算查询");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QHBoxLayout *topLayout = new QHBoxLayout;
    m_dateRange = new DateRangeWidget;
    topLayout->addWidget(m_dateRange);

    topLayout->addWidget(new QLabel("工单号："));
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("可选");
    m_editOrderNo->setFixedWidth(180);
    topLayout->addWidget(m_editOrderNo);

    m_btnQuery = new QPushButton("查询");
    m_btnQuery->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    topLayout->addWidget(m_btnQuery);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

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

    connect(m_btnQuery, &QPushButton::clicked, this, &SettlementQueryPage::onQuery);
    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &SettlementQueryPage::onDateRangeChanged);

    onQuery();
}

void SettlementQueryPage::onQuery()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();
    QString orderNo = m_editOrderNo->text().trimmed();

    QString sql;
    if (orderNo.isEmpty()) {
        sql = "SELECT s.id, w.order_no AS '工单号', v.plate_number AS '车牌号', "
              "  s.labor_fee AS '工时费', s.material_fee AS '材料费', "
              "  s.total_amount AS '总金额', e.name AS '结算人', s.settled_at AS '结算时间' "
              "FROM t_settlement s "
              "LEFT JOIN t_workorder w ON w.id = s.workorder_id "
              "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
              "LEFT JOIN t_employee e ON e.id = s.settled_by "
              "WHERE DATE(s.settled_at) BETWEEN :start AND :end "
              "ORDER BY s.settled_at DESC";
    } else {
        sql = "SELECT s.id, w.order_no AS '工单号', v.plate_number AS '车牌号', "
              "  s.labor_fee AS '工时费', s.material_fee AS '材料费', "
              "  s.total_amount AS '总金额', e.name AS '结算人', s.settled_at AS '结算时间' "
              "FROM t_settlement s "
              "LEFT JOIN t_workorder w ON w.id = s.workorder_id "
              "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
              "LEFT JOIN t_employee e ON e.id = s.settled_by "
              "WHERE DATE(s.settled_at) BETWEEN :start AND :end "
              "AND w.order_no LIKE :no "
              "ORDER BY s.settled_at DESC";
    }

    QSqlQuery query(DbManager::instance().database());
    query.prepare(sql);
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    if (!orderNo.isEmpty()) query.bindValue(":no", "%" + orderNo + "%");
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条结算记录").arg(m_model->rowCount()));
}

void SettlementQueryPage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    onQuery();
}
