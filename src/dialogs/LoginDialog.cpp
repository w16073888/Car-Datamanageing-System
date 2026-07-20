#include "LoginDialog.h"
#include "database/Session.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
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
    setFixedSize(480, 520);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

    setupUI();
}

LoginDialog::~LoginDialog()
{
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

    // Stacked Widget 切换登录/注册
    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack);

    setupLoginPage();
    setupRegisterPage();

    m_stack->setCurrentIndex(0);

    // 整体样式
    setStyleSheet(
        "QDialog { background-color: #f5f6fa; }"
        "QLineEdit {"
        "  padding: 8px 12px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; font-size: 14px;"
        "  background-color: white;"
        "}"
        "QLineEdit:focus { border-color: #3498db; }"
        "QComboBox {"
        "  padding: 8px 12px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; font-size: 14px;"
        "  background-color: white;"
        "}"
        "QPushButton {"
        "  padding: 10px; border-radius: 4px;"
        "  font-size: 15px; font-weight: bold;"
        "}"
    );
}

void LoginDialog::setupLoginPage()
{
    m_loginPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_loginPage);
    layout->setSpacing(12);

    // 标题
    QLabel *loginTitle = new QLabel("用户登录");
    loginTitle->setAlignment(Qt::AlignCenter);
    loginTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin: 10px;");
    layout->addWidget(loginTitle);

    // 工号
    layout->addWidget(new QLabel("工号："));
    m_loginEmployeeId = new QLineEdit;
    m_loginEmployeeId->setPlaceholderText("请输入工号");
    layout->addWidget(m_loginEmployeeId);

    // 密码
    layout->addWidget(new QLabel("密码："));
    m_loginPassword = new QLineEdit;
    m_loginPassword->setPlaceholderText("请输入密码");
    m_loginPassword->setEchoMode(QLineEdit::Password);
    m_loginPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_loginPassword);

    // 错误提示
    m_loginError = new QLabel;
    m_loginError->setStyleSheet("color: red; font-size: 13px;");
    m_loginError->setVisible(false);
    layout->addWidget(m_loginError);

    layout->addSpacing(10);

    // 登录按钮
    m_loginBtn = new QPushButton("登 录");
    m_loginBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }");
    layout->addWidget(m_loginBtn);

    // 跳转注册
    m_toRegisterBtn = new QPushButton("没有账号？点击注册");
    m_toRegisterBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #3498db;"
        "  font-weight: normal; text-decoration: underline; }"
        "QPushButton:hover { color: #2980b9; }");
    layout->addWidget(m_toRegisterBtn);

    layout->addStretch();

    // 连接信号
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_toRegisterBtn, &QPushButton::clicked, this, &LoginDialog::switchToRegister);
    connect(m_loginPassword, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_loginEmployeeId, &QLineEdit::returnPressed, [this]() {
        m_loginPassword->setFocus();
    });

    m_stack->addWidget(m_loginPage);
}

void LoginDialog::setupRegisterPage()
{
    m_registerPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(m_registerPage);
    layout->setSpacing(8);

    QLabel *regTitle = new QLabel("员工注册");
    regTitle->setAlignment(Qt::AlignCenter);
    regTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin: 10px;");
    layout->addWidget(regTitle);

    // 表单
    layout->addWidget(new QLabel("工号 *："));
    m_regEmployeeId = new QLineEdit;
    m_regEmployeeId->setPlaceholderText("请输入工号");
    layout->addWidget(m_regEmployeeId);

    layout->addWidget(new QLabel("姓名 *："));
    m_regName = new QLineEdit;
    m_regName->setPlaceholderText("请输入真实姓名");
    layout->addWidget(m_regName);

    layout->addWidget(new QLabel("密码 *："));
    m_regPassword = new QLineEdit;
    m_regPassword->setPlaceholderText("请输入密码（至少6位）");
    m_regPassword->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_regPassword);

    layout->addWidget(new QLabel("确认密码 *："));
    m_regConfirmPwd = new QLineEdit;
    m_regConfirmPwd->setPlaceholderText("请再次输入密码");
    m_regConfirmPwd->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_regConfirmPwd);

    layout->addWidget(new QLabel("职位 *："));
    m_regPosition = new QComboBox;
    m_regPosition->addItems({"经理", "前台", "库管", "客服"});
    layout->addWidget(m_regPosition);

    layout->addWidget(new QLabel("联系电话："));
    m_regPhone = new QLineEdit;
    m_regPhone->setPlaceholderText("请输入手机号码");
    layout->addWidget(m_regPhone);

    // 错误提示
    m_regError = new QLabel;
    m_regError->setStyleSheet("color: red; font-size: 13px;");
    m_regError->setVisible(false);
    layout->addWidget(m_regError);

    // 注册按钮
    m_regBtn = new QPushButton("注 册");
    m_regBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; }"
        "QPushButton:hover { background-color: #219a52; }");
    layout->addWidget(m_regBtn);

    // 返回登录
    m_toLoginBtn = new QPushButton("已有账号？返回登录");
    m_toLoginBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: #3498db;"
        "  font-weight: normal; text-decoration: underline; }"
        "QPushButton:hover { color: #2980b9; }");
    layout->addWidget(m_toLoginBtn);

    // 信号
    connect(m_regBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_toLoginBtn, &QPushButton::clicked, this, &LoginDialog::switchToLogin);

    m_stack->addWidget(m_registerPage);
}

