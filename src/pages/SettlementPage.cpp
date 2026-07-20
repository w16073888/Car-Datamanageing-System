#include "SettlementPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextDocument>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QDateTime>

SettlementPage::SettlementPage(QWidget *parent) : QWidget(parent) { setupUI(); }
SettlementPage::~SettlementPage() {}

void SettlementPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("工单结算");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QHBoxLayout *loadLayout = new QHBoxLayout;
    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setPlaceholderText("输入工单号");
    loadLayout->addWidget(m_editOrderNo, 1);
    m_btnLoad = new QPushButton("加载工单");
    m_btnLoad->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    loadLayout->addWidget(m_btnLoad);
    mainLayout->addLayout(loadLayout);

    m_lblOrderInfo = new QLabel("请加载已完工的工单");
    m_lblOrderInfo->setStyleSheet("padding: 8px; background: #f8f9fa; border-radius: 4px;");
    mainLayout->addWidget(m_lblOrderInfo);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    QHBoxLayout *totalLayout = new QHBoxLayout;
    m_lblLaborFee = new QLabel("工时费：¥0.00");
    m_lblMaterialFee = new QLabel("材料费：¥0.00");
    m_lblTotal = new QLabel("总计：¥0.00");
    m_lblTotal->setStyleSheet("font-size: 16px; font-weight: bold; color: #e74c3c;");
    totalLayout->addWidget(m_lblLaborFee);
    totalLayout->addWidget(m_lblMaterialFee);
    totalLayout->addWidget(m_lblTotal);
    totalLayout->addStretch();

    m_btnSettle = new QPushButton("确认结算");
    m_btnSettle->setStyleSheet("padding: 8px 16px; background: #27ae60; color: white; border-radius: 4px; font-weight: bold;");
    totalLayout->addWidget(m_btnSettle);
    m_btnPrint = new QPushButton("打印结算单");
    m_btnPrint->setStyleSheet("padding: 8px 16px; background: #8e44ad; color: white; border-radius: 4px;");
    totalLayout->addWidget(m_btnPrint);
    mainLayout->addLayout(totalLayout);

    connect(m_btnLoad, &QPushButton::clicked, this, &SettlementPage::onLoadOrder);
    connect(m_btnSettle, &QPushButton::clicked, this, &SettlementPage::onSettle);
    connect(m_btnPrint, &QPushButton::clicked, this, &SettlementPage::onPrint);
}

void SettlementPage::onLoadOrder()
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

    if (!query.next()) {
        QMessageBox::information(this, "未找到", "工单不存在");
        return;
    }

    m_currentOrderId = query.value(0).toInt();
    QString status = query.value(3).toString();
    m_lblOrderInfo->setText(
        QString("工单：%1 | 车牌：%2 | 工时费：¥%3 | 状态：%4")
        .arg(query.value(1).toString(), query.value(4).toString(),
             query.value(2).toString(), status));

    if (status != "已完工" && status != "已结算") {
        QMessageBox::warning(this, "状态错误", "只有已完工的工单可以结算");
        return;
    }

    // 加载材料明细
    query.prepare("SELECT part_name AS '备件名称', quantity AS '数量', "
                  "unit_price AS '单价', subtotal AS '小计' "
                  "FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料'");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));

    // 汇总
    double laborFee = 0;
    query.prepare("SELECT labor_fee FROM t_workorder WHERE id = :id");
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    if (query.next()) laborFee = query.value(0).toDouble();

    query.prepare("SELECT COALESCE(SUM(subtotal), 0) FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料'");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    double materialTotal = 0;
    if (query.next()) materialTotal = query.value(0).toDouble();

    m_lblLaborFee->setText(QString("工时费：¥%1").arg(laborFee, 0, 'f', 2));
    m_lblMaterialFee->setText(QString("材料费：¥%1").arg(materialTotal, 0, 'f', 2));
    m_lblTotal->setText(QString("总计：¥%1").arg(laborFee + materialTotal, 0, 'f', 2));
}

void SettlementPage::onSettle()
{
    if (m_currentOrderId == 0) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认结算", "确认结算该工单？结算后不可修改。",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT labor_fee FROM t_workorder WHERE id = :id");
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    double laborFee = 0;
    if (query.next()) laborFee = query.value(0).toDouble();

    query.prepare("SELECT COALESCE(SUM(subtotal), 0) FROM t_workorder_item WHERE workorder_id = :oid AND item_type = '材料'");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    double materialTotal = 0;
    if (query.next()) materialTotal = query.value(0).toDouble();

    double total = laborFee + materialTotal;

    DbManager::instance().beginTransaction();

    // 更新工单状态
    query.prepare("UPDATE t_workorder SET status = '已结算', total_amount = :total WHERE id = :id");
    query.bindValue(":total", total);
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);

    // 插入结算记录
    query.prepare("INSERT INTO t_settlement (workorder_id, labor_fee, material_fee, total_amount, settled_by) "
                  "VALUES (:oid, :labor, :material, :total, :by)");
    query.bindValue(":oid", m_currentOrderId);
    query.bindValue(":labor", laborFee);
    query.bindValue(":material", materialTotal);
    query.bindValue(":total", total);
    query.bindValue(":by", Session::instance().userId());
    DbManager::instance().executeQuery(query);

    // 更新车辆最后光顾日期
    query.prepare("UPDATE t_vehicle v "
                  "JOIN t_workorder w ON w.vehicle_id = v.id "
                  "SET v.last_visit_date = CURDATE() WHERE w.id = :id");
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "结算成功",
        QString("工单结算完成，总金额：¥%1").arg(total, 0, 'f', 2));
}

void SettlementPage::onPrint()
{
    QPrinter printer;
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString html = QString(
            "<div style='text-align:center;'>"
            "<h2>维修结算单</h2>"
            "<hr></div>"
            "<p><b>工单号：</b>%1</p>"
            "<p><b>工时费：</b>%2</p>"
            "<p><b>材料费：</b>%3</p>"
            "<p><b>结算金额：</b>%4</p>"
            "<hr>"
            "<p style='color:#7f8c8d;font-size:12px;'>打印时间：%5</p>"
        ).arg(m_editOrderNo->text(),
              m_lblLaborFee->text(),
              m_lblMaterialFee->text(),
              m_lblTotal->text(),
              QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        doc.setHtml(html);
        doc.print(p);
    });
    preview.exec();
}
