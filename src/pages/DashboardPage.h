#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif

class DashboardPage : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget *parent = nullptr);
    ~DashboardPage();

private slots:
    void onPeriodChanged(int index);

private:
    void setupUI();
    void refreshDashboard();

    // 关键指标
    QLabel *m_lblRevenue;
    QLabel *m_lblOrderCount;
    QLabel *m_lblCustomerCount;

    // 周期选择
    QComboBox *m_cmbPeriod;
    QPushButton *m_btnRefresh;

    // 图表
    QChartView *m_chartView;
    QChart *m_chart;
};

#endif // DASHBOARDPAGE_H
