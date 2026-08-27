#include "ExportPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>
#include <QTextStream>

ExportPage::ExportPage(QWidget *parent) : QWidget(parent) { setupUI(); }
ExportPage::~ExportPage() {}

void ExportPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    QLabel *title = new QLabel("导出档案");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QGroupBox *group = new QGroupBox("选择导出内容");
    QVBoxLayout *gLayout = new QVBoxLayout(group);

    m_chkCustomer = new QCheckBox("客户信息表（t_customer）");
    m_chkCustomer->setChecked(true);
    gLayout->addWidget(m_chkCustomer);

    m_chkVehicle = new QCheckBox("车辆档案表（t_vehicle）");
    m_chkVehicle->setChecked(true);
    gLayout->addWidget(m_chkVehicle);

    m_btnExport = new QPushButton("导出到CSV文件");
    m_btnExport->setStyleSheet(
        "QPushButton { padding: 10px; background: #3498db; color: white;"
        "  border-radius: 4px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background: #2980b9; }");
    gLayout->addWidget(m_btnExport);

    mainLayout->addWidget(group);

    m_lblStatus = new QLabel;
    m_lblStatus->setStyleSheet("color: #7f8c8d; font-size: 13px;");
    mainLayout->addWidget(m_lblStatus);
    mainLayout->addStretch();

    connect(m_btnExport, &QPushButton::clicked, this, &ExportPage::onExportCustomer);
}

void ExportPage::onExportCustomer()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录");
    if (dir.isEmpty()) return;

    int exported = 0;

    if (m_chkCustomer->isChecked()) {
        QString filePath = dir + "/customer.csv";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            // BOM for Excel
            out << QChar(0xFEFF);
            out << "ID,车辆ID,姓名,电话,类型,创建时间\n";

            QSqlQuery query(DbManager::instance().database());
            query.exec("SELECT id, vehicle_id, name, phone, type, created_at FROM t_customer");
            while (query.next()) {
                out << query.value(0).toString() << ","
                    << query.value(1).toString() << ","
                    << query.value(2).toString() << ","
                    << query.value(3).toString() << ","
                    << query.value(4).toString() << ","
                    << query.value(5).toString() << "\n";
            }
            file.close();
            exported++;
        }
    }

    if (m_chkVehicle->isChecked()) {
        QString filePath = dir + "/vehicle.csv";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << QChar(0xFEFF);
            out << "ID,车牌号,车架号,发动机号,购车日期,年审日期,保险日期,最后保养日期,最后保养公里数,最后光顾日期\n";

            QSqlQuery query(DbManager::instance().database());
            query.exec("SELECT id, plate_number, vin, engine_number, purchase_date, "
                       "inspection_date, insurance_date, last_maintenance_date, "
                       "last_maintenance_mileage, last_visit_date FROM t_vehicle");
            while (query.next()) {
                out << query.value(0).toString() << ","
                    << query.value(1).toString() << ","
                    << query.value(2).toString() << ","
                    << query.value(3).toString() << ","
                    << query.value(4).toString() << ","
                    << query.value(5).toString() << ","
                    << query.value(6).toString() << ","
                    << query.value(7).toString() << ","
                    << query.value(8).toString() << ","
                    << query.value(9).toString() << "\n";
            }
            file.close();
            exported++;
        }
    }

    m_lblStatus->setText(QString("导出完成！共导出 %1 个文件到 %2").arg(exported).arg(dir));
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 个文件到\n%2").arg(exported).arg(dir));
}

void ExportPage::onExportVehicle()
{
    onExportCustomer();
}
