#include "FrontDeskPage.h"
#include "database/DbManager.h"
#include "database/Session.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QScrollArea>
#include <QDialog>
#include <QListWidget>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QTextDocument>
#include <QDateTime>
#include <QDebug>
#include <QKeyEvent>

#define S_BTN1 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#3498db;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN1H S_BTN1 "QPushButton:hover{background:#2980b9;}"
#define S_BTN2 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN2H S_BTN2 "QPushButton:hover{background:#219a52;}"
#define S_BTNG "QPushButton{padding:6px 14px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;font-size:12px;}"
#define S_BTNGH S_BTNG "QPushButton:hover{background:#d5dbdb;}"

// 紧凑输入框样式
#define S_COMPACT \
    "QLineEdit{padding:1px 3px;min-height:18px;}" \
    "QSpinBox{padding:1px 3px;min-height:18px;}" \
    "QDoubleSpinBox{padding:1px 3px;min-height:18px;}" \
    "QComboBox{padding:1px 3px;min-height:18px;}" \
    "QDateEdit{padding:1px 3px;min-height:18px;}" \
    "QTextEdit{padding:1px 3px;}"

FrontDeskPage::FrontDeskPage(QWidget *parent)
    : QWidget(parent), m_lockedVid(0), m_foundVid(0), m_state(STATE_SEARCH) { setupUI(); }
FrontDeskPage::~FrontDeskPage() {}

void FrontDeskPage::refreshData() { resetForm(); }

// ============================================================
// eventFilter — 回车键跳转到下一个输入区域
// ============================================================
bool FrontDeskPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            // 不拦截 QComboBox 的回车（用于选择下拉项）
            if (qobject_cast<QComboBox*>(obj))
                return false;
            // 调用 FrontDeskPage 自身的 focusNextPrevChild（protected 方法，类内可访问）
            focusNextPrevChild(true);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ============================================================
