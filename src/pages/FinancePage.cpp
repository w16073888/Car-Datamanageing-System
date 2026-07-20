#include "FinancePage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTabWidget>

FinancePage::FinancePage(QWidget *parent) : QWidget(parent), m_chartView(nullptr), m_chart(nullptr)
{
    setupUI();
}

FinancePage::~FinancePage() {}

void FinancePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("财务管理");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 时间段选择
    m_dateRange = new DateRangeWidget;
    mainLayout->addWidget(m_dateRange);

    // Tab切换
    QTabWidget *tabWidget = new QTabWidget;

    // === 收入统计页 ===
    QWidget *incomeTab = new QWidget;
    QVBoxLayout *incomeLayout = new QVBoxLayout(incomeTab);
    QGroupBox *incomeGroup = new QGroupBox("收入统计");
    QGridLayout *incomeGrid = new QGridLayout(incomeGroup);
    incomeGrid->addWidget(new QLabel("结算工单数："), 0, 0);
    m_lblIncomeCount = new QLabel("0");
    m_lblIncomeCount->setStyleSheet("font-size: 20px; font-weight: bold; color: #3498db;");
    incomeGrid->addWidget(m_lblIncomeCount, 0, 1);
    incomeGrid->addWidget(new QLabel("总工时费："), 0, 2);
    m_lblIncomeLabor = new QLabel("¥0.00");
    m_lblIncomeLabor->setStyleSheet("font-size: 20px; font-weight: bold; color: #27ae60;");
    incomeGrid->addWidget(m_lblIncomeLabor, 0, 3);
    incomeGrid->addWidget(new QLabel("总材料费："), 1, 0);
    m_lblIncomeMaterial = new QLabel("¥0.00");
    m_lblIncomeMaterial->setStyleSheet("font-size: 20px; font-weight: bold; color: #e67e22;");
    incomeGrid->addWidget(m_lblIncomeMaterial, 1, 1);
    incomeGrid->addWidget(new QLabel("总收入："), 1, 2);
    m_lblIncomeTotal = new QLabel("¥0.00");
    m_lblIncomeTotal->setStyleSheet("font-size: 24px; font-weight: bold; color: #e74c3c;");
    incomeGrid->addWidget(m_lblIncomeTotal, 1, 3);
    incomeLayout->addWidget(incomeGroup);
    incomeLayout->addStretch();
    tabWidget->addTab(incomeTab, "收入统计");

    // === 成本统计页 ===
    QWidget *costTab = new QWidget;
    QVBoxLayout *costLayout = new QVBoxLayout(costTab);
    QGroupBox *costGroup = new QGroupBox("成本统计");
    QGridLayout *costGrid = new QGridLayout(costGroup);
    costGrid->addWidget(new QLabel("出库总成本（按进价）："), 0, 0);
    m_lblCostTotal = new QLabel("¥0.00");
    m_lblCostTotal->setStyleSheet("font-size: 24px; font-weight: bold; color: #e74c3c;");
    costGrid->addWidget(m_lblCostTotal, 0, 1);
    costLayout->addWidget(costGroup);
    costLayout->addStretch();
    tabWidget->addTab(costTab, "成本统计");

    // === 利润分析页 ===
    QWidget *profitTab = new QWidget;
    QVBoxLayout *profitLayout = new QVBoxLayout(profitTab);
    QGroupBox *profitGroup = new QGroupBox("利润分析");
    QGridLayout *profitGrid = new QGridLayout(profitGroup);
    profitGrid->addWidget(new QLabel("总收入："), 0, 0);
    m_lblProfitTotal = new QLabel("¥0.00");
    m_lblProfitTotal->setStyleSheet("font-size: 20px; font-weight: bold; color: #27ae60;");
    profitGrid->addWidget(m_lblProfitTotal, 0, 1);
    profitGrid->addWidget(new QLabel("利润率："), 0, 2);
    m_lblProfitRate = new QLabel("0%");
    m_lblProfitRate->setStyleSheet("font-size: 20px; font-weight: bold; color: #e67e22;");
    profitGrid->addWidget(m_lblProfitRate, 0, 3);
    profitLayout->addWidget(profitGroup);

    // 图表
    m_chart = new QChart();
    m_chart->setTitle("收入 vs 成本");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    profitLayout->addWidget(m_chartView, 1);
    tabWidget->addTab(profitTab, "利润分析");

    mainLayout->addWidget(tabWidget, 1);

    // 信号
    connect(m_dateRange, &DateRangeWidget::dateRangeChanged, this, &FinancePage::onDateRangeChanged);
    connect(tabWidget, &QTabWidget::currentChanged, this, &FinancePage::onTabChanged);

    // 初始加载
    refreshIncome();
    refreshCost();
    refreshProfit();
}

