#ifndef OUTBOUNDREPORTPAGE_H
#define OUTBOUNDREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QLabel>
#include "widgets/DateRangeWidget.h"
#include "remote/RemoteModel.h"

class QPushButton;

class OutboundReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit OutboundReportPage(QWidget *parent = nullptr);
    ~OutboundReportPage();
    void refreshData();          // 每次打开页面时由主窗口调用，保证数据最新

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onExport();

private:
    void setupUI();

    DateRangeWidget *m_dateRange;
    QTableView *m_tableView;
    RemoteModel *m_model;
    QLabel *m_resultCount;
    QPushButton *m_btnExport;
};

#endif // OUTBOUNDREPORTPAGE_H