// setupUI
// ============================================================
void FrontDeskPage::setupUI()
{
    // 紧凑样式
    setStyleSheet(S_COMPACT);

    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(10,6,10,6); outer->setSpacing(4);
    QLabel *title = new QLabel("前台工作台");
    title->setStyleSheet("font-size:17px;font-weight:bold;color:#2c3e50;");
    outer->addWidget(title);

    QScrollArea *sa = new QScrollArea; sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *c = new QWidget; QVBoxLayout *cl = new QVBoxLayout(c);
    cl->setContentsMargins(0,0,0,0); cl->setSpacing(4);
    auto L = [](const QString &t){ return new QLabel(t); };

    // ==================== 1. 车辆搜索（多字段） ====================
    m_searchGroup = new QGroupBox("车辆查找");
    QGridLayout *sg = new QGridLayout(m_searchGroup);
    sg->setContentsMargins(6,4,6,4); sg->setSpacing(3);
    sg->setColumnStretch(1,1); sg->setColumnStretch(3,1); sg->setColumnStretch(5,1);

    m_sPlate = new QLineEdit;  m_sPlate->setPlaceholderText("车牌号");
    m_sVin   = new QLineEdit;  m_sVin->setPlaceholderText("VIN");
    m_sEngine = new QLineEdit; m_sEngine->setPlaceholderText("发动机号");
    m_sOwner = new QLineEdit;  m_sOwner->setPlaceholderText("车主");
    m_sPhone = new QLineEdit;  m_sPhone->setPlaceholderText("联系电话");
    m_sModel = new QLineEdit;  m_sModel->setPlaceholderText("车型");

    // 最小宽度设置（控制在窗口内不产生横向滚动条）
    m_sPlate->setMinimumWidth(200);
    m_sVin->setMinimumWidth(400);
    m_sEngine->setMinimumWidth(200);
    m_sOwner->setMinimumWidth(100);
    m_sPhone->setMinimumWidth(200);
    m_sModel->setMinimumWidth(400);

    // 安装回车导航事件过滤器
    for (auto *w : {static_cast<QWidget*>(m_sPlate),static_cast<QWidget*>(m_sVin),static_cast<QWidget*>(m_sEngine),
                    static_cast<QWidget*>(m_sOwner),static_cast<QWidget*>(m_sPhone),static_cast<QWidget*>(m_sModel)})
        w->installEventFilter(this);

    sg->addWidget(L("车牌号:"),0,0); sg->addWidget(m_sPlate,0,1);
    sg->addWidget(L("VIN:"),0,2);    sg->addWidget(m_sVin,0,3);
    sg->addWidget(L("发动机号:"),0,4); sg->addWidget(m_sEngine,0,5);
    sg->addWidget(L("车主:"),1,0);   sg->addWidget(m_sOwner,1,1);
    sg->addWidget(L("电话:"),1,2);   sg->addWidget(m_sPhone,1,3);
    sg->addWidget(L("车型:"),1,4);   sg->addWidget(m_sModel,1,5);

    QHBoxLayout *btnRow = new QHBoxLayout;
    m_btnLock = new QPushButton("锁定车辆");   m_btnLock->setStyleSheet(S_BTN2H);
    m_lblStatus = new QLabel("未锁定");
    m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;font-size:13px;");
    btnRow->addWidget(m_btnLock);
    btnRow->addWidget(m_lblStatus); btnRow->addStretch();
    sg->addLayout(btnRow,2,0,1,6);

    cl->addWidget(m_searchGroup);

    // ==================== 2. 车辆信息展示（锁定后只读） ====================
    m_infoGroup = new QGroupBox("当前车辆信息");
    m_infoGroup->setVisible(false);
    QVBoxLayout *igOuter = new QVBoxLayout(m_infoGroup);
    igOuter->setContentsMargins(4,2,4,2); igOuter->setSpacing(2);

    QString sEdit = "padding:0px;min-height:18px;color:#000000;"
                    "border:1px solid #dcdde1;border-radius:3px;background:#fff;";
    auto mkEdit = [&](QLineEdit *&e) {
        e = new QLineEdit;
        e->setStyleSheet(sEdit);
    };
    mkEdit(m_dispPlate); mkEdit(m_dispVin); mkEdit(m_dispEngine); mkEdit(m_dispModel);
    mkEdit(m_dispOwner); mkEdit(m_dispPhone); mkEdit(m_dispAddress);
    m_dispColor = new QComboBox; m_dispColor->setEditable(true);
    m_dispColor->addItems({"","白","黑","银","红","蓝","绿","灰","黄","棕","橙","紫"});
    m_dispColor->setFixedWidth(50);
    m_dispFuel  = new QComboBox; m_dispFuel->setEditable(true);
    m_dispFuel->addItems({"","汽油","柴油","电动","混动","天然气"});
    m_dispFuel->setFixedWidth(60);
    m_dispTrans = new QComboBox; m_dispTrans->setEditable(true);
    m_dispTrans->addItems({"","自动","手动","无级变速","双离合","AMT"});
    m_dispTrans->setFixedWidth(80);
    m_dispPurchase = new QDateEdit;
    m_dispPurchase->setCalendarPopup(true);
    m_dispPurchase->setDisplayFormat("yyyy-MM-dd");
    m_dispPurchase->setStyleSheet(sEdit);
    // 固定宽度
    m_dispPlate->setFixedWidth(90);
    m_dispVin->setFixedWidth(170);
    m_dispEngine->setFixedWidth(150);
    m_dispModel->setFixedWidth(150);
    m_dispOwner->setFixedWidth(50);
    m_dispPhone->setFixedWidth(100);
    m_dispAddress->setFixedWidth(150);

    // 第0行：车牌 + VIN + 发动机 + 车型
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(3);
        row->addWidget(L("车牌:")); row->addWidget(m_dispPlate);
        row->addWidget(L("VIN:"));  row->addWidget(m_dispVin);
        row->addWidget(L("发动机:")); row->addWidget(m_dispEngine);
        row->addWidget(L("车型:")); row->addWidget(m_dispModel);
        row->addStretch();
        igOuter->addLayout(row);
    }
    // 第1行：车主 + 电话 + 地址 + 颜色 + 油类 + 变速箱 + 购车日期
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(3);
        row->addWidget(L("车主:")); row->addWidget(m_dispOwner);
        row->addWidget(L("电话:")); row->addWidget(m_dispPhone);
        row->addWidget(L("地址:")); row->addWidget(m_dispAddress);
        row->addWidget(L("颜色:")); row->addWidget(m_dispColor);
        row->addWidget(L("油类:")); row->addWidget(m_dispFuel);
        row->addWidget(L("变速箱:")); row->addWidget(m_dispTrans);
        row->addWidget(L("购车:")); row->addWidget(m_dispPurchase);
        row->addStretch();
        igOuter->addLayout(row);
    }
    // 第2行：保存修改 + 维修历史按钮
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_btnSaveVehicleInfo = new QPushButton("保存修改");
        m_btnSaveVehicleInfo->setStyleSheet(
            "QPushButton{padding:4px 12px;border:none;border-radius:3px;"
            "background:#27ae60;color:#fff;font-size:12px;font-weight:bold;}"
            "QPushButton:hover{background:#219a52;}");
        m_btnSaveVehicleInfo->setMinimumHeight(24);
        row->addWidget(m_btnSaveVehicleInfo);
        row->addStretch();
        m_btnMaintenanceHistory = new QPushButton("维修历史");
        m_btnMaintenanceHistory->setStyleSheet(
            "QPushButton{padding:4px 12px;border:none;border-radius:3px;"
            "background:#2980b9;color:#fff;font-size:12px;font-weight:bold;}"
            "QPushButton:hover{background:#2471a3;}");
        m_btnMaintenanceHistory->setMinimumHeight(24);
        row->addWidget(m_btnMaintenanceHistory);
        igOuter->addLayout(row);
    }
    cl->addWidget(m_infoGroup);

    // ==================== 3. 新车录入 ====================
    m_newGroup = new QGroupBox("新车录入（未找到时填写）");
    m_newGroup->setVisible(false);
    QGridLayout *ng = new QGridLayout(m_newGroup);
    ng->setContentsMargins(6,4,6,4); ng->setSpacing(3);
    ng->setColumnStretch(1,1); ng->setColumnStretch(3,1); ng->setColumnStretch(5,1);

    m_nPlate = new QLineEdit;   m_nPlate->setPlaceholderText("*必填");
    m_nVin   = new QLineEdit;
    m_nEngine = new QLineEdit;
    m_nModel = new QLineEdit;   m_nModel->setPlaceholderText("*必填");
    m_nOwner = new QLineEdit;   m_nOwner->setPlaceholderText("*必填");
    m_nPhone = new QLineEdit;   m_nPhone->setPlaceholderText("*必填");
    m_nAddress = new QLineEdit;
    m_nColor = new QComboBox; m_nColor->setEditable(true); m_nColor->addItems({"","白","黑","银","红","蓝","绿","灰","黄","棕","橙","紫"});
    m_nFuel  = new QComboBox; m_nFuel->setEditable(true);  m_nFuel->addItems({"","汽油","柴油","电动","混动","天然气"});
    m_nTrans = new QComboBox; m_nTrans->setEditable(true); m_nTrans->addItems({"","自动","手动","无级变速","双离合","AMT"});
    m_nPurchase = new QDateEdit; m_nPurchase->setCalendarPopup(true); m_nPurchase->setDisplayFormat("yyyy-MM-dd"); m_nPurchase->setDate(QDate::currentDate());

    // 最小宽度设置（控制在窗口内不产生横向滚动条）
    m_nPlate->setMinimumWidth(200);
    m_nVin->setMinimumWidth(400);
    m_nEngine->setMinimumWidth(200);
    m_nOwner->setMinimumWidth(100);
    m_nPhone->setMinimumWidth(200);
    m_nModel->setMinimumWidth(400);

    // 安装回车导航事件过滤器
    for (auto *w : {static_cast<QWidget*>(m_nPlate),static_cast<QWidget*>(m_nVin),static_cast<QWidget*>(m_nEngine),
                    static_cast<QWidget*>(m_nModel),static_cast<QWidget*>(m_nOwner),static_cast<QWidget*>(m_nPhone),
                    static_cast<QWidget*>(m_nAddress)})
        w->installEventFilter(this);

    ng->addWidget(L("车牌*:"),0,0); ng->addWidget(m_nPlate,0,1);
    ng->addWidget(L("VIN:"),0,2);   ng->addWidget(m_nVin,0,3);
    ng->addWidget(L("发动机:"),0,4); ng->addWidget(m_nEngine,0,5);
    // 第1行：车型* + 颜色 + 油类 + 变速箱 四合一
    {
        QHBoxLayout *row1 = new QHBoxLayout;
        row1->addWidget(L("车型*:"));
        row1->addWidget(m_nModel, 1);
        row1->addWidget(L("颜色:"));
        row1->addWidget(m_nColor);
        row1->addWidget(L("油类:"));
        row1->addWidget(m_nFuel);
        row1->addWidget(L("变速箱:"));
        row1->addWidget(m_nTrans);
        ng->addLayout(row1,1,0,1,6);
    }
    // 第2行：车主* + 电话* + 地址
    ng->addWidget(L("车主*:"),2,0); ng->addWidget(m_nOwner,2,1);
    ng->addWidget(L("电话*:"),2,2); ng->addWidget(m_nPhone,2,3);
    ng->addWidget(L("地址:"),2,4);  ng->addWidget(m_nAddress,2,5);
    ng->addWidget(L("购车时间:"),3,0); ng->addWidget(m_nPurchase,3,1);

    // 保存 + 取消按钮
    {
        QHBoxLayout *rowBtn = new QHBoxLayout;
        rowBtn->addStretch();
        m_btnSaveNewCar = new QPushButton("保存");
        m_btnSaveNewCar->setStyleSheet(S_BTN2H);
        m_btnSaveNewCar->setMinimumHeight(28);
        rowBtn->addWidget(m_btnSaveNewCar);
        m_btnCancelNewCar = new QPushButton("取消");
        m_btnCancelNewCar->setStyleSheet(S_BTNGH);
        m_btnCancelNewCar->setMinimumHeight(28);
        rowBtn->addWidget(m_btnCancelNewCar);
        ng->addLayout(rowBtn, 4, 0, 1, 6);
    }

    cl->addWidget(m_newGroup);

    // ==================== 4. 派工区 ====================
    m_dispatchGroup = new QGroupBox("派工信息");
    QVBoxLayout *dg = new QVBoxLayout(m_dispatchGroup);
    dg->setContentsMargins(4,2,4,2); dg->setSpacing(2);

    m_editOrderNo = new QLineEdit; m_editOrderNo->setReadOnly(true);
    m_editOrderNo->setStyleSheet("background:#f0f0f0;");
    m_editOrderNo->setFixedWidth(150);
    m_cmbAdvisor  = new QComboBox;   m_cmbAdvisor->setFixedWidth(50);
    m_cmbMainTech = new QComboBox;   m_cmbMainTech->setMaximumWidth(150);
    m_spinMileage = new QSpinBox;    m_spinMileage->setRange(0,9999999); m_spinMileage->setSuffix(" km");
    m_spinMileage->setFixedWidth(100);
    m_dateRepair = new QDateEdit;    m_dateRepair->setCalendarPopup(true);
    m_dateRepair->setDisplayFormat("yyyy-MM-dd"); m_dateRepair->setDate(QDate::currentDate());
    m_dateRepair->setFixedWidth(100);
    m_dateEstimated = new QDateEdit; m_dateEstimated->setCalendarPopup(true);
    m_dateEstimated->setDisplayFormat("yyyy-MM-dd"); m_dateEstimated->setDate(QDate::currentDate());
    m_dateEstimated->setFixedWidth(100);
    m_cmbShift = new QComboBox;      m_cmbShift->addItems({"","白班","夜班"});
    m_cmbShift->setFixedWidth(50);
    m_textContent = new QTextEdit;   m_textContent->setPlaceholderText("选填");
    m_textContent->setFixedHeight(40);  // 约两行高度
    m_textContent->setFixedWidth(400);
    m_textContent->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 安装回车导航事件过滤器
    m_editOrderNo->installEventFilter(this);
    m_spinMileage->installEventFilter(this);
    m_dateRepair->installEventFilter(this);
    m_dateEstimated->installEventFilter(this);

    // 第0行：工单号 + 顾问 + 主修 + 公里数 + 报修日期 + 预估完工 + 班别
    {
        QHBoxLayout *row0 = new QHBoxLayout;
        row0->setSpacing(3);
        row0->addWidget(L("工单号:")); row0->addWidget(m_editOrderNo);
        row0->addWidget(L("顾问:"));   row0->addWidget(m_cmbAdvisor);
        row0->addWidget(L("主修:"));   row0->addWidget(m_cmbMainTech, 1);
        row0->addWidget(L("公里数:")); row0->addWidget(m_spinMileage);
        row0->addWidget(L("报修:"));   row0->addWidget(m_dateRepair);
        row0->addWidget(L("完工:"));   row0->addWidget(m_dateEstimated);
        row0->addWidget(L("班别:"));   row0->addWidget(m_cmbShift);
        dg->addLayout(row0);
    }
    // 第1行：内容 + 取消按钮 同行右侧
    {
        QHBoxLayout *row2 = new QHBoxLayout;
        row2->setSpacing(3);
        row2->addWidget(L("内容:"));
        row2->addWidget(m_textContent);
        row2->addStretch();
        m_btnCancelDispatch = new QPushButton("取消派工");
        m_btnCancelDispatch->setStyleSheet(S_BTNGH);
        m_btnCancelDispatch->setMinimumHeight(28);
        row2->addWidget(m_btnCancelDispatch);
        dg->addLayout(row2);
    }

    cl->addWidget(m_dispatchGroup);

    // ==================== 5. 报修内容（多列布局，左右滚动） ====================
    m_repairGroup = new QGroupBox("报修内容");
    {
        QVBoxLayout *outerRV = new QVBoxLayout(m_repairGroup);
        outerRV->setContentsMargins(4,4,4,4);

        m_repairScroll = new QScrollArea;
        m_repairScroll->setWidgetResizable(true);
        m_repairScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_repairScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_repairScroll->setFixedHeight(240);

        m_repairColumns = new QWidget;
        m_columnsLayout = new QHBoxLayout(m_repairColumns);
        m_columnsLayout->setContentsMargins(0,0,0,0);
        m_columnsLayout->setSpacing(8);

        m_repairScroll->setWidget(m_repairColumns);
        outerRV->addWidget(m_repairScroll);
    }
    cl->addWidget(m_repairGroup);

    // 初始空行（机电/钣金/喷漆 各一行）
    addRepairRow("机电");
    addRepairRow("钣金");
    addRepairRow("喷漆");

    // ==================== 6. 费用 ====================
    m_feeGroup = new QGroupBox("费用明细");
    QVBoxLayout *fg = new QVBoxLayout(m_feeGroup);
    fg->setContentsMargins(4,2,4,2); fg->setSpacing(2);
    m_spinMat   = new QDoubleSpinBox; m_spinMat->setRange(0,999999.99);   m_spinMat->setPrefix("¥ "); m_spinMat->setDecimals(2); m_spinMat->setMaximumWidth(110);
    m_spinOther = new QDoubleSpinBox; m_spinOther->setRange(0,999999.99); m_spinOther->setPrefix("¥ "); m_spinOther->setDecimals(2); m_spinOther->setMaximumWidth(110);
    m_spinMgmt  = new QDoubleSpinBox; m_spinMgmt->setRange(0,999999.99);  m_spinMgmt->setPrefix("¥ "); m_spinMgmt->setDecimals(2); m_spinMgmt->setMaximumWidth(110);
    m_spinDep   = new QDoubleSpinBox; m_spinDep->setRange(0,999999.99);   m_spinDep->setPrefix("¥ ");  m_spinDep->setDecimals(2); m_spinDep->setMaximumWidth(110);

    // 安装回车导航
    m_spinMat->installEventFilter(this);
    m_spinOther->installEventFilter(this);
    m_spinMgmt->installEventFilter(this);
    m_spinDep->installEventFilter(this);

    connect(m_spinMat, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    connect(m_spinOther, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    connect(m_spinMgmt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);

    // 公式费合计
    m_lblFormulaFee = new QLabel("¥ 0.00");
    m_lblFormulaFee->setStyleSheet("font-size:14px;font-weight:bold;color:#2980b9;");
    // 第0行：费用输入 + 合计，同一行
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(3);
        row->addWidget(L("材料费:")); row->addWidget(m_spinMat);
        row->addWidget(L("其它费:")); row->addWidget(m_spinOther);
        row->addWidget(L("管理费:")); row->addWidget(m_spinMgmt);
        row->addWidget(L("订金:"));   row->addWidget(m_spinDep);
        row->addWidget(L("公式费合计:")); row->addWidget(m_lblFormulaFee);
        m_lblTotal = new QLabel("¥ 0.00");
        m_lblTotal->setStyleSheet("font-size:16px;font-weight:bold;color:#e74c3c;");
        row->addWidget(L("合计:")); row->addWidget(m_lblTotal);
        row->addStretch();
        fg->addLayout(row);
    }
    // 第1行：操作按钮
    {
        QHBoxLayout *br = new QHBoxLayout;
        br->setSpacing(4);
        m_btnPrint = new QPushButton("打印工单(内部)");
        m_btnPrint->setStyleSheet("QPushButton{padding:4px 10px;border:none;border-radius:3px;background:#8e44ad;color:#fff;font-size:12px;font-weight:bold;}QPushButton:hover{background:#7d3c98;}");
        m_btnCreate = new QPushButton("保存并派工");
        m_btnCreate->setStyleSheet(S_BTN1H);
        m_btnCreate->setMinimumHeight(28); m_btnPrint->setMinimumHeight(28);

        QPushButton *btnPrintQuote = new QPushButton("打印报价单(客户)");
        btnPrintQuote->setStyleSheet("QPushButton{padding:4px 10px;border:none;border-radius:3px;background:#16a085;color:#fff;font-size:12px;font-weight:bold;}QPushButton:hover{background:#138d75;}");
        btnPrintQuote->setMinimumHeight(28);
        connect(btnPrintQuote, &QPushButton::clicked, this, &FrontDeskPage::onPrintQuote);

        QPushButton *btnExportQuotePdf = new QPushButton("导出报价单PDF");
        btnExportQuotePdf->setStyleSheet("QPushButton{padding:4px 10px;border:none;border-radius:3px;background:#c0392b;color:#fff;font-size:12px;font-weight:bold;}QPushButton:hover{background:#a93226;}");
        btnExportQuotePdf->setMinimumHeight(28);
        connect(btnExportQuotePdf, &QPushButton::clicked, this, &FrontDeskPage::onExportQuotePdf);

        br->addStretch();
        br->addWidget(btnPrintQuote);
        br->addWidget(btnExportQuotePdf);
        br->addWidget(m_btnPrint);
        br->addWidget(m_btnCreate);
        fg->addLayout(br);
    }
    cl->addWidget(m_feeGroup);
    cl->addStretch();

    sa->setWidget(c); outer->addWidget(sa, 1);

    // ==================== 信号 ====================
    connect(m_btnLock,   &QPushButton::clicked, this, &FrontDeskPage::onLockVehicle);
    connect(m_btnCreate, &QPushButton::clicked, this, &FrontDeskPage::onCreateWorkOrder);
    connect(m_btnPrint,  &QPushButton::clicked, this, &FrontDeskPage::onPrintWorkOrder);
    connect(m_btnMaintenanceHistory, &QPushButton::clicked, this, &FrontDeskPage::onShowMaintenanceHistory);
    connect(m_btnSaveVehicleInfo, &QPushButton::clicked, this, &FrontDeskPage::onSaveVehicleInfo);
    connect(m_btnSaveNewCar,   &QPushButton::clicked, this, &FrontDeskPage::onSaveNewCar);
    connect(m_btnCancelNewCar, &QPushButton::clicked, this, &FrontDeskPage::onCancelNewCar);
    connect(m_btnCancelDispatch, &QPushButton::clicked, this, &FrontDeskPage::onCancelDispatch);

    // 搜索：回车/Tab/失焦触发搜索
    QList<QLineEdit*> searchFields = {m_sPlate, m_sVin, m_sEngine, m_sOwner, m_sPhone, m_sModel};
    for (auto *e : searchFields) {
        connect(e, &QLineEdit::editingFinished, this, &FrontDeskPage::triggerFuzzySearch);
    }

    loadCombos(); resetForm();
    setState(STATE_SEARCH);   // 初始：仅显示车辆查找
}

