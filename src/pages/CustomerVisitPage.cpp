#include "CustomerVisitPage.h"
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

CustomerVisitPage::CustomerVisitPage(QWidget *parent) : QWidget(parent) { setupUI(); }
CustomerVisitPage::~CustomerVisitPage() {}

void CustomerVisitPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("客户回访");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 已结算工单列表
    QLabel *listLabel = new QLabel("已结算工单（双击选择）：");
    mainLayout->addWidget(listLabel);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    // 回访录入
    QGroupBox *visitGroup = new QGroupBox("回访录入");
    QVBoxLayout *vLayout = new QVBoxLayout(visitGroup);

    m_lblOrderInfo = new QLabel("请选择一个已结算工单");
    m_lblOrderInfo->setStyleSheet("padding: 5px; background: #f8f9fa; border-radius: 4px;");
    vLayout->addWidget(m_lblOrderInfo);

    QHBoxLayout *satLayout = new QHBoxLayout;
    satLayout->addWidget(new QLabel("满意度："));
    m_cmbSatisfaction = new QComboBox;
    m_cmbSatisfaction->addItems({"满意", "一般", "不满意"});
    satLayout->addWidget(m_cmbSatisfaction);
    satLayout->addStretch();
    vLayout->addLayout(satLayout);

    vLayout->addWidget(new QLabel("备注："));
    m_textRemark = new QTextEdit;
    m_textRemark->setPlaceholderText("请输入回访备注...");
    m_textRemark->setMaximumHeight(80);
    vLayout->addWidget(m_textRemark);

    m_btnSave = new QPushButton("保存回访记录");
    m_btnSave->setStyleSheet("padding: 8px; background: #27ae60; color: white; border-radius: 4px; font-weight: bold;");
    vLayout->addWidget(m_btnSave);
    mainLayout->addWidget(visitGroup);

    connect(m_btnSave, &QPushButton::clicked, this, &CustomerVisitPage::onSaveVisit);
    connect(m_tableView, &QTableView::clicked, this, &CustomerVisitPage::onSelectOrder);

    onLoadSettledOrders();
}

void CustomerVisitPage::onLoadSettledOrders()
{
    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT w.id, w.order_no AS '工单号', v.plate_number AS '车牌号', "
        "  w.total_amount AS '金额', w.created_at AS '创建时间', s.settled_at AS '结算时间' "
        "FROM t_workorder w "
        "JOIN t_settlement s ON s.workorder_id = w.id "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "ORDER BY s.settled_at DESC");
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
}

void CustomerVisitPage::onSelectOrder(const QModelIndex &index)
{
    if (!index.isValid()) return;
    m_currentOrderId = m_model->data(m_model->index(index.row(), 0)).toInt();
    QString orderNo = m_model->data(m_model->index(index.row(), 1)).toString();
    m_lblOrderInfo->setText(QString("工单号：%1 | 车牌：%2 | 金额：%3")
                           .arg(orderNo,
                                m_model->data(m_model->index(index.row(), 2)).toString(),
                                m_model->data(m_model->index(index.row(), 3)).toString()));
}

void CustomerVisitPage::onSaveVisit()
{
    if (m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先选择一个已结算工单");
        return;
    }

    QSqlQuery query(DbManager::instance().database());
    // 检查是否已回访
    query.prepare("SELECT COUNT(*) FROM t_return_visit WHERE workorder_id = :oid");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    if (query.next() && query.value(0).toInt() > 0) {
        QMessageBox::information(this, "提示", "该工单已经回访过了");
        return;
    }

    query.prepare("INSERT INTO t_return_visit (workorder_id, satisfaction, remark, visitor_id) "
                  "VALUES (:oid, :sat, :remark, :vid)");
    query.bindValue(":oid", m_currentOrderId);
    query.bindValue(":sat", m_cmbSatisfaction->currentText());
    query.bindValue(":remark", m_textRemark->toPlainText());
    query.bindValue(":vid", Session::instance().userId());

    if (DbManager::instance().executeQuery(query)) {
        QMessageBox::information(this, "成功", "回访记录保存成功");
        m_textRemark->clear();
    } else {
        QMessageBox::warning(this, "失败", DbManager::instance().lastError());
    }
}
