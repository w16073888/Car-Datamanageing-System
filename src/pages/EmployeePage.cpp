#include "EmployeePage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QInputDialog>
#include <QCryptographicHash>

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

    m_btnAdd = new QPushButton("添加员工");
    m_btnEdit = new QPushButton("编辑");
    m_btnDelete = new QPushButton("删除");
    m_btnResetPwd = new QPushButton("重置密码");
    m_btnRefresh = new QPushButton("刷新");

    QString btnStyle =
        "QPushButton { padding: 6px 14px; border: 1px solid #bdc3c7;"
        "  border-radius: 4px; background-color: #ecf0f1; font-size: 13px; }"
        "QPushButton:hover { background-color: #3498db; color: white; }";
    m_btnAdd->setStyleSheet(btnStyle);
    m_btnEdit->setStyleSheet(btnStyle);
    m_btnDelete->setStyleSheet(btnStyle);
    m_btnResetPwd->setStyleSheet(btnStyle);
    m_btnRefresh->setStyleSheet(btnStyle);

    toolbar->addWidget(m_btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDelete);
    toolbar->addWidget(m_btnResetPwd);
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

    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("t_employee");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setHeaderData(1, Qt::Horizontal, "工号");
    m_model->setHeaderData(2, Qt::Horizontal, "姓名");
    m_model->setHeaderData(4, Qt::Horizontal, "职位");
    m_model->setHeaderData(5, Qt::Horizontal, "电话");
    m_model->setHeaderData(6, Qt::Horizontal, "创建时间");
    // 隐藏不需要的列
    m_model->setSort(0, Qt::AscendingOrder);
    m_tableView->setModel(m_model);
    m_tableView->setColumnHidden(0, true);   // id
    m_tableView->setColumnHidden(3, true);   // password
    m_tableView->setColumnHidden(7, true);   // updated_at
    m_tableView->setColumnHidden(8, true);   // is_active

    // 信号
    connect(m_btnAdd, &QPushButton::clicked, this, &EmployeePage::onAdd);
    connect(m_btnEdit, &QPushButton::clicked, this, &EmployeePage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &EmployeePage::onDelete);
    connect(m_btnResetPwd, &QPushButton::clicked, this, &EmployeePage::onResetPassword);
    connect(m_btnRefresh, &QPushButton::clicked, this, &EmployeePage::refreshData);
}

void EmployeePage::refreshData()
{
    m_model->select();
    m_tableView->resizeColumnsToContents();
}

void EmployeePage::onAdd()
{
    // 简单添加——打开输入对话框收集信息
    // 实际生产环境应使用专用表单对话框
    QDialog dlg(this);
    dlg.setWindowTitle("添加员工");
    dlg.setFixedSize(350, 350);

    QFormLayout *form = new QFormLayout(&dlg);

    QLineEdit *editEmpId = new QLineEdit;
    editEmpId->setPlaceholderText("工号");
    form->addRow("工号 *：", editEmpId);

    QLineEdit *editName = new QLineEdit;
    editName->setPlaceholderText("姓名");
    form->addRow("姓名 *：", editName);

    QLineEdit *editPwd = new QLineEdit;
    editPwd->setEchoMode(QLineEdit::Password);
    editPwd->setPlaceholderText("初始密码");
    form->addRow("密码 *：", editPwd);

    QComboBox *cmbPos = new QComboBox;
    cmbPos->addItems({"经理", "前台", "库管", "客服"});
    form->addRow("职位：", cmbPos);

    QLineEdit *editPhone = new QLineEdit;
    editPhone->setPlaceholderText("电话");
    form->addRow("电话：", editPhone);

    QDialogButtonBox *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    QString empId = editEmpId->text().trimmed();
    QString name = editName->text().trimmed();
    QString pwd = editPwd->text();
    QString pos = cmbPos->currentText();
    QString phone = editPhone->text().trimmed();

    if (empId.isEmpty() || name.isEmpty() || pwd.isEmpty()) {
        QMessageBox::warning(this, "错误", "请填写必填项（工号、姓名、密码）");
        return;
    }

    QSqlQuery query(DbManager::instance().database());
    query.prepare("INSERT INTO t_employee (employee_id, name, password, position, phone) "
                  "VALUES (:eid, :name, :pwd, :pos, :phone)");
    query.bindValue(":eid", empId);
    query.bindValue(":name", name);
    QByteArray hash = QCryptographicHash::hash(pwd.toUtf8(), QCryptographicHash::Sha256);
    query.bindValue(":pwd", QString(hash.toHex()));
    query.bindValue(":pos", pos);
    query.bindValue(":phone", phone.isEmpty() ? QVariant() : phone);

    if (!DbManager::instance().executeQuery(query)) {
        QMessageBox::warning(this, "添加失败", DbManager::instance().lastError());
        return;
    }

    refreshData();
    QMessageBox::information(this, "成功", QString("员工 %1 (%2) 添加成功").arg(name, empId));
}

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

    QSqlQuery query(DbManager::instance().database());
    query.prepare("UPDATE t_employee SET name = :name, position = :pos, phone = :phone "
                  "WHERE employee_id = :eid");
    query.bindValue(":name", editName->text().trimmed());
    query.bindValue(":pos", cmbPos->currentText());
    query.bindValue(":phone", editPhone->text().trimmed().isEmpty() ? QVariant() : editPhone->text().trimmed());
    query.bindValue(":eid", empId);

    if (!DbManager::instance().executeQuery(query)) {
        QMessageBox::warning(this, "更新失败", DbManager::instance().lastError());
        return;
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

    QSqlQuery query(DbManager::instance().database());
    query.prepare("UPDATE t_employee SET is_active = 0 WHERE id = :id");
    query.bindValue(":id", id);

    if (!DbManager::instance().executeQuery(query)) {
        QMessageBox::warning(this, "删除失败", DbManager::instance().lastError());
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

    QByteArray hash = QCryptographicHash::hash(newPwd.toUtf8(), QCryptographicHash::Sha256);

    QSqlQuery query(DbManager::instance().database());
    query.prepare("UPDATE t_employee SET password = :pwd WHERE employee_id = :eid");
    query.bindValue(":pwd", QString(hash.toHex()));
    query.bindValue(":eid", empId);

    if (!DbManager::instance().executeQuery(query)) {
        QMessageBox::warning(this, "重置失败", DbManager::instance().lastError());
        return;
    }

    QMessageBox::information(this, "成功", QString("员工 %1 密码已重置").arg(empId));
}
