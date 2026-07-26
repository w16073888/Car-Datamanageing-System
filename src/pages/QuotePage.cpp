#include "QuotePage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>

QuotePage::QuotePage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

QuotePage::~QuotePage() {}

void QuotePage::refreshData()
{
    m_editOrderNo->clear();
    m_lblOrderInfo->setText("请加载工单");
    m_editPartName->clear();
    m_editQuantity->setText("1");
    m_editUnitPrice->clear();
    m_lblLaborFee->setText("工时费：¥0.00");
    m_lblMaterialTotal->setText("材料费：¥0.00");
    m_lblGrandTotal->setText("总报价：¥0.00");
    m_currentOrderId = 0;
    m_model->setQuery(QSqlQuery());
}

void QuotePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("报价管理");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 工单选择
    QHBoxLayout *selectLayout = new QHBoxLayout;
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("输入工单号");
    selectLayout->addWidget(m_editOrderNo);
    m_btnLoad = new QPushButton("加载工单");
    m_btnLoad->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    selectLayout->addWidget(m_btnLoad);
    selectLayout->addStretch();
    mainLayout->addLayout(selectLayout);

    m_lblOrderInfo = new QLabel("请加载工单");
    m_lblOrderInfo->setStyleSheet("padding: 8px; background: #f8f9fa; border-radius: 4px;");
    mainLayout->addWidget(m_lblOrderInfo);

    // 添加报价项
    QGroupBox *addGroup = new QGroupBox("添加报价项目");
    QHBoxLayout *addLayout = new QHBoxLayout(addGroup);
    addLayout->addWidget(new QLabel("备件："));
    m_editPartName = new QLineEdit;
    m_editPartName->setPlaceholderText("备件名称");
    addLayout->addWidget(m_editPartName);
    addLayout->addWidget(new QLabel("数量："));
    m_editQuantity = new QLineEdit;
    m_editQuantity->setFixedWidth(60);
    m_editQuantity->setText("1");
    addLayout->addWidget(m_editQuantity);
    addLayout->addWidget(new QLabel("单价："));
    m_editUnitPrice = new QLineEdit;
    m_editUnitPrice->setFixedWidth(100);
    m_editUnitPrice->setPlaceholderText("0.00");
    addLayout->addWidget(m_editUnitPrice);
    m_btnAdd = new QPushButton("添加");
    m_btnAdd->setStyleSheet("padding: 6px 14px; background: #27ae60; color: white; border-radius: 4px;");
    addLayout->addWidget(m_btnAdd);
    m_btnRemove = new QPushButton("删除选中");
    m_btnRemove->setStyleSheet("padding: 6px 14px; background: #e74c3c; color: white; border-radius: 4px;");
    addLayout->addWidget(m_btnRemove);
    mainLayout->addWidget(addGroup);

    // 明细表格
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    // 汇总
    QHBoxLayout *totalLayout = new QHBoxLayout;
    m_lblLaborFee = new QLabel("工时费：¥0.00");
    m_lblLaborFee->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 5px;");
    m_lblMaterialTotal = new QLabel("材料费：¥0.00");
    m_lblMaterialTotal->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 5px;");
    m_lblGrandTotal = new QLabel("总报价：¥0.00");
    m_lblGrandTotal->setStyleSheet("font-size: 16px; font-weight: bold; color: #e74c3c; padding: 5px;");
    totalLayout->addWidget(m_lblLaborFee);
    totalLayout->addWidget(m_lblMaterialTotal);
    totalLayout->addWidget(m_lblGrandTotal);
    totalLayout->addStretch();

    m_btnPrint = new QPushButton("打印报价单");
    m_btnPrint->setStyleSheet("padding: 8px 16px; background: #8e44ad; color: white; border-radius: 4px; font-weight: bold;");
    totalLayout->addWidget(m_btnPrint);
    mainLayout->addLayout(totalLayout);

    // 信号
    connect(m_btnLoad, &QPushButton::clicked, this, &QuotePage::onLoadOrder);
    connect(m_btnAdd, &QPushButton::clicked, this, &QuotePage::onAddItem);
    connect(m_btnRemove, &QPushButton::clicked, this, &QuotePage::onRemoveItem);
    connect(m_btnPrint, &QPushButton::clicked, this, &QuotePage::onPrint);
}

