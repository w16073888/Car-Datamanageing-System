#include "EmployeePage.h"
#include "database/Session.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>
#include <QInputDialog>
#include <QDialogButtonBox>

EmployeePage::EmployeePage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refreshData();
}

EmployeePage::~EmployeePage()
{
}

void EmployeePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    QLabel *title = new QLabel("员工管理");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; padding: 5px;");
    mainLayout->addWidget(title);

    // 按钮工具栏
    QHBoxLayout *toolbar = new QHBoxLayout;

    m_btnEdit = new QPushButton("编辑");
    m_btnDelete = new QPushButton("删除");
    m_btnResetPwd = new QPushButton("重置密码");
    m_btnRegister = new QPushButton("注册新员工");
    m_btnRefresh = new QPushButton("刷新");

    QString btnStyle =
        "QPushButton { padding: 6px 14px; border: 1px solid #bdc3c7;"
        "  border-radius: 4px; background-color: #ecf0f1; font-size: 13px; }"
        "QPushButton:hover { background-color: #3498db; color: white; }";
    m_btnEdit->setStyleSheet(btnStyle);
    m_btnDelete->setStyleSheet(btnStyle);
    m_btnResetPwd->setStyleSheet(btnStyle);
    m_btnRefresh->setStyleSheet(btnStyle);

    m_btnRegister->setStyleSheet(
        "QPushButton { padding: 6px 14px; border: none; border-radius: 4px;"
        "  background-color: #27ae60; color: white; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #219a52; }");

    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addWidget(m_btnResetPwd);
    toolbar->addWidget(m_btnRegister);
    toolbar->addWidget(m_btnRefresh);
    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    // 表格
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; gridline-color: #ecf0f1; }"
        "QHeaderView::section { background-color: #34495e; color: white;"
        "  padding: 6px; border: none; font-weight: bold; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new RemoteModel(this);
    m_model->setTable("t_employee");
    m_model->setEditStrategy(RemoteModel::OnManualSubmit);
    // 只显示在职员工：删除为软删（is_active=0），过滤后列表即时消失
    m_model->setFilter("is_active = 1");
    m_model->setHeaderData(1, Qt::Horizontal, "工号");
    m_model->setHeaderData(2, Qt::Horizontal, "姓名");
    m_model->setHeaderData(3, Qt::Horizontal, "密码");
    m_model->setHeaderData(4, Qt::Horizontal, "职位");
    m_model->setHeaderData(5, Qt::Horizontal, "电话");
    m_model->setHeaderData(6, Qt::Horizontal, "创建时间");
    // 隐藏不需要的列
    m_model->setSort(0, Qt::AscendingOrder);
    m_tableView->setModel(m_model);
    m_tableView->setColumnHidden(0, true);   // id
    m_tableView->setColumnHidden(7, true);   // updated_at
    m_tableView->setColumnHidden(8, true);   // is_active

    // 信号
    connect(m_btnEdit, &QPushButton::clicked, this, &EmployeePage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &EmployeePage::onDelete);
    connect(m_btnResetPwd, &QPushButton::clicked, this, &EmployeePage::onResetPassword);
    connect(m_btnRegister, &QPushButton::clicked, this, &EmployeePage::onRegisterEmployee);
    connect(m_btnRefresh, &QPushButton::clicked, this, &EmployeePage::refreshData);
}

void EmployeePage::refreshData()
{
    m_model->select();
    m_tableView->resizeColumnsToContents();
}

// 注：position 列的 ENUM 类型修复原本在此运行时执行（ALTER TABLE 迁移），
//     现已移入服务器端一次性工具 DbSetup（sql/migrations），客户端不再执行 DDL。

void EmployeePage::onEdit()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "提示", "请先选择要编辑的员工");
        return;
    }

    int row = idx.row();
    QString empId = m_model->record(row).value("employee_id").toString();
    QString name = m_model->record(row).value("name").toString();
    QString position = m_model->record(row).value("position").toString();
    QString phone = m_model->record(row).value("phone").toString();

    QDialog dlg(this);
    dlg.setWindowTitle("编辑员工 - " + empId);
    dlg.setFixedSize(350, 280);

    QFormLayout *form = new QFormLayout(&dlg);

    QLineEdit *editName = new QLineEdit(name);
    form->addRow("姓名：", editName);

    QComboBox *cmbPos = new QComboBox;
    cmbPos->addItems({"经理", "前台", "库管", "客服"});
    cmbPos->setCurrentText(position);
    form->addRow("职位：", cmbPos);

    QLineEdit *editPhone = new QLineEdit(phone);
    form->addRow("电话：", editPhone);

    QDialogButtonBox *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    // 参数化更新（经 4s-server 执行，JSON 传输无中文编码问题）
    {
        QString phoneVal = editPhone->text().trimmed();
        RemoteQuery query;
        query.prepare("UPDATE t_employee SET name = :name, position = :position, "
                      "phone = :phone WHERE employee_id = :eid");
        query.bindValue(":name", editName->text().trimmed());
        query.bindValue(":position", cmbPos->currentText());
        query.bindValue(":phone", phoneVal.isEmpty() ? QVariant() : QVariant(phoneVal));
        query.bindValue(":eid", empId);
        if (!query.exec()) {
            QMessageBox::warning(this, "更新失败", query.lastError().text());
            return;
        }
    }

    refreshData();
}

