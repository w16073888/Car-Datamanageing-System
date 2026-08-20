#include "mainwindow.h"
#include "database/Session.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStatusBar>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDialog>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ============================================================
    // 窗口基本属性
    // ============================================================
    setWindowTitle("科盟汽修综合数据管理");
    setWindowIcon(QIcon(QCoreApplication::applicationDirPath() + "/logo.ico"));
    resize(1600, 900);
    setMinimumSize(960, 540);

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
    m_orderNoLabel = new QLabel;
    m_orderNoLabel->setStyleSheet("font-weight:bold;color:#2980b9;");
    m_orderNoLabel->setVisible(false);
    m_currentPageIndex = 0;
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addWidget(m_orderNoLabel);
    statusBar()->setStyleSheet("QStatusBar { background: #ecf0f1; border-top: 1px solid #bdc3c7; }");

    // 全局样式
    applyStyleSheet();

    // 默认显示第一个页面（按职位选择入口，员工管理页仅经理可见）
    {
        const QString pos = Session::instance().position();
        if (pos == "前台")
            m_stack->setCurrentIndex(PAGE_QUOTE);             // 工单查询
        else if (pos == "库管")
            m_stack->setCurrentIndex(PAGE_DASHBOARD);         // 经营看板
        else if (pos == "客服")
            m_stack->setCurrentIndex(PAGE_CUSTOMER_VISIT);    // 客户回访
        else
            m_stack->setCurrentIndex(PAGE_EMPLOYEE);          // 员工管理
    }

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
    // 按 PAGE_xxx 枚举顺序创建所有页面（已删除 8 个菜单不可达页面）
    m_pages[PAGE_EMPLOYEE]         = new EmployeePage;
    m_pages[PAGE_DATA_MANAGER]     = new DataManagerPage;
    m_pages[PAGE_FRONT_DESK]       = new QWidget;  // 占位，前台工作台改为弹窗模式
    m_pages[PAGE_WAREHOUSE]        = new QWidget;  // 占位，库房工作台改为弹窗模式
    m_pages[PAGE_QUOTE]            = new QuotePage;
    m_pages[PAGE_FINANCE]          = new FinancePage;
    m_pages[PAGE_SERVICE_REMINDER] = new ServiceReminderPage;
    m_pages[PAGE_CUSTOMER_VISIT]   = new CustomerVisitPage;
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
        "QMenuBar { background-color: #2c3e50; color: white; font-size: 16px; padding: 3px; }"
        "QMenuBar::item { padding: 7px 16px; background: transparent; }"
        "QMenuBar::item:selected { background-color: #34495e; }"
        "QMenu { background-color: white; border: 1px solid #bdc3c7; padding: 4px; font-size: 15px; }"
        "QMenu::item { padding: 9px 30px 9px 20px; color: #2c3e50; }"
        "QMenu::item:selected { background-color: #3498db; color: white; }"
        "QMenu::separator { height: 1px; background: #ecf0f1; margin: 4px 10px; }"
    );

    // ============================================================
    // 1. 系统维护（所有角色可用）
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
    // 2. 前台业务
    // ============================================================
    m_menuRepair = m_menuBar->addMenu("前台业务");

    m_actFrontDesk = m_menuRepair->addAction("前台工作台");
    m_actFrontDesk->setStatusTip("车辆登记、派工、打印报价单/工单");
    connect(m_actFrontDesk, &QAction::triggered, this, [this]() {
        // 前台工作台以独立固定尺寸窗口打开
        QDialog dlg(this);
        dlg.setWindowTitle("前台工作台");
        dlg.setFixedSize(1280, 720);
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        l->setContentsMargins(0, 0, 0, 0);

        FrontDeskPage *fd = new FrontDeskPage(&dlg);
        l->addWidget(fd);

        connect(fd, &FrontDeskPage::workOrderCreated,
                this, [this](int workorderId, const QString &orderNo) {
            Q_UNUSED(workorderId)
            Q_UNUSED(orderNo)
            qDebug() << "[MainWindow] 工单已创建:" << orderNo;
        });
        connect(fd, &FrontDeskPage::orderNoChanged,
                this, &MainWindow::onFrontDeskOrderNoChanged);

        dlg.exec();
        // 弹窗关闭后清除工单号显示
        m_orderNoLabel->setVisible(false);
    });

    m_menuRepair->addSeparator();

    m_actQuote = m_menuRepair->addAction("工单查询");
    connect(m_actQuote, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_QUOTE);
    });

    // ============================================================
    // 3. 库房管理
    // ============================================================
    m_menuWarehouse = m_menuBar->addMenu("库房管理");

    m_actWarehouse = m_menuWarehouse->addAction("库房工作台");
    m_actWarehouse->setStatusTip("备件领取、材料结算/提单、采购入库、库存查询、退库退货");
    connect(m_actWarehouse, &QAction::triggered, this, [this]() {
        // 库房工作台以独立固定尺寸窗口打开
        QDialog dlg(this);
        dlg.setWindowTitle("库房工作台");
        dlg.setFixedSize(1280, 720);
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        l->setContentsMargins(0, 0, 0, 0);
        WarehousePage *wp = new WarehousePage(&dlg);
        l->addWidget(wp);
        dlg.exec();
    });

    // ============================================================
    // 4. 财务管理
    // ============================================================
    m_menuFinance = m_menuBar->addMenu("财务管理");
    m_actFinance = m_menuFinance->addAction("收入统计 / 成本统计 / 利润分析");
    connect(m_actFinance, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_FINANCE);
    });

    // 服务跟踪
    m_menuService = m_menuBar->addMenu("服务跟踪");
    m_actServiceReminder = m_menuService->addAction("保养提醒");
    connect(m_actServiceReminder, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_SERVICE_REMINDER);
    });
    m_actCustomerVisit = m_menuService->addAction("客户回访");
    connect(m_actCustomerVisit, &QAction::triggered, this, [this]() {
        switchToPage(PAGE_CUSTOMER_VISIT);
    });

    // 报表查询
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

    // ============================================================
    // 按职位应用菜单权限（Session::hasPermission）
    //   经理：全开放
    //   前台/客服：关闭 库房管理；员工管理入口
    //   库管：关闭 前台业务 / 服务跟踪；员工管理入口
    // ============================================================
    m_actEmployee->setVisible(Session::instance().hasPermission("员工管理"));
    m_menuRepair->menuAction()->setVisible(Session::instance().hasPermission("前台业务"));
    m_menuWarehouse->menuAction()->setVisible(Session::instance().hasPermission("库房管理"));
    m_menuService->menuAction()->setVisible(Session::instance().hasPermission("服务跟踪"));
}