void QuotePage::onLoadOrder()
{
    QString orderNo = m_editOrderNo->text().trimmed();
    if (orderNo.isEmpty()) return;

    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT w.id, w.order_no, w.labor_fee, w.status, "
                  "v.plate_number FROM t_workorder w "
                  "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
                  "WHERE w.order_no = :no");
    query.bindValue(":no", orderNo);
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        m_currentOrderId = query.value(0).toInt();
        m_lblOrderInfo->setText(
            QString("工单：%1 | 车牌：%2 | 工时费：¥%3 | 状态：%4")
            .arg(query.value(1).toString(), query.value(4).toString(),
                 query.value(2).toString(), query.value(3).toString()));
        refreshItems();
    } else {
        QMessageBox::information(this, "未找到", "工单不存在");
    }
}

void QuotePage::onAddItem()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先加载工单");
        return;
    }
    QString name = m_editPartName->text().trimmed();
    int qty = m_editQuantity->text().toInt();
    double price = m_editUnitPrice->text().toDouble();
    if (name.isEmpty() || qty <= 0 || price <= 0) {
        QMessageBox::warning(this, "提示", "请填写完整的报价项目信息");
        return;
    }

    QSqlQuery query(DbManager::instance().database());
    query.prepare("INSERT INTO t_quote_item (workorder_id, part_name, quantity, unit_price) "
                  "VALUES (:oid, :name, :qty, :price)");
    query.bindValue(":oid", m_currentOrderId);
    query.bindValue(":name", name);
    query.bindValue(":qty", qty);
    query.bindValue(":price", price);
    if (DbManager::instance().executeQuery(query)) {
        refreshItems();
        m_editPartName->clear();
        m_editQuantity->setText("1");
        m_editUnitPrice->clear();
    }
}

void QuotePage::onRemoveItem()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) return;
    int id = m_model->data(m_model->index(idx.row(), 0)).toInt();
    QSqlQuery query(DbManager::instance().database());
    query.prepare("DELETE FROM t_quote_item WHERE id = :id");
    query.bindValue(":id", id);
    DbManager::instance().executeQuery(query);
    refreshItems();
}

void QuotePage::refreshItems()
{
    if (m_currentOrderId == 0) return;
    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT id, part_name AS '备件名称', quantity AS '数量', "
        "  unit_price AS '单价', subtotal AS '小计' "
        "FROM t_quote_item WHERE workorder_id = :oid");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    updateTotal();
}

void QuotePage::updateTotal()
{
    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT COALESCE(SUM(subtotal), 0) FROM t_quote_item WHERE workorder_id = :oid");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    double materialTotal = 0;
    if (query.next()) materialTotal = query.value(0).toDouble();

    query.prepare("SELECT labor_fee FROM t_workorder WHERE id = :id");
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    double laborFee = 0;
    if (query.next()) laborFee = query.value(0).toDouble();

    m_lblLaborFee->setText(QString("工时费：¥%1").arg(laborFee, 0, 'f', 2));
    m_lblMaterialTotal->setText(QString("材料费：¥%1").arg(materialTotal, 0, 'f', 2));
    m_lblGrandTotal->setText(QString("总报价：¥%1").arg(laborFee + materialTotal, 0, 'f', 2));
}

void QuotePage::onPrint()
{
    // 打印预览
    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString html = QString(
            "<h2 style='text-align:center;'>维修报价单</h2>"
            "<hr>"
            "<p><b>工单号：</b>%1</p>"
            "<p><b>工时费：</b>%2</p>"
            "<p><b>材料费合计：</b>%3</p>"
            "<p style='font-size:16px;color:#e74c3c;'><b>总报价：%4</b></p>"
            "<hr>"
            "<p style='color:#7f8c8d;font-size:12px;'>打印时间：%5</p>"
        ).arg(m_editOrderNo->text(),
              m_lblLaborFee->text(),
              m_lblMaterialTotal->text(),
              m_lblGrandTotal->text(),
              QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        doc.setHtml(html);
        doc.print(p);
    });
    preview.exec();
}
