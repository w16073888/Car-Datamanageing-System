#include "StockQueryPage.h"
#include "database/DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>

StockQueryPage::StockQueryPage(QWidget *parent) : QWidget(parent) { setupUI(); }
StockQueryPage::~StockQueryPage() {}

void StockQueryPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);

    QLabel *title = new QLabel("库存查询");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    m_searchInput = new QLineEdit;
    m_searchInput->setPlaceholderText("输入备件编号或名称搜索");
    searchLayout->addWidget(m_searchInput, 1);
    m_btnSearch = new QPushButton("搜索");
    m_btnSearch->setStyleSheet("padding: 6px 14px; background: #3498db; color: white; border-radius: 4px;");
    searchLayout->addWidget(m_btnSearch);
    m_btnRefresh = new QPushButton("刷新");
    m_btnRefresh->setStyleSheet("padding: 6px 14px; border: 1px solid #bdc3c7; border-radius: 4px;");
    searchLayout->addWidget(m_btnRefresh);
    mainLayout->addLayout(searchLayout);

    m_resultCount = new QLabel;
    m_resultCount->setStyleSheet("color: #7f8c8d; font-size: 13px;");
    mainLayout->addWidget(m_resultCount);

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

    connect(m_btnSearch, &QPushButton::clicked, this, &StockQueryPage::onSearch);
    connect(m_btnRefresh, &QPushButton::clicked, this, &StockQueryPage::onRefresh);
    connect(m_searchInput, &QLineEdit::returnPressed, this, &StockQueryPage::onSearch);

    onRefresh();
}

void StockQueryPage::onSearch()
{
    QString kw = m_searchInput->text().trimmed();
    QSqlQuery query(DbManager::instance().database());
    if (kw.isEmpty()) {
        onRefresh();
        return;
    }
    query.prepare("SELECT part_no AS '备件编号', name AS '备件名称', spec AS '规格', "
                  "stock AS '库存量', purchase_price AS '进货价', sale_price AS '销售价', "
                  "supplier AS '供应商', warranty_period AS '质保期' "
                  "FROM t_parts WHERE part_no LIKE :kw OR name LIKE :kw2 ORDER BY id DESC");
    query.bindValue(":kw", "%" + kw + "%");
    query.bindValue(":kw2", "%" + kw + "%");
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条记录").arg(m_model->rowCount()));
}

void StockQueryPage::onRefresh()
{
    QSqlQuery query(DbManager::instance().database());
    query.prepare("SELECT part_no AS '备件编号', name AS '备件名称', spec AS '规格', "
                  "stock AS '库存量', purchase_price AS '进货价', sale_price AS '销售价', "
                  "supplier AS '供应商', warranty_period AS '质保期' "
                  "FROM t_parts ORDER BY id DESC LIMIT 200");
    DbManager::instance().executeQuery(query);
    m_model->setQuery(std::move(query));
    m_resultCount->setText(QString("共 %1 条记录").arg(m_model->rowCount()));
}
