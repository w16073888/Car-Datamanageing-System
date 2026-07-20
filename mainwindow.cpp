#include "mainwindow.h"
#include "database/Session.h"
#include "database/DbManager.h"

#include <QApplication>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ============================================================
    // 窗口基本属性 — 固定 1280x720，禁止最大化/缩放
    // ============================================================
    setWindowTitle("汽修4S店综合管理系统");
    setFixedSize(1280, 720);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    // 移除 Qt::WindowMaximizeButtonHint 和 Qt::WindowMinimizeButtonHint 保持干净
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);

    // 堆栈容器
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // 创建所有页面
    setupPages();

    // 创建菜单栏
    setupMenuBar();

    // 状态栏
    m_statusLabel = new QLabel(
        QString("当前用户：%1 | 职位：%2")
        .arg(Session::instance().userName(), Session::instance().position()));
    statusBar()->addWidget(m_statusLabel);
    statusBar()->setStyleSheet("QStatusBar { background: #ecf0f1; border-top: 1px solid #bdc3c7; }");

    // 全局样式
    applyStyleSheet();

    // 默认显示第一个页面
    m_stack->setCurrentIndex(0);

    qDebug() << "[MainWindow] 初始化完成，用户：" << Session::instance().userName();
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认退出", "确定要退出系统吗？",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::setupPages()
{
    // 按 PAGE_xxx 枚举顺序创建所有页面
    m_pages[PAGE_EMPLOYEE]         = new EmployeePage;
    m_pages[PAGE_DATA_MANAGER]     = new DataManagerPage;
    m_pages[PAGE_VEHICLE]          = new VehiclePage;
    m_pages[PAGE_VEHICLE_QUERY]    = new VehicleQueryPage;
    m_pages[PAGE_WORK_ORDER]       = new WorkOrderPage;
    m_pages[PAGE_QUOTE]            = new QuotePage;
    m_pages[PAGE_PURCHASE]         = new PurchasePage;
    m_pages[PAGE_INVENTORY_OUT]    = new InventoryOutPage;
    m_pages[PAGE_PARTS_RETURN]     = new PartsReturnPage;
    m_pages[PAGE_PURCHASE_RETURN]  = new PurchaseReturnPage;
    m_pages[PAGE_STOCK_QUERY]      = new StockQueryPage;
    m_pages[PAGE_SETTLEMENT]       = new SettlementPage;
    m_pages[PAGE_SETTLEMENT_QUERY] = new SettlementQueryPage;
    m_pages[PAGE_FINANCE]          = new FinancePage;
    m_pages[PAGE_SERVICE_REMINDER] = new ServiceReminderPage;
    m_pages[PAGE_CUSTOMER_VISIT]   = new CustomerVisitPage;
    m_pages[PAGE_EXPORT]           = new ExportPage;
    m_pages[PAGE_BUSINESS_REPORT]  = new BusinessReportPage;
    m_pages[PAGE_INBOUND_REPORT]   = new InboundReportPage;
    m_pages[PAGE_OUTBOUND_REPORT]  = new OutboundReportPage;
    m_pages[PAGE_DASHBOARD]        = new DashboardPage;
    m_pages[PAGE_CHANGE_PASSWORD]  = new ChangePasswordPage;

    for (int i = 0; i < PAGE_COUNT; i++) {
        m_stack->addWidget(m_pages[i]);
    }
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    m_menuBar->setStyleSheet(
        "QMenuBar { background-color: #2c3e50; color: white; font-size: 14px; padding: 2px; }"
        "QMenuBar::item { padding: 6px 14px; background: transparent; }"
        "QMenuBar::item:selected { background-color: #34495e; }"
        "QMenu { background-color: white; border: 1px solid #bdc3c7; padding: 4px; }"
        "QMenu::item { padding: 8px 30px 8px 20px; color: #2c3e50; }"
        "QMenu::item:selected { background-color: #3498db; color: white; }"
        "QMenu::separator { height: 1px; background: #ecf0f1; margin: 4px 10px; }"
    );

    // ============================================================
    // 1. 系统维护
    // ============================================================
    m_menuSystem = m_menuBar->addMenu("系统维护");

    m_actEmployee = m_menuSystem->addAction("员工管理");
    connect(m_actEmployee, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_EMPLOYEE);
    });

    m_actDataManager = m_menuSystem->addAction("数据管理");
    connect(m_actDataManager, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_DATA_MANAGER);
    });

    m_menuSystem->addSeparator();

    m_actChangePwd = m_menuSystem->addAction("修改密码");
    connect(m_actChangePwd, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_CHANGE_PASSWORD);
    });

    m_menuSystem->addSeparator();

    m_actLogout = m_menuSystem->addAction("退出登录");
    connect(m_actLogout, &QAction::triggered, this, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认退出", "确定要退出登录吗？",
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            Session::instance().logout();
            close();
        }
    });

    // ============================================================
    // 2. 业务报修
    // ============================================================
    m_menuRepair = m_menuBar->addMenu("业务报修");

    m_actVehicleReg = m_menuRepair->addAction("车辆登记");
    connect(m_actVehicleReg, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_VEHICLE);
    });

    m_actVehicleQuery = m_menuRepair->addAction("车辆查询");
    connect(m_actVehicleQuery, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_VEHICLE_QUERY);
    });

    m_menuRepair->addSeparator();

    m_actWorkOrder = m_menuRepair->addAction("报修派工");
    connect(m_actWorkOrder, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_WORK_ORDER);
    });

    m_actQuote = m_menuRepair->addAction("报价管理");
    connect(m_actQuote, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_QUOTE);
    });

    // ============================================================
    // 3. 库房管理
    // ============================================================
    m_menuWarehouse = m_menuBar->addMenu("库房管理");

    m_actPurchase = m_menuWarehouse->addAction("采购入库");
    connect(m_actPurchase, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_PURCHASE);
    });

    m_actInventoryOut = m_menuWarehouse->addAction("维修出库");
    connect(m_actInventoryOut, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_INVENTORY_OUT);
    });

    m_actPartsReturn = m_menuWarehouse->addAction("备件退库");
    connect(m_actPartsReturn, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_PARTS_RETURN);
    });

    m_actPurchaseReturn = m_menuWarehouse->addAction("采购退货");
    connect(m_actPurchaseReturn, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_PURCHASE_RETURN);
    });

    m_actStockQuery = m_menuWarehouse->addAction("库存查询");
    connect(m_actStockQuery, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_STOCK_QUERY);
    });

    // ============================================================
    // 4. 结算管理
    // ============================================================
    m_menuSettlement = m_menuBar->addMenu("结算管理");

    m_actSettlement = m_menuSettlement->addAction("工单结算");
    connect(m_actSettlement, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_SETTLEMENT);
    });

    m_actSettlementQuery = m_menuSettlement->addAction("结算查询");
    connect(m_actSettlementQuery, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_SETTLEMENT_QUERY);
    });

    // ============================================================
    // 5. 财务管理
    // ============================================================
    m_menuFinance = m_menuBar->addMenu("财务管理");

    m_actFinance = m_menuFinance->addAction("收入统计 / 成本统计 / 利润分析");
    connect(m_actFinance, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_FINANCE);
    });

    // ============================================================
    // 6. 服务跟踪
    // ============================================================
    m_menuService = m_menuBar->addMenu("服务跟踪");

    m_actServiceReminder = m_menuService->addAction("保养提醒");
    connect(m_actServiceReminder, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_SERVICE_REMINDER);
    });

    m_actCustomerVisit = m_menuService->addAction("客户回访");
    connect(m_actCustomerVisit, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_CUSTOMER_VISIT);
    });

    m_actExport = m_menuService->addAction("导出档案");
    connect(m_actExport, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_EXPORT);
    });

    // ============================================================
    // 7. 报表查询
    // ============================================================
    m_menuReport = m_menuBar->addMenu("报表查询");

    m_actBusinessReport = m_menuReport->addAction("业务流水");
    connect(m_actBusinessReport, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_BUSINESS_REPORT);
    });

    m_actInboundReport = m_menuReport->addAction("入库报表");
    connect(m_actInboundReport, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_INBOUND_REPORT);
    });

    m_actOutboundReport = m_menuReport->addAction("出库报表");
    connect(m_actOutboundReport, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_OUTBOUND_REPORT);
    });

    m_actDashboard = m_menuReport->addAction("经营看板");
    connect(m_actDashboard, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_DASHBOARD);
    });
}

