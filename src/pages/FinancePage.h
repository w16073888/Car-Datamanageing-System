#ifndef FINANCEPAGE_H
#define FINANCEPAGE_H

#include <QWidget>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include "widgets/DateRangeWidget.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif

class FinancePage : public QWidget
{
    Q_OBJECT

public:
    explicit FinancePage(QWidget *parent = nullptr);
    ~FinancePage();

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onTabChanged(int index);

private:
    void setupUI();
    void refreshIncome();
    void refreshCost();
    void refreshProfit();
    void drawChart();

    DateRangeWidget *m_dateRange;

    // 收入统计
    QLabel *m_lblIncomeCount;
    QLabel *m_lblIncomeLabor;
    QLabel *m_lblIncomeMaterial;
    QLabel *m_lblIncomeTotal;

    // 成本统计
    QLabel *m_lblCostTotal;

    // 利润
    QLabel *m_lblProfitTotal;
    QLabel *m_lblProfitRate;

    // 图表
    QChartView *m_chartView;
    QChart *m_chart;
};

#endif // FINANCEPAGE_H
