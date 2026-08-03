#ifndef SETTLEMENTPAGE_H
#define SETTLEMENTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>

class SettlementPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettlementPage(QWidget *parent = nullptr);
    ~SettlementPage();
    void refreshData();

    // 明细数据结构（供打印函数使用）
    struct LaborItem {
        QString type;
        QString person;
        QString content;
        double fee;
    };
    struct PartItem {
        QString name;
        QString spec;
        int qty;
        double price;
    };

private slots:
    void onLoadOrder();
    void onNotifyWarehouse();
    void onSettle();
    void onPrintSettle();
    void onPrintQuote();

private:
    void setupUI();
    void loadOrderDetail();

    QLineEdit *m_editOrderNo;
    QPushButton *m_btnLoad;
    QPushButton *m_btnNotifyWH;
    QPushButton *m_btnSettle;
    QPushButton *m_btnPrintSettle;
    QPushButton *m_btnPrintQuote;
    QLabel *m_lblOrderInfo;
    QLabel *m_lblAdvisor;    // 服务顾问独立展示行

    // ===== 工单明细 =====
    // 备件明细表
    QLabel *m_lblPartsTitle;
    QTableWidget *m_partsTable;
    // 工时费明细表
    QLabel *m_lblLaborTitle;
    QTableWidget *m_laborTable;
    // 其他费用
    QLabel *m_lblOtherFee;
    QLabel *m_lblManagementFee;
    QLabel *m_lblDeposit;
    QLabel *m_lblTotal;

    // 缓存数据（用于打印）
    QString m_cachedOrderNo;
    QString m_cachedPlate;
    QString m_cachedModel;
    QString m_cachedOwner;
    QString m_cachedPhone;
    QString m_cachedAdvisor;
    int m_cachedMileage;
    QString m_cachedRepairDate;
    QString m_cachedTechnicians;
    QString m_cachedStatus;
    double m_cachedLaborFee;
    double m_cachedMatFee;
    double m_cachedOtherFee;
    double m_cachedMgmtFee;
    double m_cachedDeposit;
    double m_cachedTotal;
    // 工时明细缓存
    QList<LaborItem> m_cachedLaborItems;
    // 备件明细缓存
    QList<PartItem> m_cachedPartItems;

    int m_currentOrderId = 0;
};

#endif // SETTLEMENTPAGE_H