void MainWindow::switchToPage(int index)
{
    if (index >= 0 && index < PAGE_COUNT) {
        m_stack->setCurrentIndex(index);
        setWindowTitle(QString("汽修4S店综合管理系统 — %1")
                      .arg(m_menuBar->activeAction() ? m_menuBar->activeAction()->text() : ""));
    }
}

void MainWindow::applyStyleSheet()
{
    setStyleSheet(
        "QMainWindow { background-color: #f5f6fa; }"
        "QGroupBox {"
        "  font-weight: bold; border: 1px solid #bdc3c7;"
        "  border-radius: 5px; margin-top: 10px; padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 10px; padding: 0 5px;"
        "}"
        "QLineEdit {"
        "  padding: 6px 10px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; background: white;"
        "}"
        "QLineEdit:focus { border-color: #3498db; }"
        "QComboBox {"
        "  padding: 6px 10px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; background: white;"
        "}"
        "QComboBox:focus { border-color: #3498db; }"
        "QSpinBox, QDoubleSpinBox, QDateEdit {"
        "  padding: 6px; border: 1px solid #dcdde1;"
        "  border-radius: 4px; background: white;"
        "}"
        "QTextEdit { border: 1px solid #dcdde1; border-radius: 4px; background: white; }"
        "QTableView { border: 1px solid #dcdde1; background: white; }"
        "QTableView::item:selected { background-color: #3498db; color: white; }"
    );
}
