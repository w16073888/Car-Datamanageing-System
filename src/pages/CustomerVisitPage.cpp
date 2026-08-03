#include "CustomerVisitPage.h"
#include "database/DbManager.h"
#include "database/Session.h"
#include "pages/QuotePage.h"

#include <QDialog>
#include <QTextBrowser>

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

    // 回访间隔天数（结算满该天数后才进入待回访列表）
    QHBoxLayout *daysRow = new QHBoxLayout;
    daysRow->addWidget(new QLabel("回访日期："));
    m_spinVisitDays = new QSpinBox;
    m_spinVisitDays->setRange(0, 365);
    m_spinVisitDays->setValue(7);
    m_spinVisitDays->setSuffix(" 天");
    m_spinVisitDays->setToolTip("结算日期距离今天大于等于该天数时，工单才出现在待回访列表");
    daysRow->addWidget(m_spinVisitDays);
    daysRow->addStretch();
    mainLayout->addLayout(daysRow);

    // 待回访列表
    QLabel *listLabel = new QLabel("待回访列表（双击查看工单信息）：");
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

    m_lblOrderInfo = new QLabel("请选择待回访工单");
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
    connect(m_tableView, &QTableView::doubleClicked, this, &CustomerVisitPage::onOrderDoubleClicked);
    connect(m_spinVisitDays, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { onLoadSettledOrders(); });

    onLoadSettledOrders();
}

void CustomerVisitPage::onLoadSettledOrders()
{
    QSqlQuery query(DbManager::instance().database());
    // 仅显示: 已结算 + 未回访 + 结算日期距今 >= 回访间隔天数
    QString sql = QString(
        "SELECT w.id, w.order_no AS '工单号', v.plate_number AS '车牌号', "
        "  w.total_amount AS '金额', s.settled_at AS '结算时间' "
        "FROM t_workorder w "
        "JOIN t_settlement s ON s.workorder_id = w.id "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "WHERE w.status = '已结算' "
        "  AND w.is_visited = '未回访' "
        "  AND DATE_ADD(s.settled_at, INTERVAL %1 DAY) <= NOW() "
        "ORDER BY s.settled_at DESC").arg(m_spinVisitDays->value());
    query.prepare(sql);
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
    // 检查是否已回访（is_visited 回访状态列）
    query.prepare("SELECT is_visited FROM t_workorder WHERE id = :oid");
    query.bindValue(":oid", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    if (query.next() && query.value(0).toString() == "已回访") {
        QMessageBox::information(this, "提示", "该工单已经回访过了");
        return;
    }

    // 满意度与备注写入 t_workorder，同时将回访状态置为已回访
    query.prepare("UPDATE t_workorder SET satisfaction = :sat, remark = :remark, "
                  "visitor_id = :vid, visited_at = NOW(), is_visited = '已回访' "
                  "WHERE id = :oid");
    query.bindValue(":sat", m_cmbSatisfaction->currentText());
    query.bindValue(":remark", m_textRemark->toPlainText());
    query.bindValue(":vid", Session::instance().userId());
    query.bindValue(":oid", m_currentOrderId);

    if (DbManager::instance().executeQuery(query) && query.numRowsAffected() > 0) {
        QMessageBox::information(this, "成功", "回访记录保存成功");
        m_textRemark->clear();
        m_currentOrderId = 0;
        m_lblOrderInfo->setText("请选择待回访工单");
        onLoadSettledOrders();   // 刷新列表：该工单已标记为已回访，从待回访列表移除
    } else {
        QMessageBox::warning(this, "失败", DbManager::instance().lastError());
    }
}

void CustomerVisitPage::onOrderDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    m_currentOrderId = m_model->data(m_model->index(index.row(), 0)).toInt();
    QString orderNo = m_model->data(m_model->index(index.row(), 1)).toString();

    // 用结算界面静态函数生成该工单的结算信息文档并展示
    QString html = QuotePage::buildSettlementHtmlFor(m_currentOrderId);
    if (html.isEmpty()) {
        QMessageBox::warning(this, "提示", "无法生成该工单的结算信息");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(QString("工单信息 - %1").arg(orderNo));
    dlg.resize(820, 640);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QTextBrowser *browser = new QTextBrowser;
    browser->setHtml(html);
    lay->addWidget(browser);
    dlg.exec();
}
