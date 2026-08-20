#include "LoginDialog.h"
#include "database/Session.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QSqlError>
#include <QApplication>
#include <QScreen>
#include <QFont>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("汽修4S店管理系统 - 登录");
    setFixedSize(420, 380);
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
    QLabel *titleLabel = new QLabel("汽修4S店综合管理系统");
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
        "QLineEdit {"
        "  padding: 8px 12px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; font-size: 14px;"
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

    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT id, employee_id, name, password, position, is_active "
                  "FROM t_employee WHERE employee_id = :eid");
    query.bindValue(":eid", empId);

    if (!DbManager::instance().executeQuery(query)) {
        m_loginError->setText("⚠ 数据库查询失败：" + DbManager::instance().lastError());
        m_loginError->setVisible(true);
        return;
    }

    if (!query.next()) {
        m_loginError->setText("⚠ 工号不存在");
        m_loginError->setVisible(true);
        return;
    }

    int id = query.value(0).toInt();
    QString storedPwd = query.value(3).toString();
    QString position = query.value(4).toString();
    int active = query.value(5).toInt();

    if (active != 1) {
        m_loginError->setText("⚠ 该账号已被禁用，请联系管理员");
        m_loginError->setVisible(true);
        return;
    }

    // 支持明文和旧版SHA256哈希两种密码格式
    if (storedPwd != password) {
        // 明文不匹配时尝试兼容旧版SHA256哈希
        if (storedPwd == sha256(password)) {
            // 密码匹配但仍是旧版哈希 → 更新为明文
            QSqlQuery updateQuery(DbManager::instance().database());
            updateQuery.prepare("UPDATE t_employee SET password = :pwd WHERE id = :id");
            updateQuery.bindValue(":pwd", password);
            updateQuery.bindValue(":id", id);
            DbManager::instance().executeQuery(updateQuery);
        } else {
            m_loginError->setText("⚠ 密码错误");
            m_loginError->setVisible(true);
            return;
        }
    }

    // 登录成功
    Session::instance().login(id, empId, query.value(2).toString(), position);
    accept();
}
