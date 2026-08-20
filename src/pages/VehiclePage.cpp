#include "VehiclePage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QScrollArea>

VehiclePage::VehiclePage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

VehiclePage::~VehiclePage()
{
}

void VehiclePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    // 标题
    QLabel *title = new QLabel("车辆登记");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; padding: 5px;");
    mainLayout->addWidget(title);

    // 滚动区域（内容多时能滚动）
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    QWidget *content = new QWidget;
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(15);

    // ======== 车辆信息 ========
    QGroupBox *carGroup = new QGroupBox("车辆信息");
    QGridLayout *carGrid = new QGridLayout(carGroup);
    carGrid->setSpacing(10);

    carGrid->addWidget(new QLabel("车牌号 *："), 0, 0);
    m_editPlate = new QLineEdit;
    m_editPlate->setPlaceholderText("如：京A88888");
    carGrid->addWidget(m_editPlate, 0, 1);

    carGrid->addWidget(new QLabel("车架号(VIN)："), 0, 2);
    m_editVin = new QLineEdit;
    m_editVin->setPlaceholderText("17位车架号");
    carGrid->addWidget(m_editVin, 0, 3);

    carGrid->addWidget(new QLabel("发动机号："), 1, 0);
    m_editEngine = new QLineEdit;
    carGrid->addWidget(m_editEngine, 1, 1);

    carGrid->addWidget(new QLabel("购车日期："), 1, 2);
    m_datePurchase = new QDateEdit;
    m_datePurchase->setCalendarPopup(true);
    m_datePurchase->setDisplayFormat("yyyy-MM-dd");
    m_datePurchase->setDate(QDate::currentDate());
    carGrid->addWidget(m_datePurchase, 1, 3);

    carGrid->addWidget(new QLabel("年审日期："), 2, 0);
    m_dateInspection = new QDateEdit;
    m_dateInspection->setCalendarPopup(true);
    m_dateInspection->setDisplayFormat("yyyy-MM-dd");
    m_dateInspection->setDate(QDate::currentDate());
    carGrid->addWidget(m_dateInspection, 2, 1);

    carGrid->addWidget(new QLabel("保险日期："), 2, 2);
    m_dateInsurance = new QDateEdit;
    m_dateInsurance->setCalendarPopup(true);
    m_dateInsurance->setDisplayFormat("yyyy-MM-dd");
    m_dateInsurance->setDate(QDate::currentDate());
    carGrid->addWidget(m_dateInsurance, 2, 3);

    contentLayout->addWidget(carGroup);

    // ======== 车主信息 ========
    QGroupBox *ownerGroup = new QGroupBox("车主信息");
    QGridLayout *ownerGrid = new QGridLayout(ownerGroup);
    ownerGrid->setSpacing(10);

    ownerGrid->addWidget(new QLabel("车主姓名 *："), 0, 0);
    m_editOwnerName = new QLineEdit;
    m_editOwnerName->setPlaceholderText("请输入车主姓名");
    ownerGrid->addWidget(m_editOwnerName, 0, 1);

    ownerGrid->addWidget(new QLabel("联系电话 *："), 0, 2);
    m_editOwnerPhone = new QLineEdit;
    m_editOwnerPhone->setPlaceholderText("请输入手机号码");
    ownerGrid->addWidget(m_editOwnerPhone, 0, 3);

    contentLayout->addWidget(ownerGroup);

    // ======== 驾驶员信息 ========
    QGroupBox *driverGroup = new QGroupBox("驾驶员信息（选填）");
    QGridLayout *driverGrid = new QGridLayout(driverGroup);
    driverGrid->setSpacing(10);

    driverGrid->addWidget(new QLabel("驾驶员姓名："), 0, 0);
    m_editDriverName = new QLineEdit;
    driverGrid->addWidget(m_editDriverName, 0, 1);

    driverGrid->addWidget(new QLabel("驾驶员电话："), 0, 2);
    m_editDriverPhone = new QLineEdit;
    driverGrid->addWidget(m_editDriverPhone, 0, 3);

    contentLayout->addWidget(driverGroup);

    // ======== 按钮 ========
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();

    m_btnClear = new QPushButton("清空");
    m_btnClear->setFixedWidth(100);
    m_btnClear->setStyleSheet(
        "QPushButton { padding: 8px 16px; border: 1px solid #bdc3c7;"
        "  border-radius: 4px; background-color: #ecf0f1; }"
        "QPushButton:hover { background-color: #bdc3c7; }");
    btnLayout->addWidget(m_btnClear);

    m_btnSave = new QPushButton("保存车辆信息");
    m_btnSave->setFixedWidth(140);
    m_btnSave->setStyleSheet(
        "QPushButton { padding: 8px 16px; border: none; border-radius: 4px;"
        "  background-color: #3498db; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #2980b9; }");
    btnLayout->addWidget(m_btnSave);

    contentLayout->addLayout(btnLayout);
    contentLayout->addStretch();

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    // 信号
    connect(m_btnSave, &QPushButton::clicked, this, &VehiclePage::onSave);
    connect(m_btnClear, &QPushButton::clicked, this, &VehiclePage::onClear);
}

void VehiclePage::setPlateNumber(const QString &plate)
{
    m_editPlate->setText(plate);
}

