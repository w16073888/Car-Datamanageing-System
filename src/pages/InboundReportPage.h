#ifndef INBOUNDREPORTPAGE_H
#define INBOUNDREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include "widgets/DateRangeWidget.h"

class InboundReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit InboundReportPage(QWidget *parent = nullptr);
    ~InboundReportPage();

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

#endif // INBOUNDREPORTPAGE_H
