#ifndef INBOUNDREPORTPAGE_H
#define INBOUNDREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QLabel>
#include "widgets/DateRangeWidget.h"
#include "remote/RemoteModel.h"

class QPushButton;

class InboundReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit InboundReportPage(QWidget *parent = nullptr);
    ~InboundReportPage();

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onExport();

private:
    void setupUI();
    void refreshData();

    DateRangeWidget *m_dateRange;
    QTableView *m_tableView;
    RemoteModel *m_model;
    QLabel *m_resultCount;
    QPushButton *m_btnExport;
};

#endif // INBOUNDREPORTPAGE_H
