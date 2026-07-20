#include "ChangePasswordPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>

ChangePasswordPage::ChangePasswordPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ChangePasswordPage::~ChangePasswordPage()
{
}

void ChangePasswordPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);

    QLabel *title = new QLabel("修改密码");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; padding: 10px;");
    mainLayout->addWidget(title);

    QFormLayout *form = new QFormLayout;
    form->setSpacing(15);
    form->setLabelAlignment(Qt::AlignRight);

    m_editOldPwd = new QLineEdit;
    m_editOldPwd->setEchoMode(QLineEdit::Password);
    m_editOldPwd->setPlaceholderText("请输入当前密码");
    m_editOldPwd->setFixedWidth(300);
    form->addRow("当前密码：", m_editOldPwd);

    m_editNewPwd = new QLineEdit;
    m_editNewPwd->setEchoMode(QLineEdit::Password);
    m_editNewPwd->setPlaceholderText("请输入新密码（至少6位）");
    m_editNewPwd->setFixedWidth(300);
    form->addRow("新密码：", m_editNewPwd);

    m_editConfirmPwd = new QLineEdit;
    m_editConfirmPwd->setEchoMode(QLineEdit::Password);
    m_editConfirmPwd->setPlaceholderText("请再次输入新密码");
    m_editConfirmPwd->setFixedWidth(300);
    form->addRow("确认密码：", m_editConfirmPwd);

    mainLayout->addLayout(form);

    m_lblStatus = new QLabel;
    m_lblStatus->setStyleSheet("color: red; font-size: 13px;");
    m_lblStatus->setVisible(false);
    mainLayout->addWidget(m_lblStatus);

    m_btnChange = new QPushButton("修改密码");
    m_btnChange->setFixedWidth(150);
    m_btnChange->setStyleSheet(
        "QPushButton { padding: 8px 16px; border: none; border-radius: 4px;"
        "  background-color: #e67e22; color: white; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background-color: #d35400; }");
    mainLayout->addWidget(m_btnChange);
    mainLayout->addStretch();

    connect(m_btnChange, &QPushButton::clicked, this, &ChangePasswordPage::onChange);
}

void ChangePasswordPage::onChange()
{
    QString oldPwd = m_editOldPwd->text();
    QString newPwd = m_editNewPwd->text();
    QString confirmPwd = m_editConfirmPwd->text();

    if (oldPwd.isEmpty() || newPwd.isEmpty() || confirmPwd.isEmpty()) {
        m_lblStatus->setText("⚠ 请填写所有字段");
        m_lblStatus->setVisible(true);
        return;
    }

    if (newPwd.length() < 6) {
        m_lblStatus->setText("⚠ 新密码长度不能少于6位");
        m_lblStatus->setVisible(true);
        return;
    }

    if (newPwd != confirmPwd) {
        m_lblStatus->setText("⚠ 两次输入的新密码不一致");
        m_lblStatus->setVisible(true);
        return;
    }

    // 验证当前密码
    int userId = Session::instance().userId();
    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT password FROM t_employee WHERE id = :id");
    query.bindValue(":id", userId);
    DbManager::instance().executeQuery(query);

    if (!query.next()) {
        m_lblStatus->setText("⚠ 用户信息异常");
        m_lblStatus->setVisible(true);
        return;
    }

    QString storedPwd = query.value(0).toString();
    QString oldHash = QCryptographicHash::hash(oldPwd.toUtf8(), QCryptographicHash::Sha256).toHex();

    if (storedPwd != oldHash) {
        m_lblStatus->setText("⚠ 当前密码错误");
        m_lblStatus->setVisible(true);
        return;
    }

    // 更新密码
    QString newHash = QCryptographicHash::hash(newPwd.toUtf8(), QCryptographicHash::Sha256).toHex();
    query.prepare("UPDATE t_employee SET password = :pwd WHERE id = :id");
    query.bindValue(":pwd", newHash);
    query.bindValue(":id", userId);

    if (!DbManager::instance().executeQuery(query)) {
        m_lblStatus->setText("⚠ 密码修改失败：" + DbManager::instance().lastError());
        m_lblStatus->setVisible(true);
        return;
    }

    m_lblStatus->setStyleSheet("color: green; font-size: 13px;");
    m_lblStatus->setText("✓ 密码修改成功！");
    m_lblStatus->setVisible(true);

    m_editOldPwd->clear();
    m_editNewPwd->clear();
    m_editConfirmPwd->clear();
}
