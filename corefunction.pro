QT += core gui sql printsupport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = corefunction
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/database/DbManager.cpp \
    src/database/Session.cpp \
    src/dialogs/LoginDialog.cpp \
    src/dialogs/WorkOrderDetailDialog.cpp \
    src/pages/EmployeePage.cpp \
    src/pages/DataManagerPage.cpp \
    src/pages/FrontDeskPage.cpp \
    src/pages/WarehousePage.cpp \
    src/pages/QuotePage.cpp \
    src/pages/SettlementPage.cpp \
    src/pages/SettlementQueryPage.cpp \
    src/pages/PurchasePage.cpp \
    src/pages/InventoryOutPage.cpp \
    src/pages/PartsReturnPage.cpp \
    src/pages/PurchaseReturnPage.cpp \
    src/pages/StockQueryPage.cpp \
    src/pages/FinancePage.cpp \
    src/pages/ServiceReminderPage.cpp \
    src/pages/CustomerVisitPage.cpp \
    src/pages/ExportPage.cpp \
    src/pages/BusinessReportPage.cpp \
    src/pages/InboundReportPage.cpp \
    src/pages/OutboundReportPage.cpp \
    src/pages/DashboardPage.cpp \
    src/widgets/DateRangeWidget.cpp \
    src/pages/ChangePasswordPage.cpp

HEADERS += \
    mainwindow.h \
    src/database/DbManager.h \
    src/database/Session.h \
    src/dialogs/LoginDialog.h \
    src/dialogs/WorkOrderDetailDialog.h \
    src/pages/EmployeePage.h \
    src/pages/DataManagerPage.h \
    src/pages/FrontDeskPage.h \
    src/pages/WarehousePage.h \
    src/pages/QuotePage.h \
    src/pages/SettlementPage.h \
    src/pages/SettlementQueryPage.h \
    src/pages/PurchasePage.h \
    src/pages/InventoryOutPage.h \
    src/pages/PartsReturnPage.h \
    src/pages/PurchaseReturnPage.h \
    src/pages/StockQueryPage.h \
    src/pages/FinancePage.h \
    src/pages/ServiceReminderPage.h \
    src/pages/CustomerVisitPage.h \
    src/pages/ExportPage.h \
    src/pages/BusinessReportPage.h \
    src/pages/InboundReportPage.h \
    src/pages/OutboundReportPage.h \
    src/pages/DashboardPage.h \
    src/widgets/DateRangeWidget.h \
    src/pages/ChangePasswordPage.h


# 包含路径
INCLUDEPATH += src
INCLUDEPATH += src/database
INCLUDEPATH += src/dialogs
INCLUDEPATH += src/pages
INCLUDEPATH += src/widgets
