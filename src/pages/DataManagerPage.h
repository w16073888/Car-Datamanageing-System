#ifndef DATAMANAGERPAGE_H
#define DATAMANAGERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlQueryModel>
#include <QPushButton>
#include <QLabel>
#include "widgets/DateRangeWidget.h"

class DataManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit DataManagerPage(QWidget *parent = nullptr);
    ~DataManagerPage();
    void refreshData();

private slots:
    void onTableSelected(int index);
    void onCellChanged(int row, int column);
    void onRefresh();
    void onDelete();

private:
    void setupUI();
    QString tableName() const;
    void setChineseHeaders();
    void loadVisitRecords();
    bool eventFilter(QObject *obj, QEvent *event) override;

    QComboBox *m_tableSelector;
    QTableView *m_tableView;
    QSqlTableModel *m_model;
    QSqlQueryModel *m_queryModel;   // 回访记录查询视图（只读）
    QPushButton *m_btnRefresh;
    QPushButton *m_btnDelete;
    QLabel *m_hintLabel;

    // 回访记录筛选
    QWidget *m_visitFilterRow;          // 筛选行容器（选择回访记录表时显示）
    DateRangeWidget *m_visitDateRange;  // 回访时间区间
    QComboBox *m_cmbVisitSatisfaction;  // 满意度筛选
};

#endif // DATAMANAGERPAGE_H
