#ifndef CUSTOMERVISITPAGE_H
#define CUSTOMERVISITPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QLineEdit>

class CustomerVisitPage : public QWidget
{
    Q_OBJECT

public:
    explicit CustomerVisitPage(QWidget *parent = nullptr);
    ~CustomerVisitPage();

private slots:
    void onLoadSettledOrders();
    void onSelectOrder(const QModelIndex &index);
    void onSaveVisit();

private:
    void setupUI();

    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLabel *m_lblOrderInfo;
    QComboBox *m_cmbSatisfaction;
    QTextEdit *m_textRemark;
    QPushButton *m_btnSave;

    int m_currentOrderId = 0;
};

#endif // CUSTOMERVISITPAGE_H
