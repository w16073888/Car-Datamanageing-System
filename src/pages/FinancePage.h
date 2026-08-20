#ifndef FINANCEPAGE_H
#define FINANCEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QTabWidget>
#include <QScrollArea>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
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

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onTabChanged(int index);

private:
    void setupUI();
    void refreshIncome();
    void refreshProfit();
    void drawChart();

    QTabWidget *m_tabWidget;
    DateRangeWidget *m_dateRange;

    // 图表（与构造函数初始化列表顺序一致）
    QChartView *m_chartView;
    QChart *m_chart;

    // ======================== 收入统计 ========================
    // 四项收入指标
    QLabel *m_lblIncomeLabor;       // 工时费收入
    QLabel *m_lblIncomeMaterial;    // 材料费收入
    QLabel *m_lblIncomeOther;       // 其它收入（其它费+管理费）
    QLabel *m_lblIncomeTotal;       // 总收入
    QLabel *m_lblIncomeOrderCount;  // 结算工单数
    QLabel *m_lblIncomeCost;        // 进货成本
    QLabel *m_lblIncomeProfit;      // 总利润

    // ======================== 利润分析 ========================
    QLabel *m_lblProfitIncome;      // 总收入
    QLabel *m_lblProfitCost;        // 总成本
    QLabel *m_lblProfitNet;         // 净利润
    QLabel *m_lblProfitRate;        // 利润率
};

#endif // FINANCEPAGE_H
