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

private slots:
    void onLoadOrder();
    void onSettle();
    void onPrint();

private:
    void setupUI();

    QLineEdit *m_editOrderNo;
    QPushButton *m_btnLoad;
    QLabel *m_lblOrderInfo;

    // 工单明细
    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLabel *m_lblLaborFee;
    QLabel *m_lblMaterialFee;
    QLabel *m_lblTotal;

    QPushButton *m_btnSettle;
    QPushButton *m_btnPrint;

    int m_currentOrderId = 0;
};

#endif // SETTLEMENTPAGE_H
