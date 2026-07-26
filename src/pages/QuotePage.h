#ifndef QUOTEPAGE_H
#define QUOTEPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QTextEdit>

class QuotePage : public QWidget
{
    Q_OBJECT

public:
    explicit QuotePage(QWidget *parent = nullptr);
    ~QuotePage();
    void refreshData();

private slots:
    void onLoadOrder();
    void onAddItem();
    void onRemoveItem();
    void onPrint();

private:
    void setupUI();
    void refreshItems();
    void updateTotal();

    QLineEdit *m_editOrderNo;
    QPushButton *m_btnLoad;
    QLabel *m_lblOrderInfo;

    // 报价明细
    QLineEdit *m_editPartName;
    QLineEdit *m_editQuantity;
    QLineEdit *m_editUnitPrice;
    QPushButton *m_btnAdd;
    QPushButton *m_btnRemove;

    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLabel *m_lblLaborFee;
    QLabel *m_lblMaterialTotal;
    QLabel *m_lblGrandTotal;

    QPushButton *m_btnPrint;

    int m_currentOrderId = 0;
};

#endif // QUOTEPAGE_H
