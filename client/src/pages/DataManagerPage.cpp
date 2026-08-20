#include "DataManagerPage.h"
#include "database/Session.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteModel.h"
#include "remote/RemoteDb.h"
#include "utils/XlsxExporter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>
#include <QClipboard>
#include <QApplication>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDateTime>
#include <QSignalBlocker>

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
    m_tableSelector->addItem("工单表", "t_workorder");
    m_tableSelector->addItem("备件表", "t_parts");
    m_tableSelector->addItem("库存流水表", "t_inventory_log");
    m_tableSelector->addItem("结算表", "t_settlement");
    m_tableSelector->addItem("系统日志表", "t_system_log");
    m_tableSelector->addItem("操作日志表", "操作日志");
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

    // 导出 Excel 按钮（导出当前显示的数据表/视图）
    m_btnExport = new QPushButton("导出Excel");
    m_btnExport->setStyleSheet(
        "QPushButton { padding: 6px 14px; border-radius: 4px;"
        "  background-color: #27ae60; color: white; }"
        "QPushButton:hover { background-color: #229954; }");
    tableSelectLayout->addWidget(m_btnExport);

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

    // 系统日志筛选行（仅选择"系统日志表"时显示，只读查询）
    m_logFilterRow = new QWidget;
    QHBoxLayout *logFilterLayout = new QHBoxLayout(m_logFilterRow);
    logFilterLayout->setContentsMargins(0, 0, 0, 0);
    logFilterLayout->setSpacing(8);
    logFilterLayout->addWidget(new QLabel("操作时间："));
    m_logDateRange = new DateRangeWidget;
    logFilterLayout->addWidget(m_logDateRange);
    logFilterLayout->addSpacing(10);
    logFilterLayout->addWidget(new QLabel("操作类型："));
    m_cmbLogAction = new QComboBox;
    m_cmbLogAction->addItems({"全部", "insert", "update", "delete"});
    logFilterLayout->addWidget(m_cmbLogAction);
    logFilterLayout->addStretch();
    m_logFilterRow->setVisible(false);
    mainLayout->addWidget(m_logFilterRow);

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

    m_model = new RemoteModel(this);
    m_model->setEditStrategy(RemoteModel::OnFieldChange);
    m_tableView->setModel(m_model);
    m_queryModel = new RemoteModel(this);

    // Ctrl+C 复制选中单元格
    m_tableView->installEventFilter(this);

    // 信号
    connect(m_tableSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataManagerPage::onTableSelected);
    connect(m_btnRefresh, &QPushButton::clicked, this, &DataManagerPage::onRefresh);
    connect(m_btnDelete, &QPushButton::clicked, this, &DataManagerPage::onDelete);
    connect(m_btnExport, &QPushButton::clicked, this, &DataManagerPage::onExport);

    // 回访记录筛选变化 → 重新查询
    connect(m_visitDateRange, &DateRangeWidget::dateRangeChanged,
            this, [this](const QDate &, const QDate &) { loadVisitRecords(); });
    connect(m_cmbVisitSatisfaction, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { loadVisitRecords(); });

    // 系统日志/操作日志筛选变化 → 按当前选中表重新查询
    connect(m_logDateRange, &DateRangeWidget::dateRangeChanged,
            this, [this](const QDate &, const QDate &) { onLogFilterChanged(); });
    connect(m_cmbLogAction, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onLogFilterChanged(); });

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
        m_logFilterRow->setVisible(false);
        m_tableView->setModel(m_queryModel);
        loadVisitRecords();
        m_btnDelete->setVisible(Session::instance().canDeleteDataTable(pureTable));
        return;
    }

    // 系统日志表 / 操作日志表：只读查询视图 + 时间/操作类型筛选（审计日志禁止删除）
    //   两者内容平行（同一 t_system_log 数据源）；操作日志表仅将英文操作类型/表名译为中文
    if (pureTable == "t_system_log" || pureTable == "操作日志") {
        m_visitFilterRow->setVisible(false);
        m_logFilterRow->setVisible(true);
        if (m_tableView->model() != m_queryModel)
            m_tableView->setModel(m_queryModel);
        setupLogActionFilter(pureTable == "操作日志");
        if (pureTable == "操作日志")
            loadOperationLog();
        else
            loadSystemLog();
        m_btnDelete->setVisible(false);
        return;
    }

    // 普通数据表：RemoteModel 单元格编辑（OnFieldChange 即时提交）
    m_visitFilterRow->setVisible(false);
    m_logFilterRow->setVisible(false);
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
    if (pureTable == "t_inventory_log") {
        m_hintLabel->setText(QString("当前表：库存流水，共 %1 行 | 只读记录，可选中行后删除")
                             .arg(m_model->rowCount()));
    } else if (pureTable == "t_parts") {
        m_hintLabel->setText(QString("当前表：备件表，共 %1 行 | 整表只读（可删除不可编辑，"
                                     "备件数据由库房/结算业务维护）").arg(m_model->rowCount()));
    } else {
        QString hint = QString("双击单元格编辑，当前表：%1，共 %2 行 | %3")
                             .arg(m_tableSelector->currentText())
                             .arg(m_model->rowCount())
                             .arg(canDel ? "删除权限：本表允许删除"
                                         : "删除权限：当前职位无删除权限");
        m_hintLabel->setText(hint);
    }
}

