#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    static QString sha256(const QString &text);

private slots:
    void onLoginClicked();

private:
    void setupUI();

    QLineEdit *m_loginEmployeeId;
    QLineEdit *m_loginPassword;
    QPushButton *m_loginBtn;
    QLabel *m_loginError;
};

#endif // LOGINDIALOG_H
