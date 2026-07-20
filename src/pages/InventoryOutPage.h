#ifndef INVENTORYOUTPAGE_H
#define INVENTORYOUTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSqlQueryModel>

class InventoryOutPage : public QWidget
{
    Q_OBJECT

public:
    explicit InventoryOutPage(QWidget *parent = nullptr);
    ~InventoryOutPage();

private slots:
    void onSearchPart();
    void onConfirmOut();

private:
    void setupUI();

    QLineEdit *m_editOrderNo;
    QLineEdit *m_editSearchPart;
    QPushButton *m_btnSearch;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLineEdit *m_editPartNo;
    QLineEdit *m_editPartName;
    QLabel *m_lblStock;
    QSpinBox *m_spinQuantity;

    QPushButton *m_btnConfirm;

    int m_selectedPartId = 0;
};

#endif // INVENTORYOUTPAGE_H
