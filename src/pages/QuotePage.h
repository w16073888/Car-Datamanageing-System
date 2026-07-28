#ifndef QUOTEPAGE_H
#define QUOTEPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QTextEdit>
#include <QTableWidget>

class QuotePage : public QWidget
{
    Q_OBJECT

public:
    explicit QuotePage(QWidget *parent = nullptr);
    ~QuotePage();
    void refreshData();

private slots:
    void onOrderSearch();
    void onNotifyBilling();     // 通知提单: 已派工 → 待提单
    void onSettle();            // 结算: 已提单 → 已结算
    void onSaveToPdf();         // 保存结算单到PDF
    void onPrintSettlement();   // 打印结算单

private:
    void setupUI();
    void loadOrderInfo(const QString &orderNo);
    void updateActionButtons(const QString &status);
    QString buildSettlementHtml() const;

    // ---- 查询工单 ----
    QLineEdit *m_searchOrder;
    QPushButton *m_btnSearch;

    // ---- 状态显示区域 ----
    QLabel *m_lblVehicleInfo;    // 车辆 + 车主信息
    QTextEdit *m_textPartsInfo;  // 实际使用备件信息
    QLabel *m_lblTotalPrice;     // 总价格（强调）

    // ---- 操作按钮 ----
    QPushButton *m_btnNotifyBilling;   // 通知提单（已派工时显示）
    QPushButton *m_btnSettle;          // 结算（已提单时显示）
    QPushButton *m_btnSavePdf;         // 保存到PDF（已提单时显示）
    QPushButton *m_btnPrint;           // 打印结算单（已提单时显示）

    // ---- 内部状态 ----
    int m_currentOrderId;
    QString m_currentOrderNo;
    QString m_currentStatus;
};

#endif // QUOTEPAGE_H
