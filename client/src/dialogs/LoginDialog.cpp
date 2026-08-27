#include "LoginDialog.h"
#include "database/Session.h"
#include "remote/RemoteClient.h"
#include "remote/RemoteDb.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QApplication>
#include <QScreen>
#include <QFont>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("科盟汽车管理系统 - 登录");
    setFixedSize(560, 580);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

    setupUI();
}

LoginDialog::~LoginDialog()
{
}

QString LoginDialog::sha256(const QString &text)
{
    QByteArray hash = QCryptographicHash::hash(
        text.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
}

void LoginDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);

    // 标题
    QLabel *titleLabel = new QLabel("科盟汽车管理系统");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 22px; font-weight: bold; color: #2c3e50;"
        "padding: 15px; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // 副标题
    QLabel *loginTitle = new QLabel("用户登录");
    loginTitle->setAlignment(Qt::AlignCenter);
    loginTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin: 10px;");
    mainLayout->addWidget(loginTitle);

    // 工号
    mainLayout->addWidget(new QLabel("工号："));
    m_loginEmployeeId = new QLineEdit;
    m_loginEmployeeId->setPlaceholderText("请输入工号");
    mainLayout->addWidget(m_loginEmployeeId);

    // 密码
    mainLayout->addWidget(new QLabel("密码："));
    m_loginPassword = new QLineEdit;
    m_loginPassword->setPlaceholderText("请输入密码");
    m_loginPassword->setEchoMode(QLineEdit::Password);
    mainLayout->addWidget(m_loginPassword);

    // 错误提示
    m_loginError = new QLabel;
    m_loginError->setStyleSheet("color: red; font-size: 13px;");
    m_loginError->setVisible(false);
    mainLayout->addWidget(m_loginError);

    mainLayout->addSpacing(10);

    // 登录按钮
    m_loginBtn = new QPushButton("登 录");
    m_loginBtn->setStyleSheet(
        "QPushButton { padding: 10px; border-radius: 4px; font-size: 15px; font-weight: bold;"
        "  background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }");
    mainLayout->addWidget(m_loginBtn);

    mainLayout->addStretch();

    // 连接信号
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_loginEmployeeId, &QLineEdit::returnPressed, [this]() {
        m_loginPassword->setFocus();
    });

    // 整体样式
    setStyleSheet(
        "QDialog { background-color: #f5f6fa; }"
        "QLabel { font-size: 16px; color: #2c3e50; }"
        "QLineEdit {"
        "  padding: 10px 14px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; font-size: 18px;"
        "  background-color: white;"
        "}"
        "QLineEdit:focus { border-color: #3498db; }"
        "QPushButton {"
        "  padding: 10px; border-radius: 4px;"
        "  font-size: 15px; font-weight: bold;"
        "}"
    );
}

void LoginDialog::onLoginClicked()
{
    QString empId = m_loginEmployeeId->text().trimmed();
    QString password = m_loginPassword->text();

    if (empId.isEmpty() || password.isEmpty()) {
        m_loginError->setText("⚠ 请输入工号和密码");
        m_loginError->setVisible(true);
        return;
    }

    m_loginError->setVisible(false);

    // 登录校验在 4s-server 端完成（auth.login），客户端不再直连数据库
    m_loginBtn->setEnabled(false);
    m_loginBtn->setText("登录中...");

    QJsonObject r = RemoteDb::login(empId, password);
    if (!r.value("ok").toBool()) {
        m_loginError->setText("⚠ " + r.value("error").toString());
        m_loginError->setVisible(true);
        m_loginBtn->setEnabled(true);
        m_loginBtn->setText("登 录");
        return;
    }

    const QJsonObject data = r.value("data").toObject();
    RemoteClient::instance().setToken(data.value("token").toString());
    Session::instance().login(data.value("userId").toInt(),
                              data.value("employeeId").toString(),
                              data.value("name").toString(),
                              data.value("position").toString());

    m_loginBtn->setEnabled(true);
    m_loginBtn->setText("登 录");
    accept();
}
