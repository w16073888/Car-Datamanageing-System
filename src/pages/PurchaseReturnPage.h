#ifndef PURCHASERETURNPAGE_H
#define PURCHASERETURNPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSqlQueryModel>

class PurchaseReturnPage : public QWidget
{
    Q_OBJECT

public:
    explicit PurchaseReturnPage(QWidget *parent = nullptr);
    ~PurchaseReturnPage();

private slots:
    void onSearchPart();
    void onReturn();

private:
    void setupUI();

    QLineEdit *m_editSearch;
    QPushButton *m_btnSearch;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLineEdit *m_editPartName;
    QLineEdit *m_editSupplier;
    QLabel *m_lblStock;
    QSpinBox *m_spinReturnQty;
    QPushButton *m_btnReturn;

    int m_selectedPartId = 0;
};

#endif // PURCHASERETURNPAGE_H
