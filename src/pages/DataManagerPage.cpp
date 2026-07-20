#include "DataManagerPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

DataManagerPage::DataManagerPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

DataManagerPage::~DataManagerPage()
{
}

void DataManagerPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    QLabel *title = new QLabel("数据管理（经理权限）");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; padding: 5px;");
    mainLayout->addWidget(title);

    // 表选择区
    QHBoxLayout *tableSelectLayout = new QHBoxLayout;
    tableSelectLayout->addWidget(new QLabel("选择数据表："));
    m_tableSelector = new QComboBox;
    m_tableSelector->addItems({
        "t_vehicle", "t_customer", "t_workorder", "t_workorder_item",
        "t_quote_item", "t_parts", "t_inventory_log", "t_settlement",
        "t_return_visit", "t_employee", "t_system_log"
    });
    m_tableSelector->setMinimumWidth(200);
    tableSelectLayout->addWidget(m_tableSelector);

    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setStyleSheet(
        "QPushButton { padding: 6px 14px; border-radius: 4px;"
        "  background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }");
    tableSelectLayout->addWidget(m_btnRefresh);
    tableSelectLayout->addStretch();
    mainLayout->addLayout(tableSelectLayout);

    // 提示
    m_hintLabel = new QLabel("双击单元格可编辑内容，编辑后自动保存到数据库");
    m_hintLabel->setStyleSheet("color: #7f8c8d; font-size: 13px; padding: 3px;");
    mainLayout->addWidget(m_hintLabel);

    // 表格
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; gridline-color: #ecf0f1; }"
        "QHeaderView::section { background-color: #34495e; color: white;"
        "  padding: 6px; border: none; font-weight: bold; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange); // 双击编辑直接保存
    m_tableView->setModel(m_model);

    // 信号
    connect(m_tableSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataManagerPage::onTableSelected);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DataManagerPage::onRefresh);

    // 初始加载第一个表
    onTableSelected(0);
}

void DataManagerPage::onTableSelected(int index)
{
    Q_UNUSED(index)
    QString table = m_tableSelector->currentText();
    m_model->setTable(table);
    m_model->select();
    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("双击单元格编辑，当前表：%1，共 %2 行")
                         .arg(table).arg(m_model->rowCount()));
}

void DataManagerPage::onCellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
}

void DataManagerPage::onRefresh()
{
    m_model->select();
    m_tableView->resizeColumnsToContents();
}

void DataManagerPage::refreshData()
{
    onRefresh();
}