// ============================================================
// addRepairRow — 添加报修内容行
// ============================================================
void FrontDeskPage::addRepairRow(const QString &type)
{
    ItemRow row;
    row.container = nullptr;
    row.type = type;
    row.tech = new QComboBox;
    row.tech->setMinimumWidth(90);
    row.content = new QLineEdit;
    row.content->setPlaceholderText("内容");
    row.content->setFixedWidth(250);
    row.fee = new QDoubleSpinBox;
    row.fee->setRange(0, 999999.99);
    row.fee->setPrefix("¥ ");
    row.fee->setDecimals(2);
    row.fee->setMinimumWidth(100);

    // 加载技师列表
    row.tech->clear();
    row.tech->addItem("", 0);
    {
        QSqlQuery q(DbManager::instance().database());
        q.exec("SELECT id,name FROM t_employee WHERE is_active=1 ORDER BY name");
        while (q.next())
            row.tech->addItem(q.value(1).toString(), q.value(0).toInt());
    }

    // 费用变化 → 刷新合计
    connect(row.fee, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FrontDeskPage::onFeeChanged);

    // 内容编辑完成 → 如果同类最后一行的内容非空，自动新增同类型空行
    QString typeCapture = type;
    connect(row.content, &QLineEdit::editingFinished, this, [this, typeCapture]() {
        // 找该类最后一行的指针
        ItemRow *lastOfType = nullptr;
        for (int i = m_allRows.size() - 1; i >= 0; i--) {
            if (m_allRows[i].type == typeCapture) {
                lastOfType = &m_allRows[i];
                break;
            }
        }
        if (!lastOfType) return;
        bool lastHasContent = !lastOfType->content->text().trimmed().isEmpty()
                           || lastOfType->fee->value() > 0
                           || lastOfType->tech->currentData().toInt() > 0;
        if (lastHasContent)
            addRepairRow(typeCapture);
    });

    // 安装回车导航事件过滤器
    row.tech->installEventFilter(this);
    row.content->installEventFilter(this);
    row.fee->installEventFilter(this);

    m_allRows.append(row);
    rebuildRepairLayout();
}