void DataManagerPage::loadVisitRecords()
{
    QString sat = m_cmbVisitSatisfaction->currentText();

    RemoteQuery query;
    query.prepare(
        "SELECT w.id AS '工单ID', w.order_no AS '工单号', v.owner_name AS '车主', "
        "  v.owner_phone AS '车主电话', v.plate_number AS '车牌号', "
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
    query.exec();
    m_queryModel->setQuery(query);

    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("回访记录：%1 至 %2 | 满意度：%3 | 共 %4 条 | 删除=清除该工单回访信息")
                         .arg(m_visitDateRange->startDate().toString("yyyy-MM-dd"),
                              m_visitDateRange->endDate().toString("yyyy-MM-dd"),
                              sat)
                         .arg(m_queryModel->rowCount()));
}

void DataManagerPage::loadSystemLog()
{
    QString action = m_cmbLogAction->currentText();

    RemoteQuery query;
    query.prepare(
        "SELECT l.id AS 'ID', COALESCE(e.name,'') AS '操作人', "
        "  l.action_type AS '操作类型', l.table_name AS '操作表', "
        "  l.record_id AS '记录ID', l.detail AS '详情', l.created_at AS '操作时间' "
        "FROM t_system_log l "
        "LEFT JOIN t_employee e ON e.id = l.operator_id "
        "WHERE DATE(l.created_at) BETWEEN :start AND :end "
        + (action == "全部" ? QString() : QString(" AND l.action_type = :act"))
        + " ORDER BY l.created_at DESC LIMIT 2000");
    query.bindValue(":start", m_logDateRange->startDate().toString("yyyy-MM-dd"));
    query.bindValue(":end", m_logDateRange->endDate().toString("yyyy-MM-dd"));
    if (action != "全部")
        query.bindValue(":act", action);
    query.exec();
    m_queryModel->setQuery(query);

    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("系统日志：%1 至 %2 | 操作类型：%3 | 共 %4 条（只读，禁删）")
                         .arg(m_logDateRange->startDate().toString("yyyy-MM-dd"),
                              m_logDateRange->endDate().toString("yyyy-MM-dd"),
                              action)
                         .arg(m_queryModel->rowCount()));
}

// 按当前选中的日志表切换"操作类型"下拉文案：
//   系统日志表 = 全部/insert/update/delete；操作日志表 = 全部/新增/修改/删除
void DataManagerPage::setupLogActionFilter(bool chinese)
{
    const QStringList items = chinese
        ? QStringList({"全部", "新增", "修改", "删除"})
        : QStringList({"全部", "insert", "update", "delete"});

    bool needRebuild = (m_cmbLogAction->count() != items.size());
    if (!needRebuild) {
        for (int i = 0; i < items.size(); ++i) {
            if (m_cmbLogAction->itemText(i) != items.at(i)) {
                needRebuild = true;
                break;
            }
        }
    }
    if (needRebuild) {
        QSignalBlocker b(m_cmbLogAction);   // 重建项时抑制信号，避免触发重复查询
        m_cmbLogAction->clear();
        m_cmbLogAction->addItems(items);
    }
}