bool VehiclePage::validateInput()
{
    if (m_editPlate->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "校验失败", "车牌号不能为空！");
        m_editPlate->setFocus();
        return false;
    }
    if (m_editOwnerName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "校验失败", "车主姓名不能为空！");
        m_editOwnerName->setFocus();
        return false;
    }
    if (m_editOwnerPhone->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "校验失败", "车主联系电话不能为空！");
        m_editOwnerPhone->setFocus();
        return false;
    }
    return true;
}

void VehiclePage::onSave()
{
    if (!validateInput()) return;

    QString plate = m_editPlate->text().trimmed();
    QString vin = m_editVin->text().trimmed();
    QString engine = m_editEngine->text().trimmed();
    QString purDate = m_datePurchase->date().toString("yyyy-MM-dd");
    QString inspecDate = m_dateInspection->date().toString("yyyy-MM-dd");
    QString insurDate = m_dateInsurance->date().toString("yyyy-MM-dd");
    QString ownerName = m_editOwnerName->text().trimmed();
    QString ownerPhone = m_editOwnerPhone->text().trimmed();
    QString driverName = m_editDriverName->text().trimmed();
    QString driverPhone = m_editDriverPhone->text().trimmed();

    QSqlDatabase &db = DbManager::instance().database();
    DbManager::instance().beginTransaction();

    QSqlQuery query(db);

    // 1. 检查车牌号是否已存在
    query.prepare("SELECT id FROM t_vehicle WHERE plate_number = :plate");
    query.bindValue(":plate", plate);
    DbManager::instance().executeQuery(query);

    int vehicleId = 0;
    if (query.next()) {
        // 车辆已存在，更新信息
        vehicleId = query.value(0).toInt();
        query.prepare("UPDATE t_vehicle SET vin = :vin, engine_number = :eng, "
                      "purchase_date = :pur, inspection_date = :insp, "
                      "insurance_date = :insu WHERE id = :id");
        query.bindValue(":vin", vin.isEmpty() ? QVariant() : vin);
        query.bindValue(":eng", engine.isEmpty() ? QVariant() : engine);
        query.bindValue(":pur", purDate);
        query.bindValue(":insp", inspecDate);
        query.bindValue(":insu", insurDate);
        query.bindValue(":id", vehicleId);
    } else {
        // 新车
        query.prepare("INSERT INTO t_vehicle (plate_number, vin, engine_number, "
                      "purchase_date, inspection_date, insurance_date) "
                      "VALUES (:plate, :vin, :eng, :pur, :insp, :insu)");
        query.bindValue(":plate", plate);
        query.bindValue(":vin", vin.isEmpty() ? QVariant() : vin);
        query.bindValue(":eng", engine.isEmpty() ? QVariant() : engine);
        query.bindValue(":pur", purDate);
        query.bindValue(":insp", inspecDate);
        query.bindValue(":insu", insurDate);
    }

    if (!DbManager::instance().executeQuery(query)) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "保存失败", "车辆信息保存失败：" + DbManager::instance().lastError());
        return;
    }

    if (vehicleId == 0) {
        vehicleId = query.lastInsertId().toInt();
    }

    // 2. 保存车主信息
    query.prepare("SELECT id FROM t_customer WHERE vehicle_id = :vid AND type = '车主'");
    query.bindValue(":vid", vehicleId);
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        int cusId = query.value(0).toInt();
        query.prepare("UPDATE t_customer SET name = :name, phone = :phone WHERE id = :id");
        query.bindValue(":name", ownerName);
        query.bindValue(":phone", ownerPhone);
        query.bindValue(":id", cusId);
    } else {
        query.prepare("INSERT INTO t_customer (vehicle_id, name, phone, type) "
                      "VALUES (:vid, :name, :phone, '车主')");
        query.bindValue(":vid", vehicleId);
        query.bindValue(":name", ownerName);
        query.bindValue(":phone", ownerPhone);
    }

    if (!DbManager::instance().executeQuery(query)) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "保存失败", "车主信息保存失败：" + DbManager::instance().lastError());
        return;
    }

    // 3. 如果有驾驶员信息，保存
    if (!driverName.isEmpty()) {
        query.prepare("SELECT id FROM t_customer WHERE vehicle_id = :vid AND type = '驾驶员'");
        query.bindValue(":vid", vehicleId);
        DbManager::instance().executeQuery(query);

        if (query.next()) {
            int drvId = query.value(0).toInt();
            query.prepare("UPDATE t_customer SET name = :name, phone = :phone WHERE id = :id");
            query.bindValue(":name", driverName);
            query.bindValue(":phone", driverPhone);
            query.bindValue(":id", drvId);
        } else {
            query.prepare("INSERT INTO t_customer (vehicle_id, name, phone, type) "
                          "VALUES (:vid, :name, :phone, '驾驶员')");
            query.bindValue(":vid", vehicleId);
            query.bindValue(":name", driverName);
            query.bindValue(":phone", driverPhone);
        }
        DbManager::instance().executeQuery(query);
    }

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "保存成功",
        QString("车辆 %1 登记成功！").arg(plate));

    emit vehicleSaved(plate);
}

void VehiclePage::onClear()
{
    m_editPlate->clear();
    m_editVin->clear();
    m_editEngine->clear();
    m_datePurchase->setDate(QDate::currentDate());
    m_dateInspection->setDate(QDate::currentDate());
    m_dateInsurance->setDate(QDate::currentDate());
    m_editOwnerName->clear();
    m_editOwnerPhone->clear();
    m_editDriverName->clear();
    m_editDriverPhone->clear();
}