// ============================================================
// rebuildRepairLayout — 多列布局，每列最多 11 行，同类放一块
// ============================================================
void FrontDeskPage::rebuildRepairLayout()
{
    // 清空旧布局
    QLayoutItem *child;
    while ((child = m_columnsLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }

    if (m_allRows.isEmpty()) return;

    // 按类型分组，保持插入顺序: 机电 → 钣金 → 喷漆
    QStringList typeOrder = {"机电", "钣金", "喷漆"};
    QList<ItemRow*> ordered;
    for (const QString &t : typeOrder) {
        for (auto &row : m_allRows) {
            if (row.type == t)
                ordered.append(&row);
        }
    }

    // 分列：每列最多 MAX_ROWS_PER_COLUMN 行
    for (int colStart = 0; colStart < ordered.size(); colStart += MAX_ROWS_PER_COLUMN) {
        int colEnd = qMin(colStart + MAX_ROWS_PER_COLUMN, ordered.size());

        QWidget *colWidget = new QWidget;
        QVBoxLayout *colLayout = new QVBoxLayout(colWidget);
        colLayout->setContentsMargins(2, 0, 2, 0);
        colLayout->setSpacing(2);

        // 列头
        {
            QHBoxLayout *hdr = new QHBoxLayout;
            QLabel *lblTech = new QLabel("维修人");
            lblTech->setStyleSheet("font-weight:bold;");
            lblTech->setFixedWidth(92);
            QLabel *lblContent = new QLabel("内容");
            lblContent->setStyleSheet("font-weight:bold;");
            lblContent->setFixedWidth(250);
            QLabel *lblFee = new QLabel("费用");
            lblFee->setStyleSheet("font-weight:bold;");
            lblFee->setFixedWidth(100);
            hdr->addWidget(lblTech);
            hdr->addWidget(lblContent);
            hdr->addWidget(lblFee);
            hdr->addStretch();
            colLayout->addLayout(hdr);
        }

        // 本列的行，同类型连续时显示类型标签
        QString prevType;
        for (int i = colStart; i < colEnd; i++) {
            ItemRow *row = ordered[i];

            // 类型切换时插入标签
            if (row->type != prevType) {
                QLabel *typeLabel = new QLabel(row->type);
                typeLabel->setStyleSheet("font-weight:bold;color:#2c3e50;padding:2px 0;background:#ecf0f1;");
                colLayout->addWidget(typeLabel);
                prevType = row->type;
            }

            // 创建行容器（若尚未创建）
            if (!row->container) {
                row->container = new QWidget;
                QHBoxLayout *rowLayout = new QHBoxLayout(row->container);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(3);
                rowLayout->addWidget(row->tech);
                rowLayout->addWidget(row->content);
                rowLayout->addWidget(row->fee);
                rowLayout->addStretch();
            }
            colLayout->addWidget(row->container);
        }

        colLayout->addStretch();
        m_columnsLayout->addWidget(colWidget);
    }
}

// ============================================================
// loadCombos
// ============================================================
void FrontDeskPage::loadCombos()
{
    m_cmbAdvisor->clear(); m_cmbAdvisor->addItem("",0);
    QSqlQuery q(DbManager::instance().database());
    q.exec("SELECT id,name FROM t_employee WHERE position IN ('前台','经理') AND is_active=1 ORDER BY name");
    while (q.next()) m_cmbAdvisor->addItem(q.value(1).toString(), q.value(0).toInt());
    // 自动填入当前操作者
    {
        int curId = Session::instance().userId();
        for (int i = 0; i < m_cmbAdvisor->count(); i++) {
            if (m_cmbAdvisor->itemData(i).toInt() == curId) {
                m_cmbAdvisor->setCurrentIndex(i);
                break;
            }
        }
    }

    m_cmbMainTech->clear(); m_cmbMainTech->addItem("",0);
    q.exec("SELECT id,name,position FROM t_employee WHERE is_active=1 ORDER BY name");
    while (q.next()) m_cmbMainTech->addItem(QString("%1(%2)").arg(q.value(1).toString(),q.value(2).toString()), q.value(0).toInt());

    // 报修内容的技师列表在 addRepairRow() 中各自加载
}

// ============================================================
// 辅助
// ============================================================
QString FrontDeskPage::generateOrderNo()
{
    QSqlQuery q(DbManager::instance().database());
    QString p = QDate::currentDate().toString("yyyyMMdd");
    q.prepare("SELECT COUNT(*) FROM t_workorder WHERE order_no LIKE :p");
    q.bindValue(":p","WO"+p+"%");
    DbManager::instance().executeQuery(q);
    int c = 0; if (q.next()) c = q.value(0).toInt();
    return QString("WO%1%2").arg(p).arg(c+1,4,10,QChar('0'));
}

void FrontDeskPage::resetForm()
{
    m_lockedVid = 0; m_foundVid = 0;
    m_sPlate->clear(); m_sVin->clear(); m_sEngine->clear();
    m_sOwner->clear(); m_sPhone->clear(); m_sModel->clear();
    clearGhost();
    m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");

    m_editOrderNo->setText(generateOrderNo());
    emit orderNoChanged(m_editOrderNo->text());
    m_spinMileage->setValue(0);
    // 顾问保持当前用户选中
    {
        int curId = Session::instance().userId();
        for (int i = 0; i < m_cmbAdvisor->count(); i++) {
            if (m_cmbAdvisor->itemData(i).toInt() == curId) {
                m_cmbAdvisor->setCurrentIndex(i);
                break;
            }
        }
    }
    m_cmbMainTech->setCurrentIndex(0); m_textContent->clear();
    m_dateRepair->setDate(QDate::currentDate()); m_dateEstimated->setDate(QDate::currentDate());
    m_cmbShift->setCurrentIndex(0);

    // 清空动态报修内容行
    for (auto &row : m_allRows) {
        if (row.container)
            delete row.container;
    }
    m_allRows.clear();

    // 重置为每类一个空行
    addRepairRow("机电");
    addRepairRow("钣金");
    addRepairRow("喷漆");

    m_spinMat->setValue(0); m_spinOther->setValue(0); m_spinMgmt->setValue(0); m_spinDep->setValue(0);
    m_lblTotal->setText("¥ 0.00");
    m_lblFormulaFee->setText("¥ 0.00");

    // 重置新车录入表单
    m_nPlate->clear(); m_nVin->clear(); m_nEngine->clear();
    m_nModel->clear(); m_nOwner->clear(); m_nPhone->clear(); m_nAddress->clear();
    m_nColor->setCurrentIndex(0); m_nFuel->setCurrentIndex(0); m_nTrans->setCurrentIndex(0);
    m_nPurchase->setDate(QDate::currentDate());
}

double FrontDeskPage::calcRepairFee()
{
    double t = 0;
    for (auto &row : m_allRows)
        t += row.fee->value();
    return t;
}

double FrontDeskPage::calcTotalFee()
{
    return calcRepairFee() + m_spinMat->value() + m_spinOther->value() + m_spinMgmt->value();
}

void FrontDeskPage::onFeeChanged()
{
    m_lblFormulaFee->setText(QString("¥ %1").arg(calcRepairFee(),0,'f',2));
    m_lblTotal->setText(QString("¥ %1").arg(calcTotalFee(),0,'f',2));
}

// ============================================================
// 多字段模糊搜索
// ============================================================
void FrontDeskPage::triggerFuzzySearch()
{
    // 车辆已锁定时，不响应搜索字段的自动变化，防止误清除已显示的车辆信息
    // 用户需先点击"解锁"按钮才能搜索其他车辆
    if (m_lockedVid > 0)
        return;

    QString f[6] = {m_sPlate->text().trimmed(), m_sVin->text().trimmed(), m_sEngine->text().trimmed(),
                    m_sOwner->text().trimmed(), m_sPhone->text().trimmed(), m_sModel->text().trimmed()};
    bool any = false; for (auto &s : f) if (!s.isEmpty()) { any = true; break; }
    if (!any) {
        m_foundVid = 0; clearGhost(); m_lblStatus->setText("未锁定");
        m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;"); return;
    }

    QString sql = "SELECT DISTINCT v.id, v.plate_number, v.vin, v.engine_number, v.model, "
                  "c.name, c.phone FROM t_vehicle v "
                  "LEFT JOIN t_customer c ON c.vehicle_id=v.id WHERE ";
    QStringList conds;
    if (!f[0].isEmpty()) conds << "v.plate_number LIKE :p0";
    if (!f[1].isEmpty()) conds << "v.vin LIKE :p1";
    if (!f[2].isEmpty()) conds << "v.engine_number LIKE :p2";
    if (!f[3].isEmpty()) conds << "c.name LIKE :p3";
    if (!f[4].isEmpty()) conds << "c.phone LIKE :p4";
    if (!f[5].isEmpty()) conds << "v.model LIKE :p5";

    if (conds.isEmpty()) return;
    sql += "(" + conds.join(" OR ") + ") ORDER BY v.id DESC LIMIT 30";

    QSqlQuery q(DbManager::instance().database());
    q.prepare(sql);
    if (!f[0].isEmpty()) q.bindValue(":p0", "%"+f[0]+"%");
    if (!f[1].isEmpty()) q.bindValue(":p1", "%"+f[1]+"%");
    if (!f[2].isEmpty()) q.bindValue(":p2", "%"+f[2]+"%");
    if (!f[3].isEmpty()) q.bindValue(":p3", "%"+f[3]+"%");
    if (!f[4].isEmpty()) q.bindValue(":p4", "%"+f[4]+"%");
    if (!f[5].isEmpty()) q.bindValue(":p5", "%"+f[5]+"%");

    if (!DbManager::instance().executeQuery(q)) {
        qWarning() << "[FrontDesk] 模糊搜索失败:" << q.lastError().text();
        return;
    }

    struct VItem { int id; QString plate, detail; };
    QList<VItem> items;
    while (q.next()) {
        VItem vi;
        vi.id = q.value(0).toInt();
        vi.plate = q.value(1).toString();
        QStringList parts;
        parts << q.value(1).toString();
        if (!q.value(3).toString().isEmpty()) parts << "发动机:"+q.value(3).toString();
        if (!q.value(4).toString().isEmpty()) parts << q.value(4).toString();
        if (!q.value(5).toString().isEmpty()) parts << "车主:"+q.value(5).toString();
        if (!q.value(6).toString().isEmpty()) parts << "电话:"+q.value(6).toString();
        vi.detail = parts.join(" | ");
        items << vi;
    }

    if (items.isEmpty()) {
        m_foundVid = 0; clearGhost();
        m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
        setState(STATE_NEW_CAR);
        return;
    }

    if (items.size() == 1) {
        m_lockedVid = items[0].id;
        m_foundVid = 0;
        clearGhost();
        fillVehicleData(items[0].id);
        m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
        m_btnMaintenanceHistory->setVisible(true);
        m_editOrderNo->setText(generateOrderNo());
        emit orderNoChanged(m_editOrderNo->text());
        setState(STATE_DISPATCH);
        return;
    }

    // 多条 → 弹窗选择列表
    QDialog dlg(this); dlg.setWindowTitle("选择车辆");
    dlg.resize(600, 400);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);
    QLabel *hint = new QLabel(QString("找到 %1 辆车，请选择:").arg(items.size()));
    dl->addWidget(hint);
    QListWidget *list = new QListWidget;
    list->setAlternatingRowColors(true);
    list->setStyleSheet("QListWidget::item{padding:10px;border-bottom:1px solid #ecf0f1;font-size:14px;}");
    for (auto &vi : items) {
        QListWidgetItem *item = new QListWidgetItem(vi.detail);
        item->setData(Qt::UserRole, vi.id);
        item->setData(Qt::UserRole+1, vi.plate);
        list->addItem(item);
    }
    dl->addWidget(list, 1);
    QHBoxLayout *bb = new QHBoxLayout;
    QPushButton *ok = new QPushButton("选择"); ok->setStyleSheet(S_BTN1H);
    QPushButton *ca = new QPushButton("取消"); ca->setStyleSheet(S_BTNGH);
    bb->addStretch(); bb->addWidget(ok); bb->addWidget(ca);
    dl->addLayout(bb);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(ca, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(list, &QListWidget::doubleClicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted && list->currentItem()) {
        m_lockedVid = list->currentItem()->data(Qt::UserRole).toInt();
        m_foundVid = 0;
        clearGhost();
        fillVehicleData(m_lockedVid);
        m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
        m_btnMaintenanceHistory->setVisible(true);
        m_editOrderNo->setText(generateOrderNo());
        emit orderNoChanged(m_editOrderNo->text());
        setState(STATE_DISPATCH);
    } else {
        m_lockedVid = 0;
        m_foundVid = 0;
    }
}

// ============================================================
// fillVehicleData
// ============================================================
void FrontDeskPage::fillVehicleData(int vid)
{
    qDebug() << "[fillVehicleData] 被调用, vid =" << vid;

    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT v.plate_number,v.vin,v.engine_number,v.model,"
              "v.color,v.fuel_type,v.transmission,v.current_mileage,"
              "v.purchase_date,c.name,c.phone,c.address "
              "FROM t_vehicle v LEFT JOIN t_customer c ON c.vehicle_id=v.id AND c.type='车主' "
              "WHERE v.id=:id");
    q.bindValue(":id", vid);
    bool ok = DbManager::instance().executeQuery(q);
    if (!ok || !q.next()) {
        qWarning() << "[fillVehicleData] 查询失败或无数据, ok=" << ok
                   << ", error:" << q.lastError().text();
        return;
    }

    qDebug() << "[fillVehicleData] 车牌:" << q.value(0).toString()
             << " VIN:" << q.value(1).toString()
             << " 车主:" << q.value(9).toString()
             << " 电话:" << q.value(10).toString();

    m_dispPlate->setText(q.value(0).toString());
    m_dispVin->setText(q.value(1).toString());
    m_dispEngine->setText(q.value(2).toString());
    m_dispModel->setText(q.value(3).toString());
    m_dispColor->setCurrentText(q.value(4).toString());
    m_dispFuel->setCurrentText(q.value(5).toString());
    m_dispTrans->setCurrentText(q.value(6).toString());
    m_spinMileage->setValue(q.value(7).toInt());
    m_dispPurchase->setDate(q.value(8).toDate());
    m_dispOwner->setText(q.value(9).toString());
    m_dispPhone->setText(q.value(10).toString());
    m_dispAddress->setText(q.value(11).toString());
    m_btnMaintenanceHistory->setVisible(true);

    qDebug() << "[fillVehicleData] 填充完成, m_dispPlate->text() =" << m_dispPlate->text();
}

