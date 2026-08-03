#include "DataManagerPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
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

    QLabel *title = new QLabel("数据管理");
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
    m_tableSelector->addItem("系统日志表", "t_system_log");
    m_tableSelector->addItem("交易历史表", "t_vehicle_transaction");
    m_tableSelector->addItem("回访记录表", "回访记录");
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

    // 回访记录筛选行（仅选择"回访记录表"时显示）
    m_visitFilterRow = new QWidget;
    QHBoxLayout *visitFilterLayout = new QHBoxLayout(m_visitFilterRow);
    visitFilterLayout->setContentsMargins(0, 0, 0, 0);
    visitFilterLayout->setSpacing(8);
    visitFilterLayout->addWidget(new QLabel("回访时间："));
    m_visitDateRange = new DateRangeWidget;
    visitFilterLayout->addWidget(m_visitDateRange);
    visitFilterLayout->addSpacing(10);
    visitFilterLayout->addWidget(new QLabel("满意度："));
    m_cmbVisitSatisfaction = new QComboBox;
    m_cmbVisitSatisfaction->addItems({"全部", "满意", "一般", "不满意"});
    visitFilterLayout->addWidget(m_cmbVisitSatisfaction);
    visitFilterLayout->addStretch();
    m_visitFilterRow->setVisible(false);
    mainLayout->addWidget(m_visitFilterRow);

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
    m_queryModel = new QSqlQueryModel(this);

    // Ctrl+C 复制选中单元格
    m_tableView->installEventFilter(this);

    // 信号
    connect(m_tableSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataManagerPage::onTableSelected);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DataManagerPage::onRefresh);
    connect(m_btnDelete, &QPushButton::clicked, this, &DataManagerPage::onDelete);

    // 回访记录筛选变化 → 重新查询
    connect(m_visitDateRange, &DateRangeWidget::dateRangeChanged,
            this, [this](const QDate &, const QDate &) { loadVisitRecords(); });
    connect(m_cmbVisitSatisfaction, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { loadVisitRecords(); });

    // 初始加载第一个表
    onTableSelected(0);
}

void DataManagerPage::onTableSelected(int index)
{
    Q_UNUSED(index)
    QString pureTable = m_tableSelector->currentData().toString();

    // 回访记录表：只读查询视图 + 时间/满意度筛选 + 特殊删除（清除回访信息）
    if (pureTable == "回访记录") {
        m_visitFilterRow->setVisible(true);
        m_tableView->setModel(m_queryModel);
        loadVisitRecords();
        m_btnDelete->setVisible(Session::instance().canDeleteDataTable(pureTable));
        return;
    }

    // 普通数据表：QSqlTableModel 直接编辑
    m_visitFilterRow->setVisible(false);
    if (m_tableView->model() != m_model)
        m_tableView->setModel(m_model);
    m_model->setTable(pureTable);
    // 清除可能残留的过滤条件，确保显示全部记录
    m_model->setFilter("");
    m_model->select();
    setChineseHeaders();
    m_tableView->resizeColumnsToContents();

    // 按职位控制“删除选中行”按钮开放度
    bool canDel = Session::instance().canDeleteDataTable(pureTable);
    m_btnDelete->setVisible(canDel);
    m_hintLabel->setText(QString("双击单元格编辑，当前表：%1，共 %2 行 | %3")
                         .arg(m_tableSelector->currentText())
                         .arg(m_model->rowCount())
                         .arg(canDel ? "删除权限：本表允许删除"
                                     : "删除权限：当前职位无删除权限"));
}

