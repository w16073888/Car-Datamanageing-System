#include "PurchasePage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

PurchasePage::PurchasePage(QWidget *parent) : QWidget(parent) { setupUI(); }
PurchasePage::~PurchasePage() {}

void PurchasePage::refreshData()
{
    m_editPartNo->clear();
    m_editName->clear();
    m_editSpec->clear();
    m_spinQuantity->setValue(1);
    m_spinPurchasePrice->setValue(0);
    m_spinSalePrice->setValue(0);
    m_editSupplier->clear();
    m_editWarranty->clear();
    m_cmbApplicableModel->setCurrentIndex(0);
    loadApplicableModelOptions();
}

void PurchasePage::loadApplicableModelOptions()
{
    m_cmbApplicableModel->clear();
    m_cmbApplicableModel->addItem("");
    // 从已有车辆数据中加载车型选项
    QSqlQuery query(DbManager::instance().database());
    query.exec("SELECT DISTINCT CONCAT(IFNULL(brand,''), ' ', IFNULL(model,'')) "
               "FROM t_vehicle "
               "WHERE (brand IS NOT NULL AND brand != '') "
               "   OR (model IS NOT NULL AND model != '') "
               "ORDER BY brand, model");
    while (query.next()) {
        QString val = query.value(0).toString().trimmed();
        if (!val.isEmpty()) {
            m_cmbApplicableModel->addItem(val);
        }
    }
    // 也从已有的适用车型中加载
    query.exec("SELECT DISTINCT applicable_model FROM t_parts "
               "WHERE applicable_model IS NOT NULL AND applicable_model != '' "
               "ORDER BY applicable_model");
    QSet<QString> existing;
    for (int i = 0; i < m_cmbApplicableModel->count(); i++)
        existing.insert(m_cmbApplicableModel->itemText(i));
    while (query.next()) {
        QString val = query.value(0).toString().trimmed();
        if (!val.isEmpty() && !existing.contains(val)) {
            m_cmbApplicableModel->addItem(val);
            existing.insert(val);
        }
    }
}

void PurchasePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    QLabel *title = new QLabel("采购入库");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QGroupBox *group = new QGroupBox("备件信息");
    QFormLayout *form = new QFormLayout(group);
    form->setSpacing(10);

    m_editPartNo = new QLineEdit;
    m_editPartNo->setPlaceholderText("如：BP-001（唯一编号）");
    form->addRow("备件编号 *：", m_editPartNo);

    m_editName = new QLineEdit;
    m_editName->setPlaceholderText("如：机油滤清器");
    form->addRow("备件名称 *：", m_editName);

    m_editSpec = new QLineEdit;
    m_editSpec->setPlaceholderText("规格型号");
    form->addRow("规格型号：", m_editSpec);

    m_spinQuantity = new QSpinBox;
    m_spinQuantity->setRange(1, 999999);
    m_spinQuantity->setValue(1);
    form->addRow("数量：", m_spinQuantity);

    m_spinPurchasePrice = new QDoubleSpinBox;
    m_spinPurchasePrice->setRange(0.01, 999999.99);
    m_spinPurchasePrice->setDecimals(2);
    m_spinPurchasePrice->setPrefix("¥ ");
    form->addRow("进货价：", m_spinPurchasePrice);

    m_spinSalePrice = new QDoubleSpinBox;
    m_spinSalePrice->setRange(0.01, 999999.99);
    m_spinSalePrice->setDecimals(2);
    m_spinSalePrice->setPrefix("¥ ");
    form->addRow("销售价：", m_spinSalePrice);

    m_editSupplier = new QLineEdit;
    m_editSupplier->setPlaceholderText("供应商名称");
    form->addRow("供应商：", m_editSupplier);

    m_editWarranty = new QLineEdit;
    m_editWarranty->setPlaceholderText("如：12个月");
    form->addRow("质保期：", m_editWarranty);

    m_cmbApplicableModel = new QComboBox;
    m_cmbApplicableModel->setEditable(true);
    m_cmbApplicableModel->setPlaceholderText("选择或输入适用车型");
    form->addRow("适用车型：", m_cmbApplicableModel);

    mainLayout->addWidget(group);

    m_btnSave = new QPushButton("提交入库");
    m_btnSave->setStyleSheet(
        "QPushButton { padding: 10px; background: #27ae60; color: white;"
        "  border-radius: 4px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background: #219a52; }");
    m_btnSave->setFixedWidth(150);
    mainLayout->addWidget(m_btnSave);
    mainLayout->addStretch();

    connect(m_btnSave, &QPushButton::clicked, this, &PurchasePage::onSave);
    loadApplicableModelOptions();
}

