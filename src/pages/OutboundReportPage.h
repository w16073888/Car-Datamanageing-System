#ifndef OUTBOUNDREPORTPAGE_H
#define OUTBOUNDREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include "widgets/DateRangeWidget.h"

class OutboundReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit OutboundReportPage(QWidget *parent = nullptr);
    ~OutboundReportPage();

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);

private:
    void setupUI();
    void refreshData();

    DateRangeWidget *m_dateRange;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // OUTBOUNDREPORTPAGE_H
