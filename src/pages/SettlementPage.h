#ifndef SETTLEMENTPAGE_H
#define SETTLEMENTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>

class SettlementPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettlementPage(QWidget *parent = nullptr);
    ~SettlementPage();
    void refreshData();

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

    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLabel *m_lblLaborFee;
    QLabel *m_lblMaterialFee;
    QLabel *m_lblOtherFee;
    QLabel *m_lblManagementFee;
    QLabel *m_lblDeposit;
    QLabel *m_lblTotal;

    int m_currentOrderId = 0;
};

#endif // SETTLEMENTPAGE_H