void DataManagerPage::loadVisitRecords()
{
    QString sat = m_cmbVisitSatisfaction->currentText();

    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT w.id AS '工单ID', w.order_no AS '工单号', v.plate_number AS '车牌号', "
        "  w.satisfaction AS '满意度', w.remark AS '回访备注', "
        "  COALESCE(e.name,'') AS '回访人', w.visited_at AS '回访时间' "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_employee e ON e.id = w.visitor_id "
        "WHERE w.is_visited = '已回访' "
        "  AND DATE(w.visited_at) BETWEEN :start AND :end "
        + (sat == "全部" ? QString() : QString(" AND w.satisfaction = :sat"))
        + " ORDER BY w.visited_at DESC");
    query.bindValue(":start", m_visitDateRange->startDate().toString("yyyy-MM-dd"));
    query.bindValue(":end", m_visitDateRange->endDate().toString("yyyy-MM-dd"));
    if (sat != "全部")
        query.bindValue(":sat", sat);
    DbManager::instance().executeQuery(query);
    m_queryModel->setQuery(std::move(query));

    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("回访记录：%1 至 %2 | 满意度：%3 | 共 %4 条 | 删除=清除该工单回访信息")
                         .arg(m_visitDateRange->startDate().toString("yyyy-MM-dd"),
                              m_visitDateRange->endDate().toString("yyyy-MM-dd"),
                              sat)
                         .arg(m_queryModel->rowCount()));
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
    // t_workorder (28 columns)
    else if (table == "t_workorder") {
        m_model->setHeaderData(0,  Qt::Horizontal, "ID");
        m_model->setHeaderData(1,  Qt::Horizontal, "工单号");
        m_model->setHeaderData(2,  Qt::Horizontal, "车辆ID");
        m_model->setHeaderData(3,  Qt::Horizontal, "主修人ID");
        m_model->setHeaderData(4,  Qt::Horizontal, "机电主修");
        m_model->setHeaderData(5,  Qt::Horizontal, "钣金主修");
        m_model->setHeaderData(6,  Qt::Horizontal, "喷漆主修");
        m_model->setHeaderData(7,  Qt::Horizontal, "服务顾问");
        m_model->setHeaderData(8,  Qt::Horizontal, "公里数");
        m_model->setHeaderData(9,  Qt::Horizontal, "报修内容");
        m_model->setHeaderData(10, Qt::Horizontal, "工时费");
        m_model->setHeaderData(11, Qt::Horizontal, "材料费");
        m_model->setHeaderData(12, Qt::Horizontal, "其它费");
        m_model->setHeaderData(13, Qt::Horizontal, "管理费");
        m_model->setHeaderData(14, Qt::Horizontal, "总金额");
        m_model->setHeaderData(15, Qt::Horizontal, "订金");
        m_model->setHeaderData(16, Qt::Horizontal, "班别");
        m_model->setHeaderData(17, Qt::Horizontal, "主修人姓名");
        m_model->setHeaderData(18, Qt::Horizontal, "报修日期");
        m_model->setHeaderData(19, Qt::Horizontal, "预估完工");
        m_model->setHeaderData(20, Qt::Horizontal, "状态");
        m_model->setHeaderData(21, Qt::Horizontal, "满意度");
        m_model->setHeaderData(22, Qt::Horizontal, "回访备注");
        m_model->setHeaderData(23, Qt::Horizontal, "回访人ID");
        m_model->setHeaderData(24, Qt::Horizontal, "回访时间");
        m_model->setHeaderData(25, Qt::Horizontal, "创建人ID");
        m_model->setHeaderData(26, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(27, Qt::Horizontal, "更新时间");
        m_model->setHeaderData(28, Qt::Horizontal, "回访状态");
    }
    // t_workorder_item (10 columns)
    else if (table == "t_workorder_item") {
        m_model->setHeaderData(0, Qt::Horizontal, "ID");
        m_model->setHeaderData(1, Qt::Horizontal, "工单ID");
        m_model->setHeaderData(2, Qt::Horizontal, "备件ID");
        m_model->setHeaderData(3, Qt::Horizontal, "实例ID");
        m_model->setHeaderData(4, Qt::Horizontal, "备件名称");
        m_model->setHeaderData(5, Qt::Horizontal, "数量");
        m_model->setHeaderData(6, Qt::Horizontal, "单价");
        m_model->setHeaderData(7, Qt::Horizontal, "小计");
        m_model->setHeaderData(8, Qt::Horizontal, "项目类型");
        m_model->setHeaderData(9, Qt::Horizontal, "创建时间");
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
    // t_inventory_log (12 columns)
    else if (table == "t_inventory_log") {
        m_model->setHeaderData(0,  Qt::Horizontal, "ID");
        m_model->setHeaderData(1,  Qt::Horizontal, "备件ID");
        m_model->setHeaderData(2,  Qt::Horizontal, "实例ID");
        m_model->setHeaderData(3,  Qt::Horizontal, "数量");
        m_model->setHeaderData(4,  Qt::Horizontal, "单价");
        m_model->setHeaderData(5,  Qt::Horizontal, "总价");
        m_model->setHeaderData(6,  Qt::Horizontal, "操作类型");
        m_model->setHeaderData(7,  Qt::Horizontal, "关联单号");
        m_model->setHeaderData(8,  Qt::Horizontal, "操作人ID");
        m_model->setHeaderData(9,  Qt::Horizontal, "领取人");
        m_model->setHeaderData(10, Qt::Horizontal, "备注");
        m_model->setHeaderData(11, Qt::Horizontal, "操作时间");
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
    // 获取该行第一列数据作为标识（使用当前活动模型：回访记录表用的是查询模型）
    QAbstractItemModel *activeModel = m_tableView->model();
    QVariant id = activeModel->data(activeModel->index(row, 0));
    QString table = m_tableSelector->currentData().toString();
    QString tableDisplay = m_tableSelector->currentText();

    // 运行时权限校验（与按钮显隐一致，防止绕过）
    if (!Session::instance().canDeleteDataTable(table)) {
        QMessageBox::warning(this, "无权限",
            QString("当前职位无权删除「%1」中的记录").arg(tableDisplay));
        return;
    }

    // ========== 回访记录表特殊处理：删除=清除该工单的回访信息 ==========
    if (table == "回访记录") {
        QMessageBox::StandardButton r = QMessageBox::question(
            this, "确认删除",
            QString("确定要删除该回访记录吗？\n"
                    "将清除工单(ID=%1)的满意度、回访备注、回访人与回访时间，\n"
                    "并将其回访状态恢复为「未回访」。").arg(id.toString()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (r != QMessageBox::Yes)
            return;

        QSqlQuery up(DbManager::instance().database());
        up.prepare("UPDATE t_workorder SET satisfaction=NULL, remark=NULL, visitor_id=NULL, "
                   "visited_at=NULL, is_visited='未回访' WHERE id=:oid");
        up.bindValue(":oid", id.toInt());
        if (!DbManager::instance().executeQuery(up)) {
            QMessageBox::warning(this, "删除失败", DbManager::instance().lastError());
            return;
        }
        loadVisitRecords();
        return;
    }

    // ========== 工单表特殊校验（总经理可跳过） ==========
    if (table == "t_workorder") {
        int workorderId = id.toInt();
        bool isManager = (Session::instance().position() == "经理");

        if (!isManager) {
            // 1. 查询工单状态
            QSqlQuery q(DbManager::instance().database());
            q.prepare("SELECT status FROM t_workorder WHERE id = :id");
            q.bindValue(":id", workorderId);
            DbManager::instance().executeQuery(q);
            QString status;
            if (q.next()) status = q.value(0).toString();

            if (status != "已派工") {
                QMessageBox::warning(this, "无法删除",
                    QString("该工单当前状态为「%1」，仅「已派工」状态的工单允许删除。\n\n"
                            "工单状态流转后不可删除，请确认。").arg(status));
                return;
            }

            // 2. 检查是否与备件绑定（t_part_instance 或 t_workorder_item）
            q.prepare("SELECT COUNT(*) FROM t_part_instance WHERE workorder_id = :wid");
            q.bindValue(":wid", workorderId);
            DbManager::instance().executeQuery(q);
            int instanceCount = q.next() ? q.value(0).toInt() : 0;

            q.prepare("SELECT COUNT(*) FROM t_workorder_item WHERE workorder_id = :wid");
            q.bindValue(":wid", workorderId);
            DbManager::instance().executeQuery(q);
            int itemCount = q.next() ? q.value(0).toInt() : 0;

            if (instanceCount > 0 || itemCount > 0) {
                QMessageBox::warning(this, "无法删除",
                    QString("该工单已绑定备件，无法删除。\n\n"
                            "• 备件实例绑定: %1 条\n"
                            "• 工单备件明细: %2 条\n\n"
                            "请先退回已领出的备件后再删除工单。")
                    .arg(instanceCount).arg(itemCount));
                return;
            }
        }
    }

    // ========== 备件表特殊处理：删除备件前清理其相关日志与关联记录 ==========
    //   清理范围：库存流水日志(t_inventory_log)、备件实例(t_part_instance)、
    //   采购记录(t_part_purchase)，并将工单备件明细(t_workorder_item)的 part_id 解绑置空
    if (table == "t_parts") {
        int partId = id.toInt();

        QMessageBox::StandardButton partReply = QMessageBox::question(
            this, "确认删除",
            QString("确定要删除备件（ID=%1）吗？\n"
                    "将同时删除该备件相关的库存流水日志、备件实例与采购记录，\n"
                    "并解绑工单中的备件引用。\n此操作不可撤销！")
                .arg(id.toString()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (partReply != QMessageBox::Yes)
            return;

        DbManager::instance().beginTransaction();
        bool ok = true;

        // 1. 删除该备件的库存流水日志
        QSqlQuery delLog(DbManager::instance().database());
        delLog.prepare("DELETE FROM t_inventory_log WHERE part_id = :pid");
        delLog.bindValue(":pid", partId);
        if (!DbManager::instance().executeQuery(delLog)) ok = false;

        // 2. 删除该备件的备件实例
        if (ok) {
            QSqlQuery delInst(DbManager::instance().database());
            delInst.prepare("DELETE FROM t_part_instance WHERE part_id = :pid");
            delInst.bindValue(":pid", partId);
            if (!DbManager::instance().executeQuery(delInst)) ok = false;
        }

        // 3. 删除该备件的采购记录
        if (ok) {
            QSqlQuery delPur(DbManager::instance().database());
            delPur.prepare("DELETE FROM t_part_purchase WHERE part_id = :pid");
            delPur.bindValue(":pid", partId);
            if (!DbManager::instance().executeQuery(delPur)) ok = false;
        }

        // 4. 工单备件明细解绑（保留明细文本，置空 part_id 引用）
        if (ok) {
            QSqlQuery unlink(DbManager::instance().database());
            unlink.prepare("UPDATE t_workorder_item SET part_id = NULL WHERE part_id = :pid");
            unlink.bindValue(":pid", partId);
            if (!DbManager::instance().executeQuery(unlink)) ok = false;
        }

        // 5. 删除备件本体
        if (ok) {
            QSqlQuery delPart(DbManager::instance().database());
            delPart.prepare("DELETE FROM t_parts WHERE id = :pid");
            delPart.bindValue(":pid", partId);
            if (!DbManager::instance().executeQuery(delPart)) ok = false;
        }

        if (ok) {
            DbManager::instance().commitTransaction();
            m_model->select();
            m_tableView->resizeColumnsToContents();
            m_hintLabel->setText(QString("已删除备件及其相关日志，当前表：%1，共 %2 行")
                                 .arg(tableDisplay).arg(m_model->rowCount()));
        } else {
            DbManager::instance().rollbackTransaction();
            QMessageBox::warning(this, "删除失败",
                "删除备件失败：" + DbManager::instance().lastError());
        }
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认删除",
        QString("确定要删除「%1」中 ID 为 %2 的记录吗？\n此操作不可撤销！")
            .arg(tableDisplay).arg(id.toString()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (m_model->removeRow(row)) {
            m_model->submitAll();
            // 删除后重新查询，确保显示完整
            m_model->select();
            m_tableView->resizeColumnsToContents();
            m_hintLabel->setText(QString("已删除记录，当前表：%1，共 %2 行")
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
    // 回访记录视图下刷新查询模型，否则刷新普通表模型
    if (m_tableSelector->currentData().toString() == "回访记录") {
        loadVisitRecords();
        return;
    }
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