// 日志筛选变化 → 按当前选中的表重新加载（系统日志/操作日志共用同一筛选行）
void DataManagerPage::onLogFilterChanged()
{
    const QString pureTable = m_tableSelector->currentData().toString();
    if (pureTable == "操作日志")
        loadOperationLog();
    else
        loadSystemLog();
}

// 操作日志表：与系统日志表内容平行，仅把英文的操作类型/表名译为中文显示
void DataManagerPage::loadOperationLog()
{
    const QString action = m_cmbLogAction->currentText();
    // 中文下拉选项 → 英文存储值（用于查询过滤）
    QString enAction;
    if (action == "新增")      enAction = "insert";
    else if (action == "修改") enAction = "update";
    else if (action == "删除") enAction = "delete";

    RemoteQuery query;
    query.prepare(
        "SELECT l.id AS 'ID', COALESCE(e.name,'') AS '操作人', "
        "  CASE l.action_type WHEN 'insert' THEN '新增' WHEN 'update' THEN '修改' "
        "    WHEN 'delete' THEN '删除' ELSE l.action_type END AS '操作类型', "
        "  CASE l.table_name "
        "    WHEN 't_employee' THEN '员工表' WHEN 't_vehicle' THEN '车辆表' "
        "    WHEN 't_workorder' THEN '工单表' WHEN 't_parts' THEN '备件表' "
        "    WHEN 't_inventory_log' THEN '库存流水表' WHEN 't_settlement' THEN '结算表' "
        "    WHEN 't_vehicle_transaction' THEN '交易历史表' WHEN 't_workorder_item' THEN '工单明细表' "
        "    WHEN 't_quote_item' THEN '报价明细表' WHEN 't_part_instance' THEN '备件实例表' "
        "    WHEN 't_part_purchase' THEN '采购记录表' WHEN 't_maintenance_history' THEN '维修历史表' "
        "    WHEN 't_technician_work_record' THEN '技师工作记录表' "
        "    WHEN 't_system_log' THEN '系统日志表' "
        "    ELSE l.table_name END AS '操作表', "
        "  l.record_id AS '记录ID', l.detail AS '详情', l.created_at AS '操作时间' "
        "FROM t_system_log l "
        "LEFT JOIN t_employee e ON e.id = l.operator_id "
        "WHERE DATE(l.created_at) BETWEEN :start AND :end "
        + (action == "全部" ? QString() : QString(" AND l.action_type = :act"))
        + " ORDER BY l.created_at DESC LIMIT 2000");
    query.bindValue(":start", m_logDateRange->startDate().toString("yyyy-MM-dd"));
    query.bindValue(":end", m_logDateRange->endDate().toString("yyyy-MM-dd"));
    if (action != "全部")
        query.bindValue(":act", enAction);
    query.exec();
    m_queryModel->setQuery(query);

    m_tableView->resizeColumnsToContents();
    m_hintLabel->setText(QString("操作日志：%1 至 %2 | 操作类型：%3 | 共 %4 条（只读，禁删）")
                         .arg(m_logDateRange->startDate().toString("yyyy-MM-dd"),
                              m_logDateRange->endDate().toString("yyyy-MM-dd"),
                              action)
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
    // t_vehicle（迁移后：车主列置于车牌号前，最后光顾日期列已删除）
    else if (table == "t_vehicle") {
        m_model->setHeaderData(0,  Qt::Horizontal, "ID");
        m_model->setHeaderData(1,  Qt::Horizontal, "车主");
        m_model->setHeaderData(2,  Qt::Horizontal, "车主电话");
        m_model->setHeaderData(3,  Qt::Horizontal, "车主地址");
        m_model->setHeaderData(4,  Qt::Horizontal, "车牌号");
        m_model->setHeaderData(5,  Qt::Horizontal, "车架号(VIN)");
        m_model->setHeaderData(6,  Qt::Horizontal, "发动机号");
        m_model->setHeaderData(7,  Qt::Horizontal, "厂家/品牌");
        m_model->setHeaderData(8,  Qt::Horizontal, "车型/型号");
        m_model->setHeaderData(9,  Qt::Horizontal, "颜色");
        m_model->setHeaderData(10, Qt::Horizontal, "燃油类型");
        m_model->setHeaderData(11, Qt::Horizontal, "变速箱");
        m_model->setHeaderData(12, Qt::Horizontal, "地区");
        m_model->setHeaderData(13, Qt::Horizontal, "当前公里数");
        m_model->setHeaderData(14, Qt::Horizontal, "购车日期");
        m_model->setHeaderData(15, Qt::Horizontal, "年审日期");
        m_model->setHeaderData(16, Qt::Horizontal, "保险日期");
        m_model->setHeaderData(17, Qt::Horizontal, "最后保养日期");
        m_model->setHeaderData(18, Qt::Horizontal, "保养公里数");
        m_model->setHeaderData(19, Qt::Horizontal, "创建时间");
        m_model->setHeaderData(20, Qt::Horizontal, "更新时间");
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
        m_model->setHeaderData(15, Qt::Horizontal, "");
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
        // 备件目录数据由库房/结算等业务操作维护：整表只读（可删除，不可手改单元格）
        //   （库存量由 t_part_instance 在库实例数自动重算，手改会与库房口径不一致）
        for (int c = 0; c <= 11; ++c)
            m_model->setColumnReadOnly(c, true);
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
        // 库存流水属审计性记录：仅允许删除，禁止直接改字段（避免与出入库/报表口径不一致）
        for (int c = 0; c <= 11; ++c)
            m_model->setColumnReadOnly(c, true);
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

        RemoteQuery up;
        up.prepare("UPDATE t_workorder SET satisfaction=NULL, remark=NULL, visitor_id=NULL, "
                   "visited_at=NULL, is_visited='未回访' WHERE id=:oid");
        up.bindValue(":oid", id.toInt());
        if (!up.exec()) {
            QMessageBox::warning(this, "删除失败", up.lastError().text());
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
            RemoteQuery q;
            q.prepare("SELECT status FROM t_workorder WHERE id = :id");
            q.bindValue(":id", workorderId);
            q.exec();
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
            q.exec();
            int instanceCount = q.next() ? q.value(0).toInt() : 0;

            q.prepare("SELECT COUNT(*) FROM t_workorder_item WHERE workorder_id = :wid");
            q.bindValue(":wid", workorderId);
            q.exec();
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

        // 级联删除（事务）在 4s-server 端执行（data.deleteRow），客户端不再持事务
        QJsonObject delResp = RemoteDb::deleteRow("t_parts", partId);
        if (delResp.value("ok").toBool()) {
            m_model->select();
            m_tableView->resizeColumnsToContents();
            m_hintLabel->setText(QString("已删除备件及其相关日志，当前表：%1，共 %2 行")
                                 .arg(tableDisplay).arg(m_model->rowCount()));
        } else {
            QMessageBox::warning(this, "删除失败",
                "删除备件失败：" + delResp.value("error").toString());
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
    const QString cur = m_tableSelector->currentData().toString();
    // 查询模型视图下刷新各自的只读查询；普通表模型走 select()
    if (cur == "回访记录") {
        loadVisitRecords();
        return;
    }
    if (cur == "t_system_log") {
        loadSystemLog();
        return;
    }
    if (cur == "操作日志") {
        loadOperationLog();
        return;
    }
    m_model->select();
    m_tableView->resizeColumnsToContents();
}

void DataManagerPage::onExport()
{
    QAbstractItemModel *activeModel = m_tableView->model();
    if (!activeModel || activeModel->rowCount() == 0) {
        QMessageBox::information(this, "提示", "当前数据表没有可导出的数据");
        return;
    }

    const QString tableDisplay = m_tableSelector->currentText();
    QString defaultName = QString("%1_%2.xlsx")
        .arg(tableDisplay)
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm"));
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出Excel", defaultName, "Excel 文件 (*.xlsx)");
    if (filePath.isEmpty())
        return;

    QString err = XlsxExporter::writeModel(filePath, activeModel, tableDisplay);
    if (err.isEmpty())
        QMessageBox::information(this, "导出成功",
            QString("「%1」已导出 %2 行到：\n%3")
                .arg(tableDisplay).arg(activeModel->rowCount()).arg(filePath));
    else
        QMessageBox::warning(this, "导出失败", err);
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
