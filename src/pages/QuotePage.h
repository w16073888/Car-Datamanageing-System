#ifndef QUOTEPAGE_H
#define QUOTEPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QTableWidget>
#include <QDoubleSpinBox>

class QuotePage : public QWidget
{
    Q_OBJECT

public:
    explicit QuotePage(QWidget *parent = nullptr);
    ~QuotePage();
    void refreshData();
    static QString buildSettlementHtmlFor(int orderId);  // 静态构建结算单HTML：供工单详情弹窗复用

private slots:
    void onOrderSearch();
    void onNotifyBilling();     // 通知提单: 已派工 → 待提单
    void onCancelNotify();      // 取消提单: 待提单 → 已派工
    void onSettle();            // 结算: 已提单 → 已结算
    void onSaveToPdf();         // 保存结算单到PDF
    void onPrintSettlement();   // 打印结算单
    void onFeeEditChanged();    // 其他费/管理费编辑时刷新
    void onSaveEdit();          // 保存修改到数据库

private:
    void setupUI();
    void loadOrderInfo(const QString &orderNo);
    void updateActionButtons(const QString &status);
    QString buildSettlementHtml() const;

    // ---- 查询工单 ----
    QLineEdit *m_searchOrder;
    QPushButton *m_btnSearch;

    // ---- 状态显示区域 ----
    QLabel *m_lblVehicleInfo;       // 车辆 + 车主信息
    QTableWidget *m_laborTable;     // 工时费明细表
    QTableWidget *m_partsTable;     // 材料明细表
    QTableWidget *m_summaryTable;   // 费用总计表

    // ---- 操作按钮 ----
    QPushButton *m_btnNotifyBilling;   // 通知提单（已派工时显示）
    QPushButton *m_btnCancelNotify;    // 取消提单（待提单时显示）
    QPushButton *m_btnSettle;          // 结算（已提单时显示）
    QPushButton *m_btnSavePdf;         // 保存到PDF（已提单时显示）
    QPushButton *m_btnPrint;           // 打印结算单（已提单时显示）
    QPushButton *m_btnSaveEdit;        // 保存修改（可编辑状态下显示）

    // ---- 费用编辑控件 ----
    QDoubleSpinBox *m_editOtherFee;    // 其他费编辑
    QDoubleSpinBox *m_editMgmtFee;     // 管理费编辑
    QLabel *m_lblEditLaborFee;         // 工时费合计显示（编辑模式下由表格直接改）

    // ---- 内部状态 ----
    int m_currentOrderId;
    QString m_currentOrderNo;
    QString m_currentStatus;
};

#endif // QUOTEPAGE_H