void FinancePage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    refreshIncome();
    refreshCost();
    refreshProfit();
}

void FinancePage::onTabChanged(int index)
{
    if (index == 2) drawChart();
}

void FinancePage::refreshIncome()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT COUNT(DISTINCT w.id), "
        "  COALESCE(SUM(s.labor_fee), 0), "
        "  COALESCE(SUM(s.material_fee), 0), "
        "  COALESCE(SUM(s.total_amount), 0) "
        "FROM t_settlement s "
        "LEFT JOIN t_workorder w ON w.id = s.workorder_id "
        "WHERE DATE(s.settled_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        m_lblIncomeCount->setText(query.value(0).toString());
        m_lblIncomeLabor->setText(QString("¥%1").arg(query.value(1).toDouble(), 0, 'f', 2));
        m_lblIncomeMaterial->setText(QString("¥%1").arg(query.value(2).toDouble(), 0, 'f', 2));
        m_lblIncomeTotal->setText(QString("¥%1").arg(query.value(3).toDouble(), 0, 'f', 2));
    }
}

void FinancePage::refreshCost()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT COALESCE(ABS(SUM(total_price)), 0) "
        "FROM t_inventory_log "
        "WHERE operation_type = '维修出库' "
        "AND DATE(created_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        m_lblCostTotal->setText(QString("¥%1").arg(query.value(0).toDouble(), 0, 'f', 2));
    }
}

void FinancePage::refreshProfit()
{
    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QSqlQuery query(DbManager::instance().database());

    // 总收入
    query.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_settlement "
                  "WHERE DATE(settled_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    double income = 0;
    if (query.next()) income = query.value(0).toDouble();

    // 总成本
    query.prepare("SELECT COALESCE(ABS(SUM(total_price)), 0) FROM t_inventory_log "
                  "WHERE operation_type = '维修出库' "
                  "AND DATE(created_at) BETWEEN :start AND :end");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    DbManager::instance().executeQuery(query);
    double cost = 0;
    if (query.next()) cost = query.value(0).toDouble();

    double profit = income - cost;
    double rate = income > 0 ? (profit / income) * 100 : 0;

    m_lblProfitTotal->setText(QString("¥%1").arg(profit, 0, 'f', 2));
    m_lblProfitRate->setText(QString("%1%").arg(rate, 0, 'f', 1));
}

void FinancePage::drawChart()
{
    m_chart->removeAllSeries();
    // 移除旧轴
    const auto axes = m_chart->axes();
    for (auto *axis : axes) m_chart->removeAxis(axis);

    QDate start = m_dateRange->startDate();
    QDate end = m_dateRange->endDate();

    QBarSet *setIncome = new QBarSet("收入");
    QBarSet *setCost = new QBarSet("成本");
    setIncome->setColor(QColor("#3498db"));
    setCost->setColor(QColor("#e74c3c"));

    QStringList categories;

    // 按月分组
    QDate d = start;
    while (d <= end) {
        QString monthKey = d.toString("yyyy-MM");
        categories << monthKey;

        QSqlQuery query(DbManager::instance().database());
        query.prepare("SELECT COALESCE(SUM(total_amount), 0) FROM t_settlement "
                      "WHERE DATE_FORMAT(settled_at, '%Y-%m') = :month");
        query.bindValue(":month", monthKey);
        DbManager::instance().executeQuery(query);
        double inc = query.next() ? query.value(0).toDouble() : 0;
        *setIncome << inc;

        query.prepare("SELECT COALESCE(ABS(SUM(total_price)), 0) FROM t_inventory_log "
                      "WHERE operation_type = '维修出库' "
                      "AND DATE_FORMAT(created_at, '%Y-%m') = :month");
        query.bindValue(":month", monthKey);
        DbManager::instance().executeQuery(query);
        double cst = query.next() ? query.value(0).toDouble() : 0;
        *setCost << cst;

        d = d.addMonths(1);
    }

    QBarSeries *series = new QBarSeries();
    series->append(setIncome);
    series->append(setCost);
    m_chart->addSeries(series);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("金额 (¥)");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}