void PurchasePage::onSave()
{
    if (m_editPartNo->text().trimmed().isEmpty() || m_editName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "校验失败", "备件编号和名称不能为空！");
        return;
    }

    QString partNo = m_editPartNo->text().trimmed();
    QString name = m_editName->text().trimmed();
    QString spec = m_editSpec->text().trimmed();
    int qty = m_spinQuantity->value();
    double purchasePrice = m_spinPurchasePrice->value();
    double salePrice = m_spinSalePrice->value();
    QString supplier = m_editSupplier->text().trimmed();
    QString warranty = m_editWarranty->text().trimmed();
    QString applicableModel = m_cmbApplicableModel->currentText().trimmed();

    DbManager::instance().beginTransaction();

    QSqlQuery query(DbManager::instance().database());

    // 检查备件编号是否已存在
    query.prepare("SELECT id, stock FROM t_parts WHERE part_no = :no");
    query.bindValue(":no", partNo);
    DbManager::instance().executeQuery(query);

    if (query.next()) {
        // 已存在，更新库存和价格
        int existingId = query.value(0).toInt();
        int existingStock = query.value(1).toInt();
        query.prepare("UPDATE t_parts SET stock = :stock, purchase_price = :pp, "
                      "sale_price = :sp, supplier = :sup, spec = :spec, "
                      "applicable_model = :appmodel WHERE id = :id");
        query.bindValue(":stock", existingStock + qty);
        query.bindValue(":pp", purchasePrice);
        query.bindValue(":sp", salePrice);
        query.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        query.bindValue(":spec", spec.isEmpty() ? QVariant(QString()) : spec);
        query.bindValue(":appmodel", applicableModel.isEmpty() ? QVariant(QString()) : applicableModel);
        query.bindValue(":id", existingId);
    } else {
        // 新建备件
        query.prepare("INSERT INTO t_parts (part_no, name, spec, stock, purchase_price, "
                      "sale_price, supplier, warranty_period, applicable_model) "
                      "VALUES (:no, :name, :spec, :stock, :pp, :sp, :sup, :warr, :appmodel)");
        query.bindValue(":no", partNo);
        query.bindValue(":name", name);
        query.bindValue(":spec", spec.isEmpty() ? QVariant(QString()) : spec);
        query.bindValue(":stock", qty);
        query.bindValue(":pp", purchasePrice);
        query.bindValue(":sp", salePrice);
        query.bindValue(":sup", supplier.isEmpty() ? QVariant(QString()) : supplier);
        query.bindValue(":warr", warranty.isEmpty() ? QVariant(QString()) : warranty);
        query.bindValue(":appmodel", applicableModel.isEmpty() ? QVariant(QString()) : applicableModel);
    }

    if (!DbManager::instance().executeQuery(query)) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "入库失败", DbManager::instance().lastError());
        return;
    }

    // 获取备件ID
    int partId = query.lastInsertId().toInt();
    if (partId == 0) {
        query.prepare("SELECT id FROM t_parts WHERE part_no = :no");
        query.bindValue(":no", partNo);
        DbManager::instance().executeQuery(query);
        if (query.next()) partId = query.value(0).toInt();
    }

    // 记录库存流水
    query.prepare("INSERT INTO t_inventory_log (part_id, quantity, unit_price, total_price, "
                  "operation_type, operator_id) "
                  "VALUES (:pid, :qty, :price, :total, '采购入库', :op)");
    query.bindValue(":pid", partId);
    query.bindValue(":qty", qty);
    query.bindValue(":price", purchasePrice);
    query.bindValue(":total", qty * purchasePrice);
    query.bindValue(":op", Session::instance().userId());
    DbManager::instance().executeQuery(query);

    DbManager::instance().commitTransaction();

    QMessageBox::information(this, "入库成功",
        QString("备件 %1 (%2) 入库 %3 个").arg(partNo, name).arg(qty));

    refreshData();
}
