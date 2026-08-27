#ifndef SERVICEREMINDERPAGE_H
#define SERVICEREMINDERPAGE_H

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>

class ServiceReminderPage : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceReminderPage(QWidget *parent = nullptr);
    ~ServiceReminderPage();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onScan();

private:
    void setupUI();

    QSpinBox *m_spinDays;
    QSpinBox *m_spinMileage;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // SERVICEREMINDERPAGE_H
