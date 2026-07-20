#ifndef BUSINESSREPORTPAGE_H
#define BUSINESSREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include "widgets/DateRangeWidget.h"

class BusinessReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit BusinessReportPage(QWidget *parent = nullptr);
    ~BusinessReportPage();

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onRefresh();

private:
    void setupUI();
    void refreshData();

    DateRangeWidget *m_dateRange;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // BUSINESSREPORTPAGE_H
