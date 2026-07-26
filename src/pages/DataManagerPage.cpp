#include "DataManagerPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>

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
    m_tableSelector->addItem("员工表", "t_employee");
    m_tableSelector->addItem("车辆表", "t_vehicle");
    m_tableSelector->addItem("客户表", "t_customer");
    m_tableSelector->addItem("工单表", "t_workorder");
    m_tableSelector->addItem("工单明细表", "t_workorder_item");
    m_tableSelector->addItem("报价明细表", "t_quote_item");
    m_tableSelector->addItem("备件表", "t_parts");
    m_tableSelector->addItem("库存流水表", "t_inventory_log");
    m_tableSelector->addItem("结算表", "t_settlement");
    m_tableSelector->addItem("回访表", "t_return_visit");
    m_tableSelector->addItem("系统日志表", "t_system_log");
    m_tableSelector->addItem("交易历史表", "t_vehicle_transaction");
    m_tableSelector->setMinimumWidth(240);
    tableSelectLayout->addWidget(m_tableSelector);

    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setStyleSheet(
        "QPushButton { padding: 6px 14px; border-radius: 4px;"
        "  background-color: #3498db; color: white; }"
        "QPushButton:hover { background-color: #2980b9; }");
    tableSelectLayout->addWidget(m_btnRefresh);

    // 删除按钮
    m_btnDelete = new QPushButton("删除选中行");
    m_btnDelete->setStyleSheet(
        "QPushButton { padding: 6px 14px; border-radius: 4px;"
        "  background-color: #e74c3c; color: white; }"
        "QPushButton:hover { background-color: #c0392b; }");
    tableSelectLayout->addWidget(m_btnDelete);

    tableSelectLayout->addStretch();
    mainLayout->addLayout(tableSelectLayout);

    // 提示
    m_hintLabel = new QLabel("单击选单元格, Shift/Ctrl 多选, Ctrl+C 复制。双击单元格可编辑，编辑后自动保存");
    m_hintLabel->setStyleSheet("color: #7f8c8d; font-size: 13px; padding: 3px;");
    mainLayout->addWidget(m_hintLabel);

    // 表格
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; gridline-color: #ecf0f1; }"
        "QHeaderView::section { background-color: #34495e; color: white;"
        "  padding: 6px; border: none; font-weight: bold; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    m_tableView->setModel(m_model);

    // Ctrl+C 复制选中单元格
    m_tableView->installEventFilter(this);

    // 信号
    connect(m_tableSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataManagerPage::onTableSelected);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DataManagerPage::onRefresh);
    connect(m_btnDelete, &QPushButton::clicked, this, &DataManagerPage::onDelete);

    // 初始加载第一个表
    onTableSelected(0);
}

void DataManagerPage::onTableSelected(int index)
{
    Q_UNUSED(index)
    QString pureTable = m_tableSelector->currentData().toString();
    m_model->setTable(pureTable);
    m_model->select();
    setChineseHeaders();
    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("双击单元格编辑，当前表：%1，共 %2 行")
                         .arg(m_tableSelector->currentText()).arg(m_model->rowCount()));
}