void MainWindow::switchToPage(int index)
{
    if (index >= 0 && index < PAGE_COUNT) {
        QWidget *target = m_pages[index];

        // 调用目标页面的refreshData()
        if (auto *p = qobject_cast<EmployeePage*>(target)) {
            p->refreshData();
        } else if (auto *p = qobject_cast<DataManagerPage*>(target)) {
            p->refreshData();
        } else if (auto *p = qobject_cast<QuotePage*>(target)) {
            p->refreshData();
        } else if (auto *p = qobject_cast<ChangePasswordPage*>(target)) {
            p->refreshData();
        } else if (auto *p = qobject_cast<BusinessReportPage*>(target)) {
            p->refreshData();   // 业务流水：每次打开都自动刷新
        } else if (auto *p = qobject_cast<OutboundReportPage*>(target)) {
            p->refreshData();   // 出库报表：每次打开都自动刷新
        }
        // 其他页面没有refreshData或不需要刷新

        m_currentPageIndex = index;
        m_stack->setCurrentIndex(index);
        setWindowTitle(QString("科盟汽修综合数据管理 — %1")
                      .arg(m_menuBar->activeAction() ? m_menuBar->activeAction()->text() : ""));
    }
}

// ============================================================
// 车辆登记保存后的回调（保留向后兼容）
// ============================================================

void MainWindow::onVehicleSavedWithId(int vehicleId, const QString &plateNumber)
{
    Q_UNUSED(vehicleId)
    Q_UNUSED(plateNumber)
    // FrontDeskPage 内部已处理完整流程
}

void MainWindow::onFrontDeskOrderNoChanged(const QString &orderNo)
{
    m_orderNoLabel->setText(
        QString("  当前工单号：%1").arg(orderNo));
    m_orderNoLabel->setVisible(true);
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
