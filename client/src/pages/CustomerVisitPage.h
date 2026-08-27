#ifndef CUSTOMERVISITPAGE_H
#define CUSTOMERVISITPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include "remote/RemoteModel.h"

class CustomerVisitPage : public QWidget
{
    Q_OBJECT

public:
    explicit CustomerVisitPage(QWidget *parent = nullptr);
    ~CustomerVisitPage();

private slots:
    void onLoadSettledOrders();
    void onSelectOrder(const QModelIndex &index);
    void onOrderDoubleClicked(const QModelIndex &index);
    void onSaveVisit();

private:
    void setupUI();

    QTableView *m_tableView;
    RemoteModel *m_model;

    QLabel *m_lblOrderInfo;
    QSpinBox *m_spinVisitDays;    // 回访间隔天数（回访日期）
    QComboBox *m_cmbSatisfaction;
    QTextEdit *m_textRemark;
    QPushButton *m_btnSave;

    int m_currentOrderId = 0;
};

#endif // CUSTOMERVISITPAGE_H