void DataManagerPage::setChineseHeaders()
{
    QString table = m_tableSelector->currentData().toString();

    // 为每个表设置中文列头
    // t_employee
    if (table == "t_employee") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工号");
        m_model->setHeaderData(2, Qt::Horizontal, "姓名");
        m_model->setHeaderData(3, Qt::Horizontal, "密码");
        m_model->setHeaderData(4, Qt::Horizontal, "职位");
        m_model->setHeaderData(5, Qt::Horizontal, "电话");
        m_model->setHeaderData(6, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(7, Qt::Horizontal, "更新时间");
        m_model->setHeaderData(8, Qt::Horizontal, "是否启用");
    }
    // t_vehicle
    else if (table == "t_vehicle") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "车牌号");
        m_model->setHeaderData(2, Qt::Horizontal, "车架号(VIN)");
        m_model->setHeaderData(3, Qt::Horizontal, "发动机号");
        m_model->setHeaderData(4, Qt::Horizontal, "厂家/品牌");
        m_model->setHeaderData(5, Qt::Horizontal, "车型/型号");
        m_model->setHeaderData(6, Qt::Horizontal, "颜色");
        m_model->setHeaderData(7, Qt::Horizontal, "燃油类型");
        m_model->setHeaderData(8, Qt::Horizontal, "变速箱");
        m_model->setHeaderData(9, Qt::Horizontal, "地区");
        m_model->setHeaderData(10, Qt::Horizontal, "当前公里数");
        m_model->setHeaderData(11, Qt::Horizontal, "购车日期");
        m_model->setHeaderData(12, Qt::Horizontal, "年审日期");
        m_model->setHeaderData(13, Qt::Horizontal, "保险日期");
        m_model->setHeaderData(14, Qt::Horizontal, "最后保养日期");
        m_model->setHeaderData(15, Qt::Horizontal, "保养公里数");
        m_model->setHeaderData(16, Qt::Horizontal, "最后光顾日期");
        m_model->setHeaderData(17, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(18, Qt::Horizontal, "更新时间");
    }
    // t_customer
    else if (table == "t_customer") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "关联车辆ID");
        m_model->setHeaderData(2, Qt::Horizontal, "姓名");
        m_model->setHeaderData(3, Qt::Horizontal, "电话");
        m_model->setHeaderData(4, Qt::Horizontal, "地址");
        m_model->setHeaderData(5, Qt::Horizontal, "类型");
        m_model->setHeaderData(6, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(7, Qt::Horizontal, "更新时间");
    }
    // t_workorder
    else if (table == "t_workorder") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单号");
        m_model->setHeaderData(2, Qt::Horizontal, "车辆ID");
        m_model->setHeaderData(3, Qt::Horizontal, "维修责任人ID");
        m_model->setHeaderData(4, Qt::Horizontal, "公里数");
        m_model->setHeaderData(5, Qt::Horizontal, "报修内容");
        m_model->setHeaderData(6, Qt::Horizontal, "预估工时费");
        m_model->setHeaderData(7, Qt::Horizontal, "总金额");
        m_model->setHeaderData(8, Qt::Horizontal, "状态");
        m_model->setHeaderData(9, Qt::Horizontal, "创建人ID");
        m_model->setHeaderData(10, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(11, Qt::Horizontal, "更新时间");
    }
    // t_workorder_item
    else if (table == "t_workorder_item") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(2, Qt::Horizontal, "备件ID");
        m_model->setHeaderData(3, Qt::Horizontal, "备件名称");
        m_model->setHeaderData(4, Qt::Horizontal, "数量");
        m_model->setHeaderData(5, Qt::Horizontal, "单价");
        m_model->setHeaderData(6, Qt::Horizontal, "小计");
        m_model->setHeaderData(7, Qt::Horizontal, "项目类型");
        m_model->setHeaderData(8, Qt::Horizontal, "创建时间");
    }
    // t_quote_item
    else if (table == "t_quote_item") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(2, Qt::Horizontal, "备件名称");
        m_model->setHeaderData(3, Qt::Horizontal, "数量");
        m_model->setHeaderData(4, Qt::Horizontal, "单价");
        m_model->setHeaderData(5, Qt::Horizontal, "小计");
        m_model->setHeaderData(6, Qt::Horizontal, "创建时间");
    }
    // t_parts
    else if (table == "t_parts") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "备件编号");
        m_model->setHeaderData(2, Qt::Horizontal, "备件名称");
        m_model->setHeaderData(3, Qt::Horizontal, "规格型号");
        m_model->setHeaderData(4, Qt::Horizontal, "库存量");
        m_model->setHeaderData(5, Qt::Horizontal, "进货价");
        m_model->setHeaderData(6, Qt::Horizontal, "销售价");
        m_model->setHeaderData(7, Qt::Horizontal, "供应商");
        m_model->setHeaderData(8, Qt::Horizontal, "质保期");
        m_model->setHeaderData(9, Qt::Horizontal, "适用车型");
        m_model->setHeaderData(10, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(11, Qt::Horizontal, "更新时间");
    }
    // t_inventory_log
    else if (table == "t_inventory_log") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "备件ID");
        m_model->setHeaderData(2, Qt::Horizontal, "数量");
        m_model->setHeaderData(3, Qt::Horizontal, "单价");
        m_model->setHeaderData(4, Qt::Horizontal, "总价");
        m_model->setHeaderData(5, Qt::Horizontal, "操作类型");
        m_model->setHeaderData(6, Qt::Horizontal, "关联单号");
        m_model->setHeaderData(7, Qt::Horizontal, "操作人ID");
        m_model->setHeaderData(8, Qt::Horizontal, "备注");
        m_model->setHeaderData(9, Qt::Horizontal, "操作时间");
    }
    // t_settlement
    else if (table == "t_settlement") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(2, Qt::Horizontal, "工时费");
        m_model->setHeaderData(3, Qt::Horizontal, "材料费");
        m_model->setHeaderData(4, Qt::Horizontal, "总金额");
        m_model->setHeaderData(5, Qt::Horizontal, "结算人ID");
        m_model->setHeaderData(6, Qt::Horizontal, "结算时间");
    }
    // t_return_visit
    else if (table == "t_return_visit") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(2, Qt::Horizontal, "满意度");
        m_model->setHeaderData(3, Qt::Horizontal, "备注");
        m_model->setHeaderData(4, Qt::Horizontal, "回访人ID");
        m_model->setHeaderData(5, Qt::Horizontal, "回访时间");
    }
    // t_system_log
    else if (table == "t_system_log") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "操作人ID");
        m_model->setHeaderData(2, Qt::Horizontal, "操作类型");
        m_model->setHeaderData(3, Qt::Horizontal, "操作表名");
        m_model->setHeaderData(4, Qt::Horizontal, "操作记录ID");
        m_model->setHeaderData(5, Qt::Horizontal, "操作详情");
        m_model->setHeaderData(6, Qt::Horizontal, "操作时间");
    }
    // t_vehicle_transaction
    else if (table == "t_vehicle_transaction") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "车辆ID");
        m_model->setHeaderData(2, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(3, Qt::Horizontal, "交易类型");
        m_model->setHeaderData(4, Qt::Horizontal, "描述");
        m_model->setHeaderData(5, Qt::Horizontal, "金额");
        m_model->setHeaderData(6, Qt::Horizontal, "操作人ID");
        m_model->setHeaderData(7, Qt::Horizontal, "交易时间");
    }
}