// ============================================================
// 锁定
// ============================================================
void FrontDeskPage::onLockVehicle()
{
    if (m_lockedVid > 0) {
        QMessageBox::information(this, "已锁定", "车辆已锁定: " + m_dispPlate->text());
        return;
    }

    if (!m_sPlate->text().trimmed().isEmpty()) {
        triggerFuzzySearch();
        if (m_lockedVid > 0) {
            QMessageBox::information(this, "锁定成功", "已锁定车辆: " + m_dispPlate->text());
        }
        return;
    }

    QMessageBox::warning(this, "提示", "请先输入车牌号等信息搜索车辆");
}

void FrontDeskPage::onClearVehicle()
{
    m_lockedVid = 0; m_foundVid = 0;
    m_sPlate->clear(); m_sVin->clear(); m_sEngine->clear();
    m_sOwner->clear(); m_sPhone->clear(); m_sModel->clear();
    clearGhost();
    m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
    m_editOrderNo->setText(generateOrderNo());
    emit orderNoChanged(m_editOrderNo->text());
    setState(STATE_SEARCH);
}

void FrontDeskPage::clearGhost()
{
    m_dispPlate->clear(); m_dispVin->clear(); m_dispEngine->clear();
    m_dispModel->clear(); m_dispOwner->clear(); m_dispPhone->clear(); m_dispAddress->clear();
    m_dispColor->setCurrentIndex(0); m_dispFuel->setCurrentIndex(0); m_dispTrans->setCurrentIndex(0);
    m_dispPurchase->setDate(QDate::currentDate());
    m_btnMaintenanceHistory->setVisible(false);
}

// ============================================================
// setState — 切换界面状态（查找 / 录入 / 派工）
// ============================================================
void FrontDeskPage::setState(FrontDeskState s)
{
    m_state = s;
    m_searchGroup->setVisible(s == STATE_SEARCH);
    m_newGroup->setVisible(s == STATE_NEW_CAR);
    m_infoGroup->setVisible(s == STATE_DISPATCH);  // 派工状态显示车辆信息
    m_dispatchGroup->setVisible(s == STATE_DISPATCH);
    m_repairGroup->setVisible(s == STATE_DISPATCH);
    m_feeGroup->setVisible(s == STATE_DISPATCH);
}

// ============================================================
// 新车录入 → 保存并锁定
// ============================================================
void FrontDeskPage::onSaveNewCar()
{
    QString np = m_nPlate->text().trimmed();
    QString ow = m_nOwner->text().trimmed();
    QString ph = m_nPhone->text().trimmed();
    QString md = m_nModel->text().trimmed();
    if (np.isEmpty() || ow.isEmpty() || ph.isEmpty() || md.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写车牌号、车主、电话、车型等必填信息");
        return;
    }

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());
    q.prepare("INSERT INTO t_vehicle (plate_number,vin,engine_number,model,purchase_date,"
              "color,fuel_type,transmission) "
              "VALUES (:p,:v,:e,:m,:pd,:col,:fuel,:trans)");
    q.bindValue(":p",np); q.bindValue(":v",m_nVin->text().trimmed().isEmpty()?QVariant(QString()):m_nVin->text().trimmed());
    q.bindValue(":e",m_nEngine->text().trimmed().isEmpty()?QVariant(QString()):m_nEngine->text().trimmed());
    q.bindValue(":m",md);
    q.bindValue(":pd",m_nPurchase->date());
    q.bindValue(":col", m_nColor->currentText().isEmpty() ? QVariant(QString()) : m_nColor->currentText());
    q.bindValue(":fuel", m_nFuel->currentText().isEmpty() ? QVariant(QString()) : m_nFuel->currentText());
    q.bindValue(":trans", m_nTrans->currentText().isEmpty() ? QVariant(QString()) : m_nTrans->currentText());
    if (!DbManager::instance().executeQuery(q)) { DbManager::instance().rollbackTransaction(); QMessageBox::warning(this,"保存失败",q.lastError().text()); return; }
    m_lockedVid = q.lastInsertId().toInt();
    q.prepare("INSERT INTO t_customer (vehicle_id,name,phone,address,type) VALUES (:vid,:n,:p,:addr,'车主')");
    q.bindValue(":vid",m_lockedVid); q.bindValue(":n",ow); q.bindValue(":p",ph);
    q.bindValue(":addr", m_nAddress->text().trimmed().isEmpty() ? QVariant(QString()) : m_nAddress->text().trimmed());
    DbManager::instance().executeQuery(q);
    DbManager::instance().commitTransaction();

    fillVehicleData(m_lockedVid);
    m_sPlate->setText(np);
    m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
    m_btnMaintenanceHistory->setVisible(true);
    m_editOrderNo->setText(generateOrderNo());
    emit orderNoChanged(m_editOrderNo->text());
    setState(STATE_DISPATCH);
    QMessageBox::information(this,"成功","新车已保存并锁定 "+np);
}

// ============================================================
// 新车录入 → 取消，返回查找
// ============================================================
void FrontDeskPage::onCancelNewCar()
{
    m_nPlate->clear(); m_nVin->clear(); m_nEngine->clear();
    m_nModel->clear(); m_nOwner->clear(); m_nPhone->clear(); m_nAddress->clear();
    m_nColor->setCurrentIndex(0); m_nFuel->setCurrentIndex(0); m_nTrans->setCurrentIndex(0);
    m_nPurchase->setDate(QDate::currentDate());
    setState(STATE_SEARCH);
}

