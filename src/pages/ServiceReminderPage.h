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

private slots:
    void onScan();
    void onExportReminder();

private:
    void setupUI();

    QSpinBox *m_spinDays;
    QSpinBox *m_spinMileage;
    QPushButton *m_btnScan;
    QPushButton *m_btnExport;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // SERVICEREMINDERPAGE_H
