#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void switchToRegister();
    void switchToLogin();

private:
    void setupUI();
    void setupLoginPage();
    void setupRegisterPage();
    QString sha256(const QString &text) const;

    // 页面容器
    QStackedWidget *m_stack;

    // --- 登录页 ---
    QWidget *m_loginPage;
    QLineEdit *m_loginEmployeeId;
    QLineEdit *m_loginPassword;
    QPushButton *m_loginBtn;
    QPushButton *m_toRegisterBtn;
    QLabel *m_loginError;

    // --- 注册页 ---
    QWidget *m_registerPage;
    QLineEdit *m_regEmployeeId;
    QLineEdit *m_regName;
    QLineEdit *m_regPassword;
    QLineEdit *m_regConfirmPwd;
    QComboBox *m_regPosition;
    QLineEdit *m_regPhone;
    QPushButton *m_regBtn;
    QPushButton *m_toLoginBtn;
    QLabel *m_regError;
};

#endif // LOGINDIALOG_H