QString LoginDialog::sha256(const QString &text) const
{
    QByteArray hash = QCryptographicHash::hash(
        text.toUtf8(), QCryptographicHash::Sha256);
    return QString(hash.toHex());
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

    if (storedPwd != sha256(password)) {
        m_loginError->setText("⚠ 密码错误");
        m_loginError->setVisible(true);
        return;
    }

    // 登录成功
    Session::instance().login(id, empId, query.value(2).toString(), position);
    accept();
}

void LoginDialog::onRegisterClicked()
{
    QString empId = m_regEmployeeId->text().trimmed();
    QString name = m_regName->text().trimmed();
    QString password = m_regPassword->text();
    QString confirmPwd = m_regConfirmPwd->text();
    QString position = m_regPosition->currentText();
    QString phone = m_regPhone->text().trimmed();

    // 表单校验
    if (empId.isEmpty() || name.isEmpty() || password.isEmpty() || confirmPwd.isEmpty()) {
        m_regError->setText("⚠ 请填写所有必填项（带*号）");
        m_regError->setVisible(true);
        return;
    }

    if (password.length() < 6) {
        m_regError->setText("⚠ 密码长度不能少于6位");
        m_regError->setVisible(true);
        return;
    }

    if (password != confirmPwd) {
        m_regError->setText("⚠ 两次密码输入不一致");
        m_regError->setVisible(true);
        return;
    }

    m_regError->setVisible(false);

    // 检查工号是否已存在
    QSqlQuery checkQuery(DbManager::instance().database());
    checkQuery.prepare("SELECT COUNT(*) FROM t_employee WHERE employee_id = :eid");
    checkQuery.bindValue(":eid", empId);
    DbManager::instance().executeQuery(checkQuery);
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        m_regError->setText("⚠ 该工号已被注册");
        m_regError->setVisible(true);
        return;
    }

    // 插入新员工
    QSqlQuery query(DbManager::instance().database());
    query.prepare("INSERT INTO t_employee (employee_id, name, password, position, phone) "
                  "VALUES (:eid, :name, :pwd, :pos, :phone)");
    query.bindValue(":eid", empId);
    query.bindValue(":name", name);
    query.bindValue(":pwd", sha256(password));
    query.bindValue(":pos", position);
    query.bindValue(":phone", phone.isEmpty() ? QVariant() : phone);

    if (!DbManager::instance().executeQuery(query)) {
        m_regError->setText("⚠ 注册失败：" + DbManager::instance().lastError());
        m_regError->setVisible(true);
        return;
    }

    QMessageBox::information(this, "注册成功",
        QString("员工 %1 (%2) 注册成功！\n请返回登录页面进行登录。").arg(name, empId));

    // 清空注册页表单，切回登录
    m_regEmployeeId->clear();
    m_regName->clear();
    m_regPassword->clear();
    m_regConfirmPwd->clear();
    m_regPhone->clear();
    switchToLogin();
}

void LoginDialog::switchToRegister()
{
    m_loginError->setVisible(false);
    m_stack->setCurrentIndex(1);
}

void LoginDialog::switchToLogin()
{
    m_regError->setVisible(false);
    m_stack->setCurrentIndex(0);
}
