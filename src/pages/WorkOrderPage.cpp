#include "WorkOrderPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

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

WorkOrderPage::WorkOrderPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

WorkOrderPage::~WorkOrderPage()
{
}

QString WorkOrderPage::generateOrderNo()
{
    QSqlQuery query(DbManager::instance().database());
    QString datePrefix = QDate::currentDate().toString("yyyyMMdd");
    query.prepare("SELECT COUNT(*) FROM t_workorder WHERE order_no LIKE :prefix");
    query.bindValue(":prefix", "WO" + datePrefix + "%");
    DbManager::instance().executeQuery(query);
    int count = 0;
    if (query.next()) count = query.value(0).toInt();
    return QString("WO%1%2").arg(datePrefix).arg(count + 1, 4, 10, QChar('0'));
}

void WorkOrderPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);

    QLabel *title = new QLabel("报修派工");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // 上半部分：创建工单
    QHBoxLayout *topLayout = new QHBoxLayout;

    // 左侧：车辆选择
    QGroupBox *vehicleGroup = new QGroupBox("选择车辆");
    QVBoxLayout *vLayout = new QVBoxLayout(vehicleGroup);
    QHBoxLayout *searchLayout = new QHBoxLayout;
    m_editPlateSearch = new QLineEdit;
    m_editPlateSearch->setPlaceholderText("输入车牌号");
    searchLayout->addWidget(m_editPlateSearch);
    m_btnSearchVehicle = new QPushButton("查询");
    searchLayout->addWidget(m_btnSearchVehicle);
    vLayout->addLayout(searchLayout);
    m_lblVehicleInfo = new QLabel("请选择车辆");
    m_lblVehicleInfo->setWordWrap(true);
    m_lblVehicleInfo->setStyleSheet("padding: 8px; background: #f8f9fa; border-radius: 4px;");
    vLayout->addWidget(m_lblVehicleInfo);
    topLayout->addWidget(vehicleGroup);

    // 中间：工单信息
    QGroupBox *orderGroup = new QGroupBox("工单信息");
    QFormLayout *formLayout = new QFormLayout(orderGroup);

    m_editOrderNo = new QLineEdit;
    m_editOrderNo->setReadOnly(true);
    formLayout->addRow("工单号：", m_editOrderNo);

    m_cmbTechnician = new QComboBox;
    // 加载技师列表
    QSqlQuery techQuery(DbManager::instance().database());
    techQuery.exec("SELECT id, name FROM t_employee WHERE position IN ('经理','前台')");
    while (techQuery.next()) {
        m_cmbTechnician->addItem(techQuery.value(1).toString(), techQuery.value(0).toInt());
    }
    formLayout->addRow("维修责任人：", m_cmbTechnician);

    m_editMileage = new QLineEdit;
    m_editMileage->setPlaceholderText("当前公里数");
    formLayout->addRow("公里数：", m_editMileage);

    m_textRepairContent = new QTextEdit;
    m_textRepairContent->setPlaceholderText("请描述报修内容...");
    m_textRepairContent->setMaximumHeight(80);
    formLayout->addRow("报修内容：", m_textRepairContent);

    m_editLaborFee = new QLineEdit;
    m_editLaborFee->setPlaceholderText("预估工时费");
    formLayout->addRow("预估工时费：", m_editLaborFee);

    topLayout->addWidget(orderGroup);

    // 右侧：状态控制
    QGroupBox *statusGroup = new QGroupBox("工单状态");
    QVBoxLayout *sLayout = new QVBoxLayout(statusGroup);
    m_lblStatus = new QLabel("当前状态：待派工");
    m_lblStatus->setStyleSheet("font-size: 16px; font-weight: bold; color: #e67e22; padding: 10px;");
    sLayout->addWidget(m_lblStatus);
    m_btnCreate = new QPushButton("创建工单");
    m_btnCreate->setStyleSheet("padding: 8px; background: #3498db; color: white; border-radius: 4px; font-weight: bold;");
    sLayout->addWidget(m_btnCreate);
    m_btnStartRepair = new QPushButton("开始维修");
    m_btnStartRepair->setStyleSheet("padding: 8px; background: #e67e22; color: white; border-radius: 4px;");
    sLayout->addWidget(m_btnStartRepair);
    m_btnComplete = new QPushButton("完工");
    m_btnComplete->setStyleSheet("padding: 8px; background: #27ae60; color: white; border-radius: 4px;");
    sLayout->addWidget(m_btnComplete);
    sLayout->addStretch();
    topLayout->addWidget(statusGroup);

    mainLayout->addLayout(topLayout);

    // 下半部分：工单列表
    QLabel *listTitle = new QLabel("工单列表");
    listTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50; padding-top: 10px;");
    mainLayout->addWidget(listTitle);

    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; }"
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);
    m_btnRefresh = new QPushButton("刷新列表");
    mainLayout->addWidget(m_btnRefresh);

    // 信号
    connect(m_btnSearchVehicle, &QPushButton::clicked, this, &WorkOrderPage::onSelectVehicle);
    connect(m_btnCreate, &QPushButton::clicked, this, &WorkOrderPage::onCreateOrder);
    connect(m_btnStartRepair, &QPushButton::clicked, this, &WorkOrderPage::onChangeStatus);
    connect(m_btnComplete, &QPushButton::clicked, this, &WorkOrderPage::onChangeStatus);
    connect(m_btnRefresh, &QPushButton::clicked, this, &WorkOrderPage::refreshOrderList);
    connect(m_tableView, &QTableView::clicked, this, &WorkOrderPage::onOrderSelected);

    refreshOrderList();
}

