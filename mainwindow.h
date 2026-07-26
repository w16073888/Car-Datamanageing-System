#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QLabel>

#include "pages/EmployeePage.h"
#include "pages/DataManagerPage.h"
#include "pages/QuotePage.h"
#include "pages/PurchasePage.h"
#include "pages/InventoryOutPage.h"
#include "pages/PartsReturnPage.h"
#include "pages/PurchaseReturnPage.h"
#include "pages/StockQueryPage.h"
#include "pages/SettlementPage.h"
#include "pages/SettlementQueryPage.h"
#include "pages/FinancePage.h"
#include "pages/ServiceReminderPage.h"
#include "pages/CustomerVisitPage.h"
#include "pages/ExportPage.h"
#include "pages/BusinessReportPage.h"
#include "pages/InboundReportPage.h"
#include "pages/OutboundReportPage.h"
#include "pages/DashboardPage.h"
#include "pages/ChangePasswordPage.h"
#include "pages/FrontDeskPage.h"
#include "pages/WarehousePage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onVehicleSavedWithId(int vehicleId, const QString &plateNumber);
    void onFrontDeskOrderNoChanged(const QString &orderNo);

private:
    void setupMenuBar();
    void setupPages();
    void applyStyleSheet();
    void switchToPage(int index);

    // 页面枚举 — 只保留独立页面，统一入口由 FrontDeskPage / WarehousePage 提供
    enum PageIndex {
        PAGE_EMPLOYEE = 0,
        PAGE_DATA_MANAGER,
        PAGE_FRONT_DESK,        // 前台工作台（车辆登记+派工+打印报价单/工单）
        PAGE_WAREHOUSE,         // 库房工作台（备件领取+材料结算+采购入库+库存查询+退库退货）
        PAGE_SETTLEMENT,        // 工单结算
        PAGE_SETTLEMENT_QUERY,
        PAGE_QUOTE,             // 报价管理
        PAGE_PURCHASE,
        PAGE_INVENTORY_OUT,
        PAGE_PARTS_RETURN,
        PAGE_PURCHASE_RETURN,
        PAGE_STOCK_QUERY,
        PAGE_FINANCE,
        PAGE_SERVICE_REMINDER,
        PAGE_CUSTOMER_VISIT,
        PAGE_EXPORT,
        PAGE_BUSINESS_REPORT,
        PAGE_INBOUND_REPORT,
        PAGE_OUTBOUND_REPORT,
        PAGE_DASHBOARD,
        PAGE_CHANGE_PASSWORD,
        PAGE_COUNT
    };

    QStackedWidget *m_stack;

    // 所有页面
    QWidget *m_pages[PAGE_COUNT];

    // 菜单栏
    QMenuBar *m_menuBar;

    // 系统维护
    QMenu *m_menuSystem;
    QAction *m_actEmployee;
    QAction *m_actDataManager;
    QAction *m_actChangePwd;
    QAction *m_actLogout;

    // 业务报修（前台）
    QMenu *m_menuRepair;
    QAction *m_actFrontDesk;

    // 库房管理（仓库）
    QMenu *m_menuWarehouse;
    QAction *m_actWarehouse;

    // 结算管理
    QMenu *m_menuSettlement;
    QAction *m_actSettlement;
    QAction *m_actSettlementQuery;

    // 报价管理
    QAction *m_actQuote;

    // 财务管理
    QMenu *m_menuFinance;
    QAction *m_actFinance;

    // 服务跟踪
    QMenu *m_menuService;
    QAction *m_actServiceReminder;
    QAction *m_actCustomerVisit;
    QAction *m_actExport;

    // 报表查询
    QMenu *m_menuReport;
    QAction *m_actBusinessReport;
    QAction *m_actInboundReport;
    QAction *m_actOutboundReport;
    QAction *m_actDashboard;

    // 状态栏
    QLabel *m_statusLabel;
    QLabel *m_orderNoLabel;
    int     m_currentPageIndex;
};

#endif // MAINWINDOW_H
