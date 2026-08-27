#include "ServiceReminderPage.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QShowEvent>

ServiceReminderPage::ServiceReminderPage(QWidget *parent) : QWidget(parent) { setupUI(); }
ServiceReminderPage::~ServiceReminderPage() {}

void ServiceReminderPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    onScan();
}

void ServiceReminderPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("保养提醒");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QGroupBox *filterGroup = new QGroupBox("筛选条件");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterGroup);
    filterLayout->addWidget(new QLabel("距上次保养超过："));
    m_spinDays = new QSpinBox;
    m_spinDays->setRange(0, 3650);
    m_spinDays->setValue(180);
    m_spinDays->setSuffix(" 天");
    filterLayout->addWidget(m_spinDays);
    filterLayout->addWidget(new QLabel("或行驶超过："));
    m_spinMileage = new QSpinBox;
    m_spinMileage->setRange(0, 100000);
    m_spinMileage->setValue(5000);
    m_spinMileage->setSuffix(" 公里");
    filterLayout->addWidget(m_spinMileage);
    mainLayout->addWidget(filterGroup);

    m_resultCount = new QLabel("共 0 辆车需要保养提醒");
    m_resultCount->setStyleSheet("color: #7f8c8d; font-size: 13px;");
    mainLayout->addWidget(m_resultCount);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new RemoteModel(this);
    m_tableView->setModel(m_model);

    // 筛选条件变化 → 自动扫描
    connect(m_spinDays, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ServiceReminderPage::onScan);
    connect(m_spinMileage, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ServiceReminderPage::onScan);

    // 初始扫描
    onScan();
}

void ServiceReminderPage::onScan()
{
    int days = m_spinDays->value();
    int mileage = m_spinMileage->value();

    RemoteQuery query;
    query.prepare(
        "SELECT v.owner_name AS '车主', v.owner_phone AS '车主电话', v.plate_number AS '车牌号', "
        "  v.last_maintenance_date AS '上次保养日期', "
        "  v.last_maintenance_mileage AS '上次保养公里数', "
        "  DATEDIFF(CURDATE(), v.last_maintenance_date) AS '距今天数' "
        "FROM t_vehicle v "
        "WHERE (v.last_maintenance_date IS NOT NULL "
        "  AND DATEDIFF(CURDATE(), v.last_maintenance_date) > :days) "
        "   OR (v.last_maintenance_mileage IS NOT NULL "
        "  AND v.last_maintenance_mileage > 0) "
        "ORDER BY v.last_maintenance_date ASC"
    );
    query.bindValue(":days", days);
    query.bindValue(":mileage", mileage);
    query.exec();
    m_model->setQuery(query);
    m_resultCount->setText(QString("共 %1 辆车需要保养提醒").arg(m_model->rowCount()));
}