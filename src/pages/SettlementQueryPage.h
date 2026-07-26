#ifndef SETTLEMENTQUERYPAGE_H
#define SETTLEMENTQUERYPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include "widgets/DateRangeWidget.h"

class SettlementQueryPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettlementQueryPage(QWidget *parent = nullptr);
    ~SettlementQueryPage();
    void refreshData();

private slots:
    void onQuery();
    void onDateRangeChanged(const QDate &start, const QDate &end);

private:
    void setupUI();

    DateRangeWidget *m_dateRange;
    QLineEdit *m_editOrderNo;
    QPushButton *m_btnQuery;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // SETTLEMENTQUERYPAGE_H
