#include "InventoryOutPage.h"
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

InventoryOutPage::InventoryOutPage(QWidget *parent) : QWidget(parent) { setupUI(); }
InventoryOutPage::~InventoryOutPage() {}

void InventoryOutPage::refreshData()
{
    m_editOrderNo->clear();
    m_editSearchPart->clear();
    m_lblStock->setText("库存：-");
    m_spinQuantity->setValue(1);
    m_selectedPartId = 0;
    m_model->setQuery(QSqlQuery());
}

void InventoryOutPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("维修出库");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 关联工单
    QHBoxLayout *orderLayout = new QHBoxLayout;
    orderLayout->addWidget(new QLabel("关联工单号："));
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("输入工单号");
    orderLayout->addWidget(m_editOrderNo, 1);
    mainLayout->addLayout(orderLayout);

    // 备件搜索区
    QHBoxLayout *searchLayout = new QHBoxLayout;
    m_editSearchPart = new QLineEdit;
    m_editSearchPart->setPlaceholderText("输入备件编号或名称模糊搜索");
    searchLayout->addWidget(m_editSearchPart, 1);
    m_btnSearch = new QPushButton("搜索");
    m_btnSearch->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    searchLayout->addWidget(m_btnSearch);
    mainLayout->addLayout(searchLayout);

    // 备件列表
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    // 出库操作区
    QGroupBox *opGroup = new QGroupBox("出库操作");
    QHBoxLayout *opLayout = new QHBoxLayout(opGroup);
    opLayout->addWidget(new QLabel("出库数量："));
    m_spinQuantity = new QSpinBox;
    m_spinQuantity->setRange(1, 99999);
    opLayout->addWidget(m_spinQuantity);
    m_lblStock = new QLabel("库存：-");
    opLayout->addWidget(m_lblStock);
    m_btnConfirm = new QPushButton("确认出库");
    m_btnConfirm->setStyleSheet("padding: 6px 14px; background: #e67e22; color: white; border-radius: 4px; font-weight: bold;");
    opLayout->addWidget(m_btnConfirm);
    opLayout->addStretch();
    mainLayout->addWidget(opGroup);

    connect(m_btnSearch, &QPushButton::clicked, this, &InventoryOutPage::onSearchPart);
    connect(m_btnConfirm, &QPushButton::clicked, this, &InventoryOutPage::onConfirmOut);
    connect(m_tableView, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        m_selectedPartId = m_model->data(m_model->index(row, 0)).toInt();
        int stock = m_model->data(m_model->index(row, 3)).toInt();
        m_lblStock->setText(QString("库存：%1").arg(stock));
        m_spinQuantity->setMaximum(stock);
    });
}

void InventoryOutPage::onSearchPart()
{
    QString keyword = m_editSearchPart->text().trimmed();
    QSqlQuery query(DbManager::instance().database());
    if (keyword.isEmpty()) {
        query.prepare("SELECT id, part_no AS '备件编号', name AS '备件名称', "
                      "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                      "FROM t_parts WHERE stock > 0 ORDER BY id DESC LIMIT 200");
    } else {
        query.prepare("SELECT id, part_no AS '备件编号', name AS '备件名称', "
                      "stock AS '库存', sale_price AS '售价', supplier AS '供应商' "
                      "FROM t_parts WHERE (part_no LIKE :kw OR name LIKE :kw2) AND stock > 0 "
                      "ORDER BY id DESC LIMIT 200");
        query.bindValue(":kw", "%" + keyword + "%");
        query.bindValue(":kw2", "%" + keyword + "%");
    }
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
}

void InventoryOutPage::onConfirmOut()
{
    if (m_selectedPartId == 0) {
        QMessageBox::warning(this, "提示", "请先选择备件");
        return;
    }
    QString orderNo = m_editOrderNo->text().trimmed();
    if (orderNo.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入关联工单号");
        return;
    }
    int qty = m_spinQuantity->value();

    DbManager::instance().beginTransaction();

    QSqlQuery query(DbManager::instance().database());

    // 检查库存
    query.prepare("SELECT stock, COALESCE(NULLIF(sale_price,0), purchase_price, 0) FROM t_parts WHERE id = :id");
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);
    if (!query.next()) {
        DbManager::instance().rollbackTransaction();
        return;
    }
    int stock = query.value(0).toInt();
    double salePrice = query.value(1).toDouble();
    if (qty > stock) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "库存不足", QString("当前库存 %1，出库数量不能超过 %1").arg(stock));
        return;
    }

    // 扣减库存
    query.prepare("UPDATE t_parts SET stock = stock - :qty WHERE id = :id");
    query.bindValue(":qty", qty);
    query.bindValue(":id", m_selectedPartId);
    if (!DbManager::instance().executeQuery(query)) {
        DbManager::instance().rollbackTransaction();
        return;
    }

    // 记录流水
    query.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
                  "operation_type, ref_order_no, operator_id) "
                  "VALUES (:pid, :qty, :price, :total, '维修出库', :ref, :op)");
    query.bindValue(":pid", m_selectedPartId);
    query.bindValue(":qty", -qty);
    query.bindValue(":price", salePrice);
    query.bindValue(":total", -qty * salePrice);
    query.bindValue(":ref", orderNo);
    query.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(query);

    // 同时记录到工单明细
    query.prepare("SELECT name FROM t_parts WHERE id = :id");
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);
    QString partName = query.next() ? query.value(0).toString() : "未知";

    query.prepare("SELECT id FROM t_workorder WHERE order_no = :no");
    query.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(query);
    int orderId = 0;
    if (query.next()) orderId = query.value(0).toInt();

    if (orderId > 0) {
        query.prepare("INSERT INTO t_workorder_item (workorder_id, part_id, part_name, "
                      "quantity, unit_price, item_type) "
                      "VALUES (:oid, :pid, :name, :qty, :price, '材料')");
        query.bindValue(":oid", orderId);
        query.bindValue(":pid", m_selectedPartId);
        query.bindValue(":name", partName);
        query.bindValue(":qty", qty);
        query.bindValue(":price", salePrice);
        DbManager::instance().executeQuery(query);
    }

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "出库成功",
        QString("%1 x %2 出库成功，关联工单：%3").arg(partName).arg(qty).arg(orderNo));
    onSearchPart();
}
