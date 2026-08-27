#ifndef DATAMANAGERPAGE_H
#define DATAMANAGERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include "widgets/DateRangeWidget.h"
#include "remote/RemoteModel.h"

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
    void onExport();

private:
    void setupUI();
    QString tableName() const;
    void setChineseHeaders();
    void loadVisitRecords();
    void loadSystemLog();           // 系统日志表：按时间段/操作类型只读查询（英文操作类型）
    void loadOperationLog();        // 操作日志表：与系统日志平行，仅将英文值译为中文（只读）
    void setupLogActionFilter(bool chinese); // 按当前日志表切换"操作类型"下拉文案
    void onLogFilterChanged();      // 日志筛选变化 → 按当前选中表重新加载
    bool eventFilter(QObject *obj, QEvent *event) override;

    QComboBox *m_tableSelector;
    QTableView *m_tableView;
    RemoteModel *m_model;           // 可编辑表模型（OnFieldChange）
    RemoteModel *m_queryModel;      // 回访记录查询视图（只读）
    QPushButton *m_btnRefresh;
    QPushButton *m_btnDelete;
    QPushButton *m_btnExport;
    QLabel *m_hintLabel;

    // 回访记录筛选
    QWidget *m_visitFilterRow;          // 筛选行容器（选择回访记录表时显示）
    DateRangeWidget *m_visitDateRange;  // 回访时间区间
    QComboBox *m_cmbVisitSatisfaction;  // 满意度筛选

    // 系统日志/操作日志筛选
    QWidget *m_logFilterRow;            // 筛选行容器（选择日志表时显示）
    DateRangeWidget *m_logDateRange;    // 操作时间区间
    QComboBox *m_cmbLogAction;          // 操作类型筛选（系统日志=insert/update/delete；操作日志=新增/修改/删除）
};

#endif // DATAMANAGERPAGE_H