// ============================================================
// 派工 → 取消，返回查找
// ============================================================
void FrontDeskPage::onCancelDispatch()
{
    onClearVehicle();
}

// ============================================================
// 保存车辆信息修改
// ============================================================
void FrontDeskPage::onSaveVehicleInfo()
{
    if (m_lockedVid == 0) return;

    if (QMessageBox::question(this, "确认保存",
            "确认保存对车辆信息的修改？",
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());

    // 更新 t_vehicle
    q.prepare("UPDATE t_vehicle SET plate_number=:p, vin=:v, engine_number=:e, "
              "model=:m, color=:col, fuel_type=:fuel, transmission=:trans, "
              "purchase_date=:pd WHERE id=:id");
    q.bindValue(":p", m_dispPlate->text().trimmed());
    q.bindValue(":v", m_dispVin->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispVin->text().trimmed());
    q.bindValue(":e", m_dispEngine->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispEngine->text().trimmed());
    q.bindValue(":m", m_dispModel->text().trimmed());
    q.bindValue(":col", m_dispColor->currentText().isEmpty() ? QVariant(QString()) : m_dispColor->currentText());
    q.bindValue(":fuel", m_dispFuel->currentText().isEmpty() ? QVariant(QString()) : m_dispFuel->currentText());
    q.bindValue(":trans", m_dispTrans->currentText().isEmpty() ? QVariant(QString()) : m_dispTrans->currentText());
    q.bindValue(":pd", m_dispPurchase->date());
    q.bindValue(":id", m_lockedVid);
    if (!DbManager::instance().executeQuery(q)) {
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this, "保存失败", q.lastError().text());
        return;
    }

    // 更新 t_customer（车主信息）
    q.prepare("UPDATE t_customer SET name=:n, phone=:ph, address=:addr "
              "WHERE vehicle_id=:vid AND type='车主'");
    q.bindValue(":n", m_dispOwner->text().trimmed());
    q.bindValue(":ph", m_dispPhone->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispPhone->text().trimmed());
    q.bindValue(":addr", m_dispAddress->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispAddress->text().trimmed());
    q.bindValue(":vid", m_lockedVid);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();
    QMessageBox::information(this, "保存成功", "车辆信息已更新");
}

// ============================================================
// 创建工单
// ============================================================
void FrontDeskPage::onCreateWorkOrder()
{
    if (m_lockedVid == 0) { QMessageBox::warning(this,"提示","请先锁定车辆"); return; }
    if (m_cmbMainTech->currentData().toInt() == 0) { QMessageBox::warning(this,"提示","请选主修人"); return; }

    // ========== 1. 公里数验证：不能小于数据库中之前保存的公里数 ==========
    {
        QSqlQuery q(DbManager::instance().database());
        q.prepare("SELECT current_mileage FROM t_vehicle WHERE id=:vid");
        q.bindValue(":vid", m_lockedVid);
        if (DbManager::instance().executeQuery(q) && q.next()) {
            int dbMileage = q.value(0).toInt();
            if (m_spinMileage->value() < dbMileage) {
                QMessageBox::warning(this, "公里数异常",
                    QString("当前输入公里数(%1 km)小于车辆档案中记录的公里数(%2 km)，请核实后重新输入。")
                    .arg(m_spinMileage->value()).arg(dbMileage));
                return;
            }
        }
    }

    // ========== 2. 报修内容验证：至少填入一个条目 ==========
    bool hasItem = false;
    for (auto &row : m_allRows) {
        if (row.tech->currentData().toInt() > 0
            || !row.content->text().trimmed().isEmpty()
            || row.fee->value() > 0)
        { hasItem = true; break; }
    }
    if (!hasItem) { QMessageBox::warning(this,"提示","请至少填写一个报修内容条目（机电/钣金/喷漆）"); return; }

    QString orderNo = m_editOrderNo->text();
    if (orderNo.isEmpty()) { orderNo = generateOrderNo(); m_editOrderNo->setText(orderNo); }

    double labor = calcRepairFee();
    double mat = m_spinMat->value(), oth = m_spinOther->value(), mgmt = m_spinMgmt->value();
    double total = labor + mat + oth + mgmt;

    // ---- 调试: 打印即将插入的工单关键字段 ----
    qDebug() << "========== [onCreateWorkOrder] 准备插入工单 ==========";
    qDebug() << "  工单号:" << orderNo;
    qDebug() << "  车辆ID:" << m_lockedVid;
    qDebug() << "  主修人ID:" << m_cmbMainTech->currentData().toInt();
    qDebug() << "  顾问ID:" << (m_cmbAdvisor->currentData().toInt() > 0 ? m_cmbAdvisor->currentData().toInt() : 0);
    qDebug() << "  公里数:" << m_spinMileage->value();
    qDebug() << "  报修日期:" << m_dateRepair->date().toString("yyyy-MM-dd");
    qDebug() << "  预估完工:" << m_dateEstimated->date().toString("yyyy-MM-dd");
    qDebug() << "  班别:" << m_cmbShift->currentText();
    qDebug() << "  主修人姓名:" << m_cmbMainTech->currentText();
    qDebug() << "  报修内容:" << m_textContent->toPlainText();
    qDebug() << "  工时费:" << labor << " 材料费:" << mat << " 其它费:" << oth
             << " 管理费:" << mgmt << " 订金:" << m_spinDep->value();
    qDebug() << "  总金额:" << total;
    qDebug() << "  状态: 已派工";
    qDebug() << "  创建人ID:" << Session::instance().userId();
    qDebug() << "==========================================================";

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());
    q.prepare("INSERT INTO t_workorder (order_no,vehicle_id,technician_id,customer_service_id,mileage,"
              "repair_content,repair_date,estimated_date,shift,main_technician,"
              "material_fee,other_fee,management_fee,labor_fee,total_amount,deposit,status,created_by) "
              "VALUES (:no,:vid,:tid,:csi,:mile,:cont,:rd,:ed,:sh,:mtech,"
              ":mf,:of,:mgf,:lf,:total,:dep,'已派工',:creator)");
    q.bindValue(":no",orderNo); q.bindValue(":vid",m_lockedVid);
    q.bindValue(":tid",m_cmbMainTech->currentData().toInt());
    q.bindValue(":csi",m_cmbAdvisor->currentData().toInt()>0?m_cmbAdvisor->currentData().toInt():QVariant(QMetaType::fromType<int>()));
    q.bindValue(":mile",m_spinMileage->value());
    q.bindValue(":cont",m_textContent->toPlainText());
    q.bindValue(":rd",m_dateRepair->date()); q.bindValue(":ed",m_dateEstimated->date());
    q.bindValue(":sh",m_cmbShift->currentText()); q.bindValue(":mtech",m_cmbMainTech->currentText());
    q.bindValue(":mf",mat); q.bindValue(":of",oth); q.bindValue(":mgf",mgmt);
    q.bindValue(":lf",labor); q.bindValue(":total",total); q.bindValue(":dep",m_spinDep->value());
    q.bindValue(":creator",Session::instance().userId());
    if (!DbManager::instance().executeQuery(q)) {
        qWarning() << "[onCreateWorkOrder] 工单插入失败! 错误:" << q.lastError().text();
        qWarning() << "[onCreateWorkOrder] 执行的SQL:" << q.lastQuery();
        DbManager::instance().rollbackTransaction();
        QMessageBox::warning(this,"失败",q.lastError().text());
        return;
    }
    qDebug() << "[onCreateWorkOrder] 工单主表插入成功, lastInsertId:" << q.lastInsertId().toInt();

    int woid = q.lastInsertId().toInt();
    for (auto &row : m_allRows) {
        int techId = row.tech->currentData().toInt();
        QString cont = row.content->text().trimmed();
        double fee = row.fee->value();
        if (techId == 0 && cont.isEmpty() && fee == 0) continue;
        q.prepare("INSERT INTO t_workorder_repair_item (workorder_id,item_type,repair_person,repair_content,fee) "
                  "VALUES (:woid,:type,:person,:cont,:fee)");
        q.bindValue(":woid",woid); q.bindValue(":type",row.type);
        q.bindValue(":person",row.tech->currentText()); q.bindValue(":cont",cont); q.bindValue(":fee",fee);
        DbManager::instance().executeQuery(q);
        if (techId > 0) {
            q.prepare("INSERT INTO t_technician_work_record (workorder_id,technician_id,item_type,work_content,fee) "
                      "VALUES (:woid,:tid,:type,:cont,:fee)");
            q.bindValue(":woid",woid); q.bindValue(":tid",techId);
            q.bindValue(":type",row.type); q.bindValue(":cont",cont); q.bindValue(":fee",fee);
            DbManager::instance().executeQuery(q);
        }
    }
    q.prepare("INSERT INTO t_vehicle_transaction (vehicle_id,workorder_id,transaction_type,description,operator_id) "
              "VALUES (:vid,:woid,'进厂维修',:desc,:op)");
    q.bindValue(":vid",m_lockedVid); q.bindValue(":woid",woid);
    q.bindValue(":desc",QString("创建工单 %1").arg(orderNo));
    q.bindValue(":op",Session::instance().userId());
    DbManager::instance().executeQuery(q);

    // 更新车辆档案中的公里数
    q.prepare("UPDATE t_vehicle SET current_mileage=:mile WHERE id=:vid");
    q.bindValue(":mile", m_spinMileage->value());
    q.bindValue(":vid", m_lockedVid);
    DbManager::instance().executeQuery(q);

    DbManager::instance().commitTransaction();

    QMessageBox::information(this,"派工成功",QString("工单 %1 已创建\n总费用: ¥%2").arg(orderNo).arg(total,0,'f',2));
    emit workOrderCreated(woid, orderNo);
    onClearVehicle();  // 内含 setState(STATE_SEARCH)
}

