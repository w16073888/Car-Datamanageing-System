#ifndef EMPLOYEEPAGE_H
#define EMPLOYEEPAGE_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QSqlTableModel>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QFormLayout>

class EmployeePage : public QWidget
{
    Q_OBJECT

public:
    explicit EmployeePage(QWidget *parent = nullptr);
    ~EmployeePage();
    void refreshData();

private slots:
    void onEdit();
    void onDelete();
    void onResetPassword();
    void onRegisterEmployee();

private:
    void setupUI();

    QTableView *m_tableView;
    QSqlTableModel *m_model;
    QPushButton *m_btnEdit;
    QPushButton *m_btnDelete;
    QPushButton *m_btnResetPwd;
    QPushButton *m_btnRegister;
    QPushButton *m_btnRefresh;
};

#endif // EMPLOYEEPAGE_H
