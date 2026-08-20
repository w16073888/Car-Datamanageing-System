QT += core gui sql network printsupport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = 4s-client
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# ============================================================
# 源文件（已删除 8 个菜单不可达页面：
#   SettlementPage/SettlementQueryPage/PurchasePage/
#   InventoryOutPage/PartsReturnPage/PurchaseReturnPage/
#   StockQueryPage/ExportPage）
# ============================================================
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/remote/RemoteClient.cpp \
    src/remote/RemoteQuery.cpp \
    src/remote/RemoteModel.cpp \
    src/remote/RemoteDb.cpp \
    src/remote/SqlUtil.cpp \
    src/database/Session.cpp \
    src/dialogs/LoginDialog.cpp \
    src/dialogs/WorkOrderDetailDialog.cpp \
    src/pages/EmployeePage.cpp \
    src/pages/DataManagerPage.cpp \
    src/pages/FrontDeskPage.cpp \
    src/pages/WarehousePage.cpp \
    src/pages/QuotePage.cpp \
    src/pages/FinancePage.cpp \
    src/pages/ServiceReminderPage.cpp \
    src/pages/CustomerVisitPage.cpp \
    src/pages/BusinessReportPage.cpp \
    src/pages/InboundReportPage.cpp \
    src/pages/OutboundReportPage.cpp \
    src/pages/DashboardPage.cpp \
    src/pages/ChangePasswordPage.cpp \
    src/widgets/DateRangeWidget.cpp \
    src/widgets/SearchCompleter.cpp \
    src/utils/XlsxExporter.cpp

HEADERS += \
    mainwindow.h \
    src/remote/JsonProtocol.h \
    src/remote/RemoteClient.h \
    src/remote/RemoteQuery.h \
    src/remote/RemoteModel.h \
    src/remote/RemoteDb.h \
    src/remote/SqlUtil.h \
    src/database/Session.h \
    src/dialogs/LoginDialog.h \
    src/dialogs/WorkOrderDetailDialog.h \
    src/pages/EmployeePage.h \
    src/pages/DataManagerPage.h \
    src/pages/FrontDeskPage.h \
    src/pages/WarehousePage.h \
    src/pages/QuotePage.h \
    src/pages/FinancePage.h \
    src/pages/ServiceReminderPage.h \
    src/pages/CustomerVisitPage.h \
    src/pages/BusinessReportPage.h \
    src/pages/InboundReportPage.h \
    src/pages/OutboundReportPage.h \
    src/pages/DashboardPage.h \
    src/pages/ChangePasswordPage.h \
    src/widgets/DateRangeWidget.h \
    src/widgets/SearchCompleter.h \
    src/utils/XlsxExporter.h

# 包含路径
INCLUDEPATH += src
INCLUDEPATH += src/remote
INCLUDEPATH += src/database
INCLUDEPATH += src/dialogs
INCLUDEPATH += src/pages
INCLUDEPATH += src/widgets
INCLUDEPATH += src/utils

# 资源
RC_ICONS = logo.ico
