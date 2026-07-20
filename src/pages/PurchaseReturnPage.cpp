#include "PurchaseReturnPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

PurchaseReturnPage::PurchaseReturnPage(QWidget *parent) : QWidget(parent) { setupUI(); }
PurchaseReturnPage::~PurchaseReturnPage() {}

void PurchaseReturnPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("采购退货");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    m_editSearch = new QLineEdit;
    m_editSearch->setPlaceholderText("输入备件编号或名称");
    searchLayout->addWidget(m_editSearch, 1);
    m_btnSearch = new QPushButton("搜索");
    m_btnSearch->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    searchLayout->addWidget(m_btnSearch);
    mainLayout->addLayout(searchLayout);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    QGroupBox *opGroup = new QGroupBox("退货操作");
    QFormLayout *form = new QFormLayout(opGroup);
    m_editPartName = new QLineEdit;
    m_editPartName->setReadOnly(true);
    form->addRow("备件：", m_editPartName);
    m_editSupplier = new QLineEdit;
    m_editSupplier->setReadOnly(true);
    form->addRow("供应商：", m_editSupplier);
    m_lblStock = new QLabel("0");
    form->addRow("当前库存：", m_lblStock);
    m_spinReturnQty = new QSpinBox;
    m_spinReturnQty->setRange(1, 99999);
    form->addRow("退货数量：", m_spinReturnQty);
    m_btnReturn = new QPushButton("确认退货");
    m_btnReturn->setStyleSheet("padding: 8px; background: #e74c3c; color: white; border-radius: 4px; font-weight: bold;");
    form->addRow(m_btnReturn);
    mainLayout->addWidget(opGroup);

    connect(m_btnSearch, &QPushButton::clicked, this, &PurchaseReturnPage::onSearchPart);
    connect(m_btnReturn, &QPushButton::clicked, this, &PurchaseReturnPage::onReturn);
    connect(m_tableView, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_selectedPartId = m_model->data(m_model->index(row, 0)).toInt();
        m_editPartName->setText(m_model->data(m_model->index(row, 2)).toString());
        m_editSupplier->setText(m_model->data(m_model->index(row, 5)).toString());
        int stock = m_model->data(m_model->index(row, 3)).toInt();
        m_lblStock->setText(QString::number(stock));
        m_spinReturnQty->setMaximum(stock);
    });
}

void PurchaseReturnPage::onSearchPart()
{
    QString kw = m_editSearch->text().trimmed();
    QSqlQuery query(DbManager::instance().database());
    if (kw.isEmpty()) {
        query.prepare("SELECT id, part_no AS '编号', name AS '名称', stock AS '库存', "
                      "purchase_price AS '进价', supplier AS '供应商' "
                      "FROM t_parts WHERE stock > 0 ORDER BY id DESC LIMIT 200");
    } else {
        query.prepare("SELECT id, part_no AS '编号', name AS '名称', stock AS '库存', "
                      "purchase_price AS '进价', supplier AS '供应商' "
                      "FROM t_parts WHERE (part_no LIKE :kw OR name LIKE :kw2) AND stock > 0 "
                      "ORDER BY id DESC LIMIT 200");
        query.bindValue(":kw", "%" + kw + "%");
        query.bindValue(":kw2", "%" + kw + "%");
    }
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
}

void PurchaseReturnPage::onReturn()
{
    if (m_selectedPartId == 0) {
        QMessageBox::warning(this, "提示", "请选择备件");
        return;
    }
    int qty = m_spinReturnQty->value();

    DbManager::instance().beginTransaction();

    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT stock, purchase_price FROM t_parts WHERE id = :id");
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);
    if (!query.next()) { DbManager::instance().rollbackTransaction(); return; }
    int stock = query.value(0).toInt();
    double price = query.value(1).toDouble();
    if (qty > stock) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "错误", "退货数量超过库存");
        return;
    }

    query.prepare("UPDATE t_parts SET stock = stock - :qty WHERE id = :id");
    query.bindValue(":qty", qty);
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);

    query.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
                  "operation_type, operator_id) "
                  "VALUES (:pid, :qty, :price, :total, '采购退货', :op)");
    query.bindValue(":pid", m_selectedPartId);
    query.bindValue(":qty", -qty);
    query.bindValue(":price", price);
    query.bindValue(":total", -qty * price);
    query.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(query);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退货成功", "已退货 " + QString::number(qty) + " 个");
    onSearchPart();
}