void EmployeePage::onDelete()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "提示", "请先选择要删除的员工");
        return;
    }

    int row = idx.row();
    int id = m_model->record(row).value("id").toInt();
    QString name = m_model->record(row).value("name").toString();

    if (id == Session::instance().userId()) {
        QMessageBox::warning(this, "禁止操作", "不能删除自己的账号");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除", QString("确定要删除员工 %1 吗？").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    RemoteQuery query;
    query.prepare("UPDATE t_employee SET is_active = 0 WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        QMessageBox::warning(this, "删除失败", query.lastError().text());
        return;
    }

    refreshData();
}

void EmployeePage::onResetPassword()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::information(this, "提示", "请先选择要重置密码的员工");
        return;
    }

    QString empId = m_model->record(idx.row()).value("employee_id").toString();

    bool ok;
    QString newPwd = QInputDialog::getText(this, "重置密码",
        QString("请输入 %1 的新密码：").arg(empId),
        QLineEdit::Password, "123456", &ok);
    if (!ok || newPwd.isEmpty()) return;

    RemoteQuery query;
    query.prepare("UPDATE t_employee SET password = :pwd WHERE employee_id = :eid");
    query.bindValue(":pwd", newPwd);
    query.bindValue(":eid", empId);

    if (!query.exec()) {
        QMessageBox::warning(this, "重置失败", query.lastError().text());
        return;
    }

    QMessageBox::information(this, "成功", QString("员工 %1 密码已重置").arg(empId));
}

void EmployeePage::onRegisterEmployee()
{
    // 打开注册对话框，复用原有的注册逻辑
    QDialog dlg(this);
    dlg.setWindowTitle("注册新员工");
    dlg.setFixedSize(380, 420);

    QFormLayout *form = new QFormLayout(&dlg);
    form->setSpacing(10);

    QLineEdit *editEmpId = new QLineEdit;
    editEmpId->setPlaceholderText("请输入工号");
    form->addRow("工号 *：", editEmpId);

    QLineEdit *editName = new QLineEdit;
    editName->setPlaceholderText("请输入真实姓名");
    form->addRow("姓名 *：", editName);

    QLineEdit *editPwd = new QLineEdit;
    editPwd->setEchoMode(QLineEdit::Password);
    editPwd->setPlaceholderText("请输入密码（至少6位）");
    form->addRow("密码 *：", editPwd);

    QLineEdit *editConfirmPwd = new QLineEdit;
    editConfirmPwd->setEchoMode(QLineEdit::Password);
    editConfirmPwd->setPlaceholderText("请再次输入密码");
    form->addRow("确认密码 *：", editConfirmPwd);

    QComboBox *cmbPos = new QComboBox;
    cmbPos->addItems({"经理", "前台", "库管", "客服"});
    form->addRow("职位 *：", cmbPos);

    QLineEdit *editPhone = new QLineEdit;
    editPhone->setPlaceholderText("请输入手机号码");
    form->addRow("联系电话：", editPhone);

    // 错误提示
    QLabel *errorLabel = new QLabel;
    errorLabel->setStyleSheet("color: red; font-size: 13px;");
    errorLabel->setVisible(false);
    form->addRow(errorLabel);

    QDialogButtonBox *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btnBox->button(QDialogButtonBox::Ok)->setText("注册");
    form->addRow(btnBox);

    // 断开 QDialogButtonBox 的默认 auto-accept/reject，
    // 改由我们自己控制：只有校验通过且数据库写入成功才调用 dlg.accept()
    QObject::disconnect(btnBox, &QDialogButtonBox::accepted, nullptr, nullptr);
    QObject::disconnect(btnBox, &QDialogButtonBox::rejected, nullptr, nullptr);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, [&]() {
        // 表单校验
        QString empId = editEmpId->text().trimmed();
        QString name = editName->text().trimmed();
        QString password = editPwd->text();
        QString confirmPwd = editConfirmPwd->text();
        QString position = cmbPos->currentText();
        QString phone = editPhone->text().trimmed();

        if (empId.isEmpty() || name.isEmpty() || password.isEmpty() || confirmPwd.isEmpty()) {
            errorLabel->setText("⚠ 请填写所有必填项（带*号）");
            errorLabel->setVisible(true);
            return;
        }

        if (password.length() < 6) {
            errorLabel->setText("⚠ 密码长度不能少于6位");
            errorLabel->setVisible(true);
            return;
        }

        if (password != confirmPwd) {
            errorLabel->setText("⚠ 两次密码输入不一致");
            errorLabel->setVisible(true);
            return;
        }

        errorLabel->setVisible(false);

        // 检查工号是否已存在
        RemoteQuery checkQuery;
        checkQuery.prepare("SELECT COUNT(*) FROM t_employee WHERE employee_id = :eid");
        checkQuery.bindValue(":eid", empId);
        checkQuery.exec();
        if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            errorLabel->setText("⚠ 该工号已被注册");
            errorLabel->setVisible(true);
            return;
        }

        // 插入新员工（参数化，经 4s-server 执行）
        {
            RemoteQuery query;
            query.prepare("INSERT INTO t_employee "
                          "(employee_id, name, password, position, phone) "
                          "VALUES (:eid, :name, :pwd, :pos, :phone)");
            query.bindValue(":eid", empId);
            query.bindValue(":name", name);
            query.bindValue(":pwd", password);
            query.bindValue(":pos", position);
            query.bindValue(":phone", phone.isEmpty() ? QVariant() : QVariant(phone));
            if (!query.exec()) {
                errorLabel->setText("⚠ 注册失败：" + query.lastError().text());
                errorLabel->setVisible(true);
                return;
            }
        }

        QMessageBox::information(&dlg, "注册成功",
            QString("员工 %1 (%2) 注册成功！").arg(name, empId));
        dlg.accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        refreshData();
    }
}
