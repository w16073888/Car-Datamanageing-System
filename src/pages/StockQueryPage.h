#ifndef STOCKQUERYPAGE_H
#define STOCKQUERYPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>

class StockQueryPage : public QWidget
{
    Q_OBJECT

public:
    explicit StockQueryPage(QWidget *parent = nullptr);
    ~StockQueryPage();
    void refreshData();

private slots:
    void onSearch();
    void onRefresh();

private:
    void setupUI();

    QLineEdit *m_searchInput;
    QPushButton *m_btnSearch;
    QPushButton *m_btnRefresh;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // STOCKQUERYPAGE_H