// ============================================================
// 打印工单
// ============================================================
void FrontDeskPage::onPrintWorkOrder()
{
    QString no = m_editOrderNo->text();
    if (no.isEmpty()) { QMessageBox::warning(this,"提示","请先创建工单"); return; }
    QPrinter printer; QPrintPreviewDialog pp(&printer, this);
    connect(&pp, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString rows;
        for (auto &row : m_allRows) {
            QString tech = row.tech->currentText(), cont = row.content->text().trimmed();
            double fee = row.fee->value();
            if (tech.isEmpty() && cont.isEmpty() && fee==0) continue;
            rows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>¥%4</td></tr>")
                    .arg(row.type, tech, cont).arg(fee, 0, 'f', 2);
        }
        double total = calcTotalFee();
        QString html = QString(
            "<div style='text-align:center;'><h2>维修工单</h2><hr></div>"
            "<p><b>工单号：</b>%1</p><p><b>车牌号：</b>%2</p><p><b>主修人：</b>%3</p>"
            "<p><b>公里数：</b>%4 km</p><p><b>报修日期：</b>%5</p>"
            "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
            "<tr style='background:#34495e;color:white;'><th>类别</th><th>维修人</th><th>内容</th><th>费用</th></tr>%6</table>"
            "<p style='font-weight:bold;text-align:right;'>合计：¥%7</p>"
            "<hr><p style='color:#7f8c8d;font-size:12px;'>打印时间：%8</p>"
        ).arg(no, m_dispPlate->text(), m_cmbMainTech->currentText())
         .arg(m_spinMileage->value()).arg(m_dateRepair->date().toString("yyyy-MM-dd"))
         .arg(rows).arg(total,0,'f',2)
         .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
        doc.setHtml(html); doc.print(p);
    });
    pp.exec();
}

// ============================================================
// buildQuoteHtml — 构建报价单HTML内容（供打印和PDF导出复用）
// ============================================================
QString FrontDeskPage::buildQuoteHtml()
{
    // ============ 1. 维修项目明细（机电/钣金/喷漆） ============
    QString repairRows;
    double laborTotal = 0;
    bool hasItems = false;
    for (auto &row : m_allRows) {
        QString tech = row.tech->currentText(), cont = row.content->text().trimmed();
        double fee = row.fee->value();
        if (tech.isEmpty() && cont.isEmpty() && fee == 0) continue;
        hasItems = true;
        laborTotal += fee;
        repairRows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>¥%4</td></tr>")
                .arg(row.type, tech.isEmpty() ? "-" : tech,
                     cont.isEmpty() ? "-" : cont)
                .arg(fee, 0, 'f', 2);
    }

    // ============ 2. 费用数据 ============
    double mat  = m_spinMat->value();
    double oth  = m_spinOther->value();
    double mgmt = m_spinMgmt->value();
    double dep  = m_spinDep->value();
    double total = laborTotal + mat + oth + mgmt;

    // 格式化数字
    auto Y = [](double v) { return QString("¥%1").arg(v, 0, 'f', 2); };
    QString sMileage = QString::number(m_spinMileage->value());
    QString sNow = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

    // 格式化空值
    auto F = [](const QString &s) { return s.isEmpty() ? "-" : s; };

    // ============ 3. 取车辆完整信息（补充一次查询以获取所有字段） ============
    QString sBrand, sColor, sFuel, sTrans, sPurchase, sAddress, sOwner2, sPhone2;
    if (m_lockedVid > 0) {
        QSqlQuery q(DbManager::instance().database());
        q.prepare("SELECT v.plate_number, v.vin, v.engine_number, v.model, "
                  "v.color, v.fuel_type, v.transmission, v.purchase_date, "
                  "c.name, c.phone, c.address "
                  "FROM t_vehicle v LEFT JOIN t_customer c ON c.vehicle_id=v.id AND c.type='车主' "
                  "WHERE v.id=:id");
        q.bindValue(":id", m_lockedVid);
        DbManager::instance().executeQuery(q);
        if (q.next()) {
            sBrand    = q.value(3).toString();
            sColor    = q.value(4).toString();
            sFuel     = q.value(5).toString();
            sTrans    = q.value(6).toString();
            sPurchase = q.value(7).toDate().toString("yyyy-MM-dd");
            sOwner2   = q.value(8).toString();
            sPhone2   = q.value(9).toString();
            sAddress  = q.value(10).toString();
        }
    }

    // ============ 4. 构建HTML ============
    return QString(
        "<html><head><meta charset='utf-8'><style>"
        "body{font-family:'Microsoft YaHei','SimHei',sans-serif;font-size:11pt;margin:0;padding:0;color:#222;}"
        "h2{font-size:18pt;margin:6pt 0;text-align:center;color:#1a1a1a;}"
        ".subtitle{text-align:center;font-size:9pt;color:#666;margin-bottom:8pt;}"
        ".sect-title{font-size:12pt;font-weight:bold;color:#2c3e50;background:#ecf0f1;"
        " padding:4pt 8pt;margin:10pt 0 4pt 0;border-left:4pt solid #2980b9;}"
        "table.info{width:100%;border-collapse:collapse;margin:3pt 0;}"
        "table.info td{padding:3pt 6pt;border:0.5pt solid #ddd;font-size:10pt;}"
        "table.info td.label{background:#f8f9fa;font-weight:bold;width:13%;white-space:nowrap;}"
        "table.info td.value{width:20%;}"
        "table.repair{width:100%;border-collapse:collapse;margin:3pt 0;}"
        "table.repair th{background:#34495e;color:#fff;font-size:10pt;padding:4pt 6pt;text-align:center;}"
        "table.repair td{font-size:10pt;padding:3pt 6pt;border:0.5pt solid #ddd;}"
        "table.repair tr:nth-child(even){background:#f9f9f9;}"
        "table.summary{width:55%;border-collapse:collapse;margin:6pt 0 6pt auto;}"
        "table.summary td{padding:3pt 8pt;font-size:10pt;}"
        "table.summary td.sum-label{text-align:right;font-weight:bold;white-space:nowrap;}"
        "table.summary td.sum-value{text-align:right;white-space:nowrap;width:100pt;}"
        "table.summary tr.total td{font-size:13pt;font-weight:bold;color:#c0392b;border-top:double 2pt #c0392b;}"
        ".footer{font-size:8pt;color:#888;text-align:center;margin-top:10pt;}"
        "hr{ border:none;border-top:1pt solid #bdc3c7;margin:6pt 0;}"
        "</style></head><body>"

        // ---- 标题 ----
        "<h2>维修报价单</h2>"
        "<p class='subtitle'>工单号：%1 &nbsp;|&nbsp; 制单时间：%18</p>"
        "<hr>"

        // ---- 第一部分：车辆基本信息 ----
        "<div class='sect-title'>一、车辆基本信息</div>"
        "<table class='info'>"
        "<tr>"
        "<td class='label'>车牌号</td><td class='value'>%2</td>"
        "<td class='label'>品牌 / 车型</td><td class='value'>%3</td>"
        "<td class='label'>颜  色</td><td class='value'>%15</td>"
        "</tr><tr>"
        "<td class='label'>车架号(VIN)</td><td class='value' colspan='3'>%4</td>"
        "<td class='label'>燃油类型</td><td class='value'>%16</td>"
        "</tr><tr>"
        "<td class='label'>发动机号</td><td class='value' colspan='3'>%5</td>"
        "<td class='label'>变速箱</td><td class='value'>%17</td>"
        "</tr><tr>"
        "<td class='label'>当前里程</td><td class='value'>%6 km</td>"
        "<td class='label'>购车日期</td><td class='value'>%7</td>"
        "<td class='label'></td><td class='value'></td>"
        "</tr>"
        "</table>"

        // ---- 第二部分：车主信息 ----
        "<div class='sect-title'>二、车主信息</div>"
        "<table class='info'>"
        "<tr>"
        "<td class='label'>姓  名</td><td class='value'>%8</td>"
        "<td class='label'>联系电话</td><td class='value'>%9</td>"
        "<td class='label'></td><td class='value'></td>"
        "</tr><tr>"
        "<td class='label'>地  址</td><td class='value' colspan='5'>%19</td>"
        "</tr>"
        "</table>"

        // ---- 第三部分：工单信息 ----
        "<div class='sect-title'>三、工单信息</div>"
        "<table class='info'>"
        "<tr>"
        "<td class='label'>服务顾问</td><td class='value'>%10</td>"
        "<td class='label'>主 修 人</td><td class='value'>%11</td>"
        "<td class='label'>班  别</td><td class='value'>%12</td>"
        "</tr><tr>"
        "<td class='label'>报修日期</td><td class='value'>%13</td>"
        "<td class='label'>预估完工</td><td class='value'>%14</td>"
        "<td class='label'>公里数</td><td class='value'>%6 km</td>"
        "</tr>"
        "</table>"

        // 报修内容（如有）
        "%20"

        // ---- 第四部分：维修项目明细 ----
        "<div class='sect-title'>四、维修项目明细</div>"
        "<table class='repair'>"
        "<tr><th style='width:12%;'>类别</th><th style='width:18%;'>维修人</th>"
        "<th style='width:48%;'>维修内容</th><th style='width:22%;'>费用</th></tr>"
        "%21"
        "</table>"

        // ---- 第五部分：费用汇总 ----
        "<div class='sect-title'>五、费用汇总</div>"
        "<table class='summary'>"
        "<tr><td class='sum-label'>工时费合计：</td><td class='sum-value'>%22</td></tr>"
        "<tr><td class='sum-label'>材料费：</td><td class='sum-value'>%23</td></tr>"
        "<tr><td class='sum-label'>其它费：</td><td class='sum-value'>%24</td></tr>"
        "<tr><td class='sum-label'>管理费：</td><td class='sum-value'>%25</td></tr>"
        "<tr><td class='sum-label'>订  金：</td><td class='sum-value'>%26</td></tr>"
        "<tr class='total'><td class='sum-label'>合  计：</td><td class='sum-value'>%27</td></tr>"
        "</table>"

        // ---- 页脚 ----
        "<hr><p class='footer'>本报价单有效期3天，最终价格以实际结算为准。"
        "如有疑问请与本公司服务顾问联系。</p>"
        "</body></html>"
    )
    // 参数列表 %1~%27
    .arg(m_editOrderNo->text())                        // %1  工单号
    .arg(F(m_dispPlate->text()))                       // %2  车牌号
    .arg(F(sBrand.isEmpty() ? m_dispModel->text() : sBrand + " " + m_dispModel->text())) // %3  品牌车型
    .arg(F(m_dispVin->text()))                         // %4  VIN
    .arg(F(m_dispEngine->text()))                      // %5  发动机号
    .arg(sMileage)                                     // %6  公里数
    .arg(F(sPurchase.isEmpty() ? m_dispPurchase->date().toString("yyyy-MM-dd") : sPurchase)) // %7  购车日期
    .arg(F(sOwner2.isEmpty() ? m_dispOwner->text() : sOwner2))       // %8  车主
    .arg(F(sPhone2.isEmpty() ? m_dispPhone->text() : sPhone2))       // %9  电话
    .arg(F(m_cmbAdvisor->currentText()))               // %10 顾问
    .arg(F(m_cmbMainTech->currentText()))              // %11 主修人
    .arg(F(m_cmbShift->currentText()))                 // %12 班别
    .arg(m_dateRepair->date().toString("yyyy-MM-dd"))  // %13 报修日期
    .arg(m_dateEstimated->date().toString("yyyy-MM-dd")) // %14 预估完工
    .arg(F(sColor))                                    // %15 颜色
    .arg(F(sFuel))                                     // %16 燃油
    .arg(F(sTrans))                                    // %17 变速箱
    .arg(sNow)                                         // %18 制单时间
    .arg(F(sAddress.isEmpty() ? m_dispAddress->text() : sAddress))   // %19 地址
    // 报修内容（如有）
    .arg(m_textContent->toPlainText().trimmed().isEmpty() ? QString() :
         QString("<div class='sect-title' style='margin-top:4pt;'>报修描述</div>"
                 "<p style='margin:3pt 8pt;font-size:10pt;'>%1</p>")
         .arg(m_textContent->toPlainText().trimmed())) // %20
    .arg(hasItems ? repairRows : "<tr><td colspan='4' style='text-align:center;color:#999;padding:8pt;'>暂无维修项目</td></tr>") // %21
    .arg(Y(laborTotal))                                // %22 工时费
    .arg(Y(mat))                                       // %23 材料费
    .arg(Y(oth))                                       // %24 其它费
    .arg(Y(mgmt))                                      // %25 管理费
    .arg(Y(dep))                                       // %26 订金
    .arg(Y(total));                                    // %27 总计
}

