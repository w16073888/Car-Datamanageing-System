#include "PartsReturnPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

PartsReturnPage::PartsReturnPage(QWidget *parent) : QWidget(parent) { setupUI(); }
PartsReturnPage::~PartsReturnPage() {}

void PartsReturnPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("备件退库");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel("选择已出库工单："));
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("输入工单号");
    topLayout->addWidget(m_editOrderNo, 1);
    m_btnLoad = new QPushButton("加载出库记录");
    m_btnLoad->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    topLayout->addWidget(m_btnLoad);
    mainLayout->addLayout(topLayout);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    QGroupBox *opGroup = new QGroupBox("退库操作");
    QHBoxLayout *opLayout = new QHBoxLayout(opGroup);
    m_lblSelectedPart = new QLabel("请选择一条出库记录");
    opLayout->addWidget(m_lblSelectedPart, 1);
    opLayout->addWidget(new QLabel("退库数量："));
    m_spinReturnQty = new QSpinBox;
    m_spinReturnQty->setRange(1, 99999);
    opLayout->addWidget(m_spinReturnQty);
    m_btnReturn = new QPushButton("确认退库");
    m_btnReturn->setStyleSheet("padding: 6px 14px; background: #27ae60; color: white; border-radius: 4px; font-weight: bold;");
    opLayout->addWidget(m_btnReturn);
    mainLayout->addWidget(opGroup);

    connect(m_btnLoad, &QPushButton::clicked, this, &PartsReturnPage::onLoadOutRecords);
    connect(m_btnReturn, &QPushButton::clicked, this, &PartsReturnPage::onReturnPart);
    connect(m_tableView, &QTableView::clicked, [this](const QModelIndex &idx) {
        if (!idx.isValid()) return;
        int row = idx.row();
        int outQty = m_model->data(m_model->index(row, 2)).toInt();
        QString partName = m_model->data(m_model->index(row, 3)).toString();
        m_selectedItemId = m_model->data(m_model->index(row, 0)).toInt();
        m_selectedPartId = m_model->data(m_model->index(row, 1)).toInt();
        m_maxReturnQty = outQty;
        m_lblSelectedPart->setText(QString("备件：%1 | 原出库数量：%2").arg(partName).arg(outQty));
        m_spinReturnQty->setMaximum(outQty);
    });
}

void PartsReturnPage::onLoadOutRecords()
{
    QString orderNo = m_editOrderNo->text().trimmed();
    if (orderNo.isEmpty()) return;

    // 仅搜索已经被工人领出的备件（实例状态为已领出或已安装）
    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT wi.id, wi.part_id, wi.quantity AS '出库数量', "
        "  wi.part_name AS '备件名称', wi.unit_price AS '单价', wi.subtotal AS '小计' "
        "FROM t_workorder_item wi "
        "JOIN t_workorder w ON w.id = wi.workorder_id "
        "JOIN t_part_instance i ON i.id = wi.part_instance_id "
        "WHERE w.order_no = :no AND wi.item_type = '材料' "
        "AND i.status IN ('已领出','已安装')");
    query.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
}

void PartsReturnPage::onReturnPart()
{
    if (m_selectedItemId == 0) {
        QMessageBox::warning(this, "提示", "请选择一条出库记录");
        return;
    }
    int qty = m_spinReturnQty->value();
    if (qty > m_maxReturnQty) {
        QMessageBox::warning(this, "错误", "退库数量不能超过原出库数量");
        return;
    }

    DbManager::instance().beginTransaction();
    QSqlQuery query(DbManager::instance().database());

    // 获取该工单明细关联的已领出/已安装实例
    QList<int> instanceIds;
    query.prepare("SELECT part_instance_id FROM t_workorder_item WHERE id = :id");
    query.bindValue(":id", m_selectedItemId);
    DbManager::instance().executeQuery(query);
    if (query.next() && !query.isNull(0)) {
        // 获取该工单号下所有符合条件的实例
        QString orderNo = m_editOrderNo->text().trimmed();
        query.prepare("SELECT i.id FROM t_part_instance i "
                      "JOIN t_workorder_item wi ON wi.part_instance_id = i.id "
                      "JOIN t_workorder w ON w.id = wi.workorder_id "
                      "WHERE w.order_no = :no AND i.part_id = :pid "
                      "AND i.status IN ('已领出','已安装') "
                      "ORDER BY i.status = '已领出' DESC LIMIT :lim");
        query.bindValue(":no", orderNo);
        query.bindValue(":pid", m_selectedPartId);
        query.bindValue(":lim", qty);
        DbManager::instance().executeQuery(query);
        while (query.next()) instanceIds << query.value(0).toInt();
    }

    if (instanceIds.size() < qty) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "退库失败",
            QString("可退库的备件仅 %1 件，退库数量不能超过 %1").arg(instanceIds.size()));
        return;
    }

    // 获取备件信息用于记录流水
    query.prepare("SELECT COALESCE(purchase_price, 0) FROM t_parts WHERE id = :id");
    query.bindValue(":id", m_selectedPartId);
    DbManager::instance().executeQuery(query);
    double costPrice = query.next() ? query.value(0).toDouble() : 0;

    QString orderNo = m_editOrderNo->text().trimmed();

    // 逐个实例退库
    for (int instId : instanceIds) {
        QSqlQuery u(DbManager::instance().database());
        u.prepare("UPDATE t_part_instance SET status = '在库', vehicle_id = NULL, "
                  "workorder_id = NULL, recipient = NULL, updated_at = NOW() "
                  "WHERE id = :iid");
        u.bindValue(":iid", instId);
        DbManager::instance().executeQuery(u);

        QSqlQuery log(DbManager::instance().database());
        log.prepare("INSERT INTO t_inventory_log (part_id, part_instance_id, quantity, "
                    "unit_price, total_price, operation_type, ref_order_no, "
                    "operator_id, remark) "
                    "VALUES (:pid, :iid, 1, :price, :total, '备件退库', :ref, :op, '备件退库')");
        log.bindValue(":pid", m_selectedPartId);
        log.bindValue(":iid", instId);
        log.bindValue(":price", costPrice);
        log.bindValue(":total", costPrice);
        log.bindValue(":ref", orderNo.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : orderNo);
        log.bindValue(":op", Session::instance().userId());
        DbManager::instance().executeQuery(log);
    }

    // 更新工单明细中的数量
    query.prepare("UPDATE t_workorder_item SET quantity = quantity - :qty WHERE id = :id");
    query.bindValue(":qty", qty);
    query.bindValue(":id", m_selectedItemId);
    DbManager::instance().executeQuery(query);

    // 如果工单明细数量归零则删除
    query.prepare("DELETE FROM t_workorder_item WHERE id = :id AND quantity <= 0");
    query.bindValue(":id", m_selectedItemId);
    DbManager::instance().executeQuery(query);

    // 更新库存缓存
    query.prepare("UPDATE t_parts SET stock = (SELECT COUNT(*) FROM t_part_instance "
                  "WHERE part_id = :pid AND status = '在库') WHERE id = :pid2");
    query.bindValue(":pid", m_selectedPartId);
    query.bindValue(":pid2", m_selectedPartId);
    DbManager::instance().executeQuery(query);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "退库成功", "退库完成，库存已更新");
    onLoadOutRecords();
}
