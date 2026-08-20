#ifndef CHANGEPASSWORDPAGE_H
#define CHANGEPASSWORDPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ChangePasswordPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChangePasswordPage(QWidget *parent = nullptr);
    ~ChangePasswordPage();
    void refreshData();

private slots:
    void onChange();

private:
    void setupUI();

    QLineEdit *m_editOldPwd;
    QLineEdit *m_editNewPwd;
    QLineEdit *m_editConfirmPwd;
    QPushButton *m_btnChange;
    QLabel *m_lblStatus;
};

#endif // CHANGEPASSWORDPAGE_H