void DataManagerPage::onCellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
}

void DataManagerPage::onDelete()
{
    // 获取当前选中行
    QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, "提示", "请先选择要删除的行");
        return;
    }

    int row = index.row();
    // 获取该行第一列数据作为标识
    QVariant id = m_model->data(m_model->index(row, 0));

    QString tableDisplay = m_tableSelector->currentText();
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除「%1」中 ID 为 %2 的记录吗？\n此操作不可撤销！")
            .arg(tableDisplay).arg(id.toString()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_model->removeRow(row)) {
            m_model->submitAll();
            m_hintLabel->setText(QString("已删除 ID 为 %1 的记录，当前表：%2，共 %3 行")
                                 .arg(id.toString())
                                 .arg(tableDisplay)
                                 .arg(m_model->rowCount()));
        } else {
            QMessageBox::warning(this, "删除失败",
                "删除记录时发生错误：" + m_model->lastError().text());
        }
    }
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

QString DataManagerPage::tableName() const
{
    return m_tableSelector->currentData().toString();
}

bool DataManagerPage::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_tableView && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Copy)) {
            // 收集选中单元格，构造制表符分隔的文本
            QModelIndexList indexes = m_tableView->selectionModel()->selectedIndexes();
            if (indexes.isEmpty())
                return true;

            // 按 (row, column) 排序
            std::sort(indexes.begin(), indexes.end(),
                [](const QModelIndex &a, const QModelIndex &b) {
                    return a.row() < b.row()
                        || (a.row() == b.row() && a.column() < b.column());
                });

            QString text;
            int prevRow = indexes.first().row();
            for (const QModelIndex &idx : indexes) {
                if (idx.row() != prevRow)
                    text += '\n';
                else if (!text.isEmpty() && text.back() != '\n')
                    text += '\t';
                text += idx.data(Qt::DisplayRole).toString();
                prevRow = idx.row();
            }

            QApplication::clipboard()->setText(text);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
