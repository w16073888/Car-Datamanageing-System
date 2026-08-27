#ifndef BUSINESSREPORTPAGE_H
#define BUSINESSREPORTPAGE_H

#include <QWidget>
#include <QTableView>
#include <QLabel>
#include <QTabWidget>
#include "widgets/DateRangeWidget.h"
#include "remote/RemoteModel.h"

class QPushButton;

// ============================================================
// 业务流水 — 通过页签切换两种明细查询：
//   1. 结算工单明细
//   2. 报修工单明细
// 两个查询共用顶部 DateRangeWidget 选择时间范围。
// 各页签具体显示列待补充（见 refreshSettle / refreshRepair 中的 TODO）。
// ============================================================
class BusinessReportPage : public QWidget
{
    Q_OBJECT

public:
    explicit BusinessReportPage(QWidget *parent = nullptr);
    ~BusinessReportPage();
    void refreshData();          // 同时刷新两个页签（页面打开时调用）

private slots:
    void onDateRangeChanged(const QDate &start, const QDate &end);
    void onTabChanged(int index);
    void onExport();                                        // 导出当前页签明细为 Excel
    void onSettleDoubleClicked(const QModelIndex &index);   // 双击结算明细行 → 工单详情弹窗
    void onRepairDoubleClicked(const QModelIndex &index);   // 双击报修明细行 → 工单详情弹窗

private:
    void setupUI();
    void refreshActive();        // 只刷新当前页签
    void refreshSettle();        // 结算工单明细查询（列待补充）
    void refreshRepair();        // 报修工单明细查询（列待补充）

    DateRangeWidget *m_dateRange;   // 共用的时间范围选择

    QTabWidget *m_tabWidget;        // 页签容器
    QTableView *m_settleTable;      // 结算工单明细表格
    QTableView *m_repairTable;      // 报修工单明细表格
    RemoteModel *m_settleModel;
    RemoteModel *m_repairModel;
    QLabel *m_settleCount;          // 结算明细条数
    QLabel *m_repairCount;          // 报修明细条数
    QPushButton *m_btnExport;       // 导出当前页签明细
};

#endif // BUSINESSREPORTPAGE_H