// ============================================================
// 打印报价单（面向客户）
// ============================================================
void FrontDeskPage::onPrintQuote()
{
    if (m_editOrderNo->text().isEmpty() || m_lockedVid == 0) {
        QMessageBox::warning(this,"提示","请先锁定车辆并创建工单");
        return;
    }
    QPrinter printer; QPrintPreviewDialog pp(&printer, this);
    connect(&pp, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        doc.setHtml(buildQuoteHtml()); doc.print(p);
    });
    pp.exec();
}

// ============================================================
// 导出报价单PDF
// ============================================================
void FrontDeskPage::onExportQuotePdf()
{
    if (m_editOrderNo->text().isEmpty() || m_lockedVid == 0) {
        QMessageBox::warning(this, "提示", "请先锁定车辆并填写报价信息");
        return;
    }

    // 文件名中的工单号与 PDF 内容中的工单号同源（m_editOrderNo）
    QString defaultName = QString("报价单_%1_%2.pdf")
        .arg(m_editOrderNo->text(), m_dispPlate->text());
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出报价单PDF", defaultName, "PDF文件 (*.pdf)");
    if (filePath.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(buildQuoteHtml());
    // 使用 Point 单位计算页面尺寸，与 HTML/CSS 的渲染分辨率匹配
    QSizeF pageSize = printer.pageRect(QPrinter::Point).size();
    doc.setPageSize(pageSize);
    // 让内容填满整页宽度
    doc.setTextWidth(pageSize.width());
    doc.print(&printer);

    QMessageBox::information(this, "导出成功",
        QString("报价单已保存到:\n%1").arg(filePath));
}

// ============================================================
// 查看维修历史
// ============================================================
void FrontDeskPage::onShowMaintenanceHistory()
{
    if (m_lockedVid == 0) return;

    // 查询维修历史
    QSqlQuery q(DbManager::instance().database());
    q.prepare("SELECT mh.maintenance_date, mh.total_amount, mh.cumulative_amount, "
              "mh.parts_summary, mh.repair_summary, w.order_no "
              "FROM t_maintenance_history mh "
              "LEFT JOIN t_workorder w ON w.id = mh.workorder_id "
              "WHERE mh.vehicle_id = :vid "
              "ORDER BY mh.maintenance_date ASC, mh.id ASC");
    q.bindValue(":vid", m_lockedVid);
    DbManager::instance().executeQuery(q);

    // 收集数据
    struct HistRow { QString date, orderNo, parts, repairs, costStr; double cumulative; };
    QList<HistRow> rows;
    double runningTotal = 0.0;
    while (q.next()) {
        HistRow r;
        r.date    = q.value(0).toDateTime().toString("yyyy-MM-dd hh:mm");
        double cost = q.value(1).toDouble();
        runningTotal += cost;
        r.cumulative = runningTotal;
        r.costStr = QString::number(cost, 'f', 2);
        r.orderNo = q.value(5).toString();
        r.parts   = q.value(3).toString();
        r.repairs = q.value(4).toString();
        rows << r;
    }

    if (rows.isEmpty()) {
        QMessageBox::information(this, "维修历史",
            QString("车辆 %1 暂无维修历史记录。").arg(m_dispPlate->text()));
        return;
    }

    // 构建对话框
    QDialog dlg(this);
    dlg.setWindowTitle(QString("维修历史 — %1").arg(m_dispPlate->text()));
    dlg.resize(850, 500);
    QVBoxLayout *dl = new QVBoxLayout(&dlg);
    dl->setContentsMargins(10,10,10,10); dl->setSpacing(8);

    QLabel *header = new QLabel(
        QString("车辆 %1   共 %2 次维修记录   累计消费 ¥%3")
        .arg(m_dispPlate->text()).arg(rows.size())
        .arg(rows.last().cumulative, 0, 'f', 2));
    header->setStyleSheet("font-size:14px;font-weight:bold;color:#2c3e50;");
    dl->addWidget(header);

    QTableWidget *table = new QTableWidget(rows.size(), 5, &dlg);
    table->setHorizontalHeaderLabels({"日期","工单号","维修项目","使用备件","费用(累计)"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->setStyleSheet("QHeaderView::section{background-color:#34495e;color:white;padding:5px;}");

    for (int i = 0; i < rows.size(); i++) {
        const auto &r = rows[i];
        table->setItem(i, 0, new QTableWidgetItem(r.date));
        table->setItem(i, 1, new QTableWidgetItem(r.orderNo));
        QTableWidgetItem *repItem = new QTableWidgetItem(r.repairs);
        repItem->setToolTip(r.repairs);
        table->setItem(i, 2, repItem);
        QTableWidgetItem *partItem = new QTableWidgetItem(r.parts);
        partItem->setToolTip(r.parts);
        table->setItem(i, 3, partItem);
        QTableWidgetItem *costItem = new QTableWidgetItem(
            QString("¥%1 (累计¥%2)").arg(r.costStr).arg(r.cumulative, 0, 'f', 2));
        costItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(i, 4, costItem);
    }

    dl->addWidget(table, 1);

    QHBoxLayout *bb = new QHBoxLayout;
    bb->addStretch();
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setStyleSheet(
        "QPushButton{padding:6px 20px;border:none;border-radius:3px;"
        "background:#3498db;color:#fff;font-weight:bold;}"
        "QPushButton:hover{background:#2980b9;}");
    bb->addWidget(closeBtn);
    dl->addLayout(bb);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
}

// ============================================================
// 打印结算单（面向客户）
// ============================================================
void FrontDeskPage::onPrintSettlement()
{
    QMessageBox::information(this,"提示","结算单打印请到「结算管理-工单结算」界面操作");
}