void WorkOrderPage::onSelectVehicle()
{
    QString plate = m_editPlateSearch->text().trimmed();
    if (plate.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入车牌号");
        return;
    }
    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT v.id, v.plate_number, v.vin, c.name, c.phone "
                  "FROM t_vehicle v LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
                  "WHERE v.plate_number LIKE :plate LIMIT 1");
    query.bindValue(":plate", "%" + plate + "%");
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        m_currentVehicleId = query.value(0).toInt();
        m_lblVehicleInfo->setText(
            QString("车牌：%1\nVIN：%2\n车主：%3\n电话：%4")
            .arg(query.value(1).toString(), query.value(2).toString(),
                 query.value(3).toString(), query.value(4).toString()));
    } else {
        QMessageBox::information(this, "未找到", "未找到该车辆信息，请先登记车辆");
    }
}

void WorkOrderPage::onCreateOrder()
{
    if (m_currentVehicleId == 0) {
        QMessageBox::warning(this, "提示", "请先选择车辆");
        return;
    }

    QString orderNo = generateOrderNo();
    m_editOrderNo->setText(orderNo);

    QSqlQuery query(DbManager::instance().database());
    query.prepare("INSERT INTO t_workorder (order_no, vehicle_id, technician_id, "
                  "mileage, repair_content, labor_fee, status, created_by) "
                  "VALUES (:no, :vid, :tech, :mile, :content, :fee, '待派工', :creator)");
    query.bindValue(":no", orderNo);
    query.bindValue(":vid", m_currentVehicleId);
    query.bindValue(":tech", m_cmbTechnician->currentData().toInt());
    query.bindValue(":mile", m_editMileage->text().toInt());
    query.bindValue(":content", m_textRepairContent->toPlainText());
    query.bindValue(":fee", m_editLaborFee->text().toDouble());
    query.bindValue(":creator", Session::instance().userId());

    if (!DbManager::instance().executeQuery(query)) {
        QMessageBox::warning(this, "创建失败", DbManager::instance().lastError());
        return;
    }

    m_currentOrderId = query.lastInsertId().toInt();
    m_lblStatus->setText("当前状态：待派工");
    QMessageBox::information(this, "成功", "工单 " + orderNo + " 创建成功！");
    refreshOrderList();
}

void WorkOrderPage::onChangeStatus()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn || m_currentOrderId == 0) {
        QMessageBox::warning(this, "提示", "请先选择工单");
        return;
    }

    QString newStatus;
    if (btn == m_btnStartRepair) newStatus = "维修中";
    else if (btn == m_btnComplete) newStatus = "已完工";
    else return;

    QSqlQuery query(DbManager::instance().database());
    // 状态流转检查
    query.prepare("SELECT status FROM t_workorder WHERE id = :id");
    query.bindValue(":id", m_currentOrderId);
    DbManager::instance().executeQuery(query);
    if (!query.next()) return;

    QString curStatus = query.value(0).toString();
    if ((curStatus == "待派工" && newStatus == "维修中") ||
        (curStatus == "维修中" && newStatus == "已完工")) {
        query.prepare("UPDATE t_workorder SET status = :status WHERE id = :id");
        query.bindValue(":status", newStatus);
        query.bindValue(":id", m_currentOrderId);
        if (DbManager::instance().executeQuery(query)) {
            m_lblStatus->setText("当前状态：" + newStatus);
            refreshOrderList();
        }
    } else {
        QMessageBox::warning(this, "状态错误",
            QString("无法从 '%1' 变更为 '%2'").arg(curStatus, newStatus));
    }
}

void WorkOrderPage::onOrderSelected(const QModelIndex &index)
{
    if (!index.isValid()) return;
    m_currentOrderId = m_model->data(m_model->index(index.row(), 0)).toInt();
    QString orderNo = m_model->data(m_model->index(index.row(), 1)).toString();
    QString status = m_model->data(m_model->index(index.row(), 6)).toString();
    m_editOrderNo->setText(orderNo);
    m_lblStatus->setText("当前状态：" + status);
}

void WorkOrderPage::refreshOrderList()
{
    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT w.id, w.order_no AS '工单号', v.plate_number AS '车牌号', "
        "  e.name AS '责任人', w.mileage AS '公里数', "
        "  w.labor_fee AS '工时费', w.status AS '状态', w.created_at AS '创建时间' "
        "FROM t_workorder w "
        "LEFT JOIN t_vehicle v ON v.id = w.vehicle_id "
        "LEFT JOIN t_employee e ON e.id = w.technician_id "
        "ORDER BY w.id DESC LIMIT 200"
    );
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
}
