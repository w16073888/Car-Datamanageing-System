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
    // 采购退货仅搜索在库中的备件（基于 t_part_instance）
    QString kw = m_editSearch->text().trimmed();
    QSqlQuery query(DbManager::instance().database());
    QString sql = QString(
        "SELECT p.id, p.part_no AS '编号', p.name AS '名称', "
        "COUNT(CASE WHEN i.status='在库' THEN 1 END) AS '库存', "
        "COALESCE(p.purchase_price, 0) AS '进价', "
        "COALESCE(p.supplier, '') AS '供应商' "
        "FROM t_parts p "
        "JOIN t_part_instance i ON i.part_id = p.id "
        "WHERE i.status = '在库'");
    if (!kw.isEmpty()) {
        sql += " AND (p.part_no LIKE :kw OR p.name LIKE :kw2)";
    }
    sql += " GROUP BY p.id, p.part_no, p.name, p.purchase_price, p.supplier "
           "ORDER BY p.id DESC LIMIT 200";
    query.prepare(sql);
    if (!kw.isEmpty()) {
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

    // 获取在库实例ID列表
    QList<int> instanceIds;
    query.prepare("SELECT id FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '在库' "
                  "ORDER BY id ASC LIMIT :lim");
    query.bindValue(":pid", m_selectedPartId);
    query.bindValue(":lim", qty);
    DbManager::instance().executeQuery(query);
    while (query.next()) instanceIds << query.value(0).toInt();

    if (instanceIds.size() < qty) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "错误",
            QString("当前仅有 %1 件在库可退，退货数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取进价
    query.prepare("SELECT COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);
    double price = query.next() ? query.value(0).toDouble() : 0;

    // 逐个实例退货
    for (int instId : instanceIds) {
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_part_instance SET status = '已退货', "
                  "workorder_id = NULL, vehicle_id = NULL, recipient = NULL, "
                  "updated_at = NOW() WHERE id = :iid");
        u.bindValue(":iid", instId);
        DbManager::instance().executeQuery(u);

        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, "
                    "unit_price, total_price, operation_type, operator_id, remark) "
                    "VALUES (:pid, :iid, -1, :price, :total, '采购退货', :op, '采购退货')");
        log.bindValue(":pid", m_selectedPartId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", price);
        log.bindValue(":total", -price);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 更新库存缓存
    query.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    query.bindValue(":pid", m_selectedPartId);
    query.bindValue(":pid2", m_selectedPartId);
    DbManager::instance().executeQuery(query);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退货成功", "已退货 " + QString::number(qty) + " 个");
    onSearchPart();
}
