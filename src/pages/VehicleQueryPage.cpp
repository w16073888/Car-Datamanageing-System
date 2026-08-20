#include "VehicleQueryPage.h"
#include "database/DbManager.h"
#include "database/Session.h"
#include "VehiclePage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDialog>
#include <QDialogButtonBox>

VehicleQueryPage::VehicleQueryPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refreshData();
}

VehicleQueryPage::~VehicleQueryPage()
{
}

void VehicleQueryPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    // 标题
    QLabel *title = new QLabel("车辆查询");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; padding: 5px;");
    mainLayout->addWidget(title);

    // 搜索区域
    QHBoxLayout *searchLayout = new QHBoxLayout;

    m_searchType = new QComboBox;
    m_searchType->addItems({"车牌号", "车架号(VIN)", "车主电话"});
    m_searchType->setFixedWidth(120);
    searchLayout->addWidget(m_searchType);

    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText("请输入关键字搜索...");
    m_searchInput->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchInput, 1);

    m_btnSearch = new QPushButton("搜 索");
    m_btnSearch->setFixedWidth(80);
    m_btnSearch->setStyleSheet(
        "QPushButton { padding: 6px 16px; border: none; border-radius: 4px;"
        "  background-color: #3498db; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #2980b9; }");
    searchLayout->addWidget(m_btnSearch);

    mainLayout->addLayout(searchLayout);

    // 结果数量
    m_resultCount = new QLabel("共 0 条记录");
    m_resultCount->setStyleSheet("color: #7f8c8d; font-size: 13px; padding: 3px;");
    mainLayout->addWidget(m_resultCount);

    // 表格
    m_tableView = new QTableView;
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setStyleSheet(
        "QTableView { border: 1px solid #dcdde1; gridline-color: #ecf0f1; }"
        "QTableView::item { padding: 5px; }"
        "QHeaderView::section { background-color: #34495e; color: white;"
        "  padding: 6px; border: none; font-weight: bold; }");
    mainLayout->addWidget(m_tableView, 1);

    m_model = new QSqlQueryModel(this);
    m_tableView->setModel(m_model);

    // 信号
    connect(m_btnSearch, &QPushButton::clicked, this, &VehicleQueryPage::onSearch);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &VehicleQueryPage::onSearch);
    connect(m_tableView, &QTableView::doubleClicked, this, &VehicleQueryPage::onDoubleClicked);

    refreshData();
}

void VehicleQueryPage::refreshData()
{
    QSqlQuery query(DbManager::instance().database());
    query.prepare(
        "SELECT v.plate_number AS '车牌号', v.vin AS '车架号', "
        "  v.engine_number AS '发动机号', v.purchase_date AS '购车日期', "
        "  v.inspection_date AS '年审日期', v.insurance_date AS '保险日期', "
        "  v.last_maintenance_date AS '最后保养', v.last_maintenance_mileage AS '保养公里数', "
        "  v.last_visit_date AS '最后光顾', "
        "  c.name AS '车主', c.phone AS '联系电话' "
        "FROM t_vehicle v "
        "LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
        "ORDER BY v.id DESC LIMIT 200"
    );
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条记录").arg(m_model->rowCount()));

    // 设置列宽
    if (m_model->columnCount() > 0) {
        m_tableView->setColumnWidth(0, 130);  // 车牌号
        m_tableView->setColumnWidth(1, 180);  // 车架号
    }
}

void VehicleQueryPage::onSearch()
{
    QString keyword = m_searchInput->text().trimmed();
    int type = m_searchType->currentIndex();

    QString sql;
    if (keyword.isEmpty()) {
        refreshData();
        return;
    }

    switch (type) {
    case 0: // 车牌号
        sql = "SELECT v.plate_number AS '车牌号', v.vin AS '车架号', "
              "  v.engine_number AS '发动机号', v.purchase_date AS '购车日期', "
              "  v.inspection_date AS '年审日期', v.insurance_date AS '保险日期', "
              "  v.last_maintenance_date AS '最后保养', v.last_maintenance_mileage AS '保养公里数', "
              "  v.last_visit_date AS '最后光顾', "
              "  c.name AS '车主', c.phone AS '联系电话' "
              "FROM t_vehicle v "
              "LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
              "WHERE v.plate_number LIKE :kw "
              "ORDER BY v.id DESC LIMIT 200";
        break;
    case 1: // 车架号
        sql = "SELECT v.plate_number AS '车牌号', v.vin AS '车架号', "
              "  v.engine_number AS '发动机号', v.purchase_date AS '购车日期', "
              "  v.inspection_date AS '年审日期', v.insurance_date AS '保险日期', "
              "  v.last_maintenance_date AS '最后保养', v.last_maintenance_mileage AS '保养公里数', "
              "  v.last_visit_date AS '最后光顾', "
              "  c.name AS '车主', c.phone AS '联系电话' "
              "FROM t_vehicle v "
              "LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
              "WHERE v.vin LIKE :kw "
              "ORDER BY v.id DESC LIMIT 200";
        break;
    case 2: // 车主电话
        sql = "SELECT v.plate_number AS '车牌号', v.vin AS '车架号', "
              "  v.engine_number AS '发动机号', v.purchase_date AS '购车日期', "
              "  v.inspection_date AS '年审日期', v.insurance_date AS '保险日期', "
              "  v.last_maintenance_date AS '最后保养', v.last_maintenance_mileage AS '保养公里数', "
              "  v.last_visit_date AS '最后光顾', "
              "  c.name AS '车主', c.phone AS '联系电话' "
              "FROM t_vehicle v "
              "LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主' "
              "WHERE c.phone LIKE :kw "
              "ORDER BY v.id DESC LIMIT 200";
        break;
    }

    QSqlQuery query(DbManager::instance().database());
    query.prepare(sql);
    query.bindValue(":kw", "%" + keyword + "%");
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条记录").arg(m_model->rowCount()));
}

void VehicleQueryPage::onDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // 获取车牌号
    QString plate = m_model->data(m_model->index(index.row(), 0)).toString();

    // 弹出编辑窗口（使用VehiclePage嵌入QDialog）
    QDialog dlg(this);
    dlg.setWindowTitle("编辑车辆档案 - " + plate);
    dlg.setFixedSize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    VehiclePage *vehiclePage = new VehiclePage(&dlg);
    vehiclePage->setPlateNumber(plate);
    layout->addWidget(vehiclePage);

    // 修改保存信号——保存后刷新
    connect(vehiclePage, &VehiclePage::vehicleSaved, this, [this]() {
        refreshData();
    });

    dlg.exec();
}
