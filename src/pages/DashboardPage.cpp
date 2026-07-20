#include "DashboardPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QTime>

DashboardPage::DashboardPage(QWidget *parent) : QWidget(parent), m_chartView(nullptr), m_chart(nullptr)
{
    setupUI();
}

DashboardPage::~DashboardPage() {}

void DashboardPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("经营看板");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 关键指标
    QGroupBox *kpiGroup = new QGroupBox("关键指标");
    QGridLayout *kpiGrid = new QGridLayout(kpiGroup);
    kpiGrid->addWidget(new QLabel("总营业额："), 0, 0);
    m_lblRevenue = new QLabel("¥0.00");
    m_lblRevenue->setStyleSheet("font-size: 22px; font-weight: bold; color: #e74c3c;");
    kpiGrid->addWidget(m_lblRevenue, 0, 1);
    kpiGrid->addWidget(new QLabel("总工单数："), 0, 2);
    m_lblOrderCount = new QLabel("0");
    m_lblOrderCount->setStyleSheet("font-size: 22px; font-weight: bold; color: #3498db;");
    kpiGrid->addWidget(m_lblOrderCount, 0, 3);
    kpiGrid->addWidget(new QLabel("客户数："), 0, 4);
    m_lblCustomerCount = new QLabel("0");
    m_lblCustomerCount->setStyleSheet("font-size: 22px; font-weight: bold; color: #27ae60;");
    kpiGrid->addWidget(m_lblCustomerCount, 0, 5);
    mainLayout->addWidget(kpiGroup);

    // 周期和刷新
    QHBoxLayout *ctrlLayout = new QHBoxLayout;
    ctrlLayout->addWidget(new QLabel("统计周期："));
    m_cmbPeriod = new QComboBox;
    m_cmbPeriod->addItems({"近7天", "近30天", "近90天"});
    ctrlLayout->addWidget(m_cmbPeriod);
    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    ctrlLayout->addWidget(m_btnRefresh);
    ctrlLayout->addStretch();
    mainLayout->addLayout(ctrlLayout);

    // 图表
    m_chart = new QChart();
    m_chart->setTitle("收入趋势");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(m_chartView, 1);

    connect(m_cmbPeriod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DashboardPage::onPeriodChanged);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DashboardPage::refreshDashboard);

    refreshDashboard();
}

void DashboardPage::onPeriodChanged(int index)
{
    Q_UNUSED(index)
    refreshDashboard();
}

void DashboardPage::refreshDashboard()
{
    int days = 7;
    if (m_cmbPeriod->currentIndex() == 1) days = 30;
    else if (m_cmbPeriod->currentIndex() == 2) days = 90;

    QDate start = QDate::currentDate().addDays(-days);
    QDate end = QDate::currentDate();

    // 总营业额
    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_settlement "
                  "WHERE DATE(settled_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    if (query.next())
        m_lblRevenue->setText(QString("¥%1").arg(query.value(0).toDouble(), 0, 'f', 2));

    // 总工单数
    query.prepare("SELECT COUNT(*) FROM t_workorder "
                  "WHERE DATE(created_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    if (query.next()) m_lblOrderCount->setText(query.value(0).toString());

    // 客户数
    query.prepare("SELECT COUNT(DISTINCT vehicle_id) FROM t_workorder "
                  "WHERE DATE(created_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    if (query.next()) m_lblCustomerCount->setText(query.value(0).toString());

    // 绘制收入趋势折线图
    m_chart->removeAllSeries();
    const auto axes = m_chart->axes();
    for (auto *axis : axes) m_chart->removeAxis(axis);

    QLineSeries *series = new QLineSeries();
    series->setName("每日收入");

    double maxVal = 0;
    for (int i = 0; i < days; i++) {
        QDate d = start.addDays(i);
        query.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_settlement "
                      "WHERE DATE(settled_at) = :d");
        query.bindValue(":d", d.toString("yyyy-MM-dd"));
        DbManager::instance().executeQuery(query);
        double val = 0;
        if (query.next()) val = query.value(0).toDouble();
        QDateTime dt(d, QTime(0, 0));
        series->append(dt.toMSecsSinceEpoch(), val);
        if (val > maxVal) maxVal = val;
    }

    m_chart->addSeries(series);

    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("MM-dd");
    axisX->setTitleText("日期");
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("收入 (¥)");
    axisY->setRange(0, maxVal * 1.2 > 0 ? maxVal * 1.2 : 100);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}
