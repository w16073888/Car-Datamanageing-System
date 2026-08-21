#include "FrontDeskPage.h"
#include "database/Session.h"
#include "remote/RemoteQuery.h"
#include "remote/RemoteDb.h"
#include "remote/SqlUtil.h"

#include <QJsonArray>
#include <QJsonValue>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlError>
#include <QDialog>
#include <QListWidget>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QTextDocument>
#include <QDateTime>
#include <QDebug>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QScrollArea>
#include <QSignalBlocker>

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
    : QWidget(parent), m_state(STATE_SEARCH), m_lockedVid(0), m_foundVid(0),
      m_lastVehicleCount(0), m_mergeTargetWoid(0) { setupUI(); }
FrontDeskPage::~FrontDeskPage() {}

void FrontDeskPage::refreshData() { resetForm(); }

// ============================================================
// eventFilter — 键盘导航：回车/Tab 前进，左右方向键在文本输入框间跳跃
// ============================================================
bool FrontDeskPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        const int key = keyEvent->key();

        // 回车/小键盘回车：前进到下一个输入区域
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            // 不拦截 QComboBox 的回车（用于选择下拉项）
            if (qobject_cast<QComboBox*>(obj))
                return false;
            navNext(obj);
            return true;
        }

        // Tab：前进到下一个输入区域（含下拉框/日期控件）
        if (key == Qt::Key_Tab) {
            navNext(obj);
            return true;
        }

        // 左右方向键：光标在文本框边界时跳转到相邻文本输入框，
        // 否则保留文本框内移动光标的原生行为（非 QLineEdit 一律原生）。
        if (key == Qt::Key_Right) {
            auto *e = qobject_cast<QLineEdit*>(obj);
            if (e && e->selectedText().isEmpty() && e->cursorPosition() >= e->text().length()) {
                navNext(obj);
                return true;
            }
            return false;
        }
        if (key == Qt::Key_Left) {
            auto *e = qobject_cast<QLineEdit*>(obj);
            if (e && e->selectedText().isEmpty() && e->cursorPosition() <= 0) {
                navPrev(obj);
                return true;
            }
            return false;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ============================================================
// 键盘导航实现
// ============================================================
QList<QWidget*> FrontDeskPage::navFullChain() const
{
    QList<QWidget*> chain;
    if (m_state == STATE_SEARCH) {
        chain = { m_sPlate, m_sVin, m_sEngine, m_sOwner, m_sPhone, m_sModel };
    } else if (m_state == STATE_NEW_CAR) {
        chain = { m_nPlate, m_nVin, m_nEngine, m_nModel, m_nColor, m_nFuel, m_nTrans,
                  m_nOwner, m_nPhone, m_nAddress, m_nPurchase };
    } else if (m_state == STATE_DISPATCH) {
        // 报修行按类型顺序（机电 → 钣金 → 喷漆），内容+费用各占一个输入位；
        // 自动新增的空行按类型插在原行之后，因此"费用→下一区域"自然落到新行。
        QStringList typeOrder = { "机电", "钣金", "喷漆" };
        QList<const ItemRow*> ordered;
        for (const QString &t : typeOrder)
            for (const auto &row : m_allRows)
                if (row.type == t)
                    ordered.append(&row);
        for (const ItemRow *r : ordered) {
            chain << r->content << r->fee;
        }
        chain << m_partSearch << m_partPrice;
    }
    return chain;
}

QList<QWidget*> FrontDeskPage::navTextChain() const
{
    QList<QWidget*> chain;
    if (m_state == STATE_SEARCH) {
        chain = { m_sPlate, m_sVin, m_sEngine, m_sOwner, m_sPhone, m_sModel };
    } else if (m_state == STATE_NEW_CAR) {
        chain = { m_nPlate, m_nVin, m_nEngine, m_nOwner, m_nPhone, m_nAddress };
    } else if (m_state == STATE_DISPATCH) {
        QStringList typeOrder = { "机电", "钣金", "喷漆" };
        QList<const ItemRow*> ordered;
        for (const QString &t : typeOrder)
            for (const auto &row : m_allRows)
                if (row.type == t)
                    ordered.append(&row);
        for (const ItemRow *r : ordered)
            chain << r->content;
        chain << m_partSearch << m_partPrice;
    }
    return chain;
}

FrontDeskPage::ItemRow *FrontDeskPage::repairRowOf(QObject *obj)
{
    for (auto &row : m_allRows)
        if (row.content == obj || row.fee == obj)
            return &row;
    return nullptr;
}

void FrontDeskPage::focusInput(QWidget *w)
{
    if (!w)
        return;
    w->setFocus(Qt::OtherFocusReason);
    if (auto *e = qobject_cast<QLineEdit*>(w))
        e->selectAll();
    // 确保目标在滚动区内可见（报修内容滚动区 / 页面滚动区）
    QWidget *p = w->parentWidget();
    while (p) {
        if (auto *sa = qobject_cast<QScrollArea*>(p)) {
            sa->ensureWidgetVisible(w);
            break;
        }
        p = p->parentWidget();
    }
}

void FrontDeskPage::navNext(QObject *obj)
{
    // 派工状态：报修内容/费用 走特殊跳转逻辑
    if (m_state == STATE_DISPATCH) {
        if (ItemRow *row = repairRowOf(obj)) {
            if (obj == row->content) { navNextRepairContent(row); return; }
            if (obj == row->fee)     { navNextRepairFee(row);   return; }
        }
    }

    QWidget *cur = qobject_cast<QWidget*>(obj);
    if (!cur) { focusNextPrevChild(true); return; }
    QList<QWidget*> chain = navFullChain();
    const int idx = chain.indexOf(cur);
    if (idx >= 0) {
        focusInput(chain[(idx + 1) % chain.size()]);
        return;
    }
    focusNextPrevChild(true);   // 链外字段：保持原 tab 顺序
}

void FrontDeskPage::navPrev(QObject *obj)
{
    QWidget *cur = qobject_cast<QWidget*>(obj);
    if (!cur) { focusNextPrevChild(false); return; }
    QList<QWidget*> chain = navTextChain();
    const int idx = chain.indexOf(cur);
    if (idx >= 0) {
        focusInput(chain[(idx - 1 + chain.size()) % chain.size()]);
        return;
    }
    focusNextPrevChild(false);
}

void FrontDeskPage::navNextRepairContent(ItemRow *row)
{
    // 该行有内容（内容或费用已填）→ 先落本行费用；focus 移走时 editingFinished
    // 会自动新增同类型空行（若本行是同类型最后一行），费用回车时即可跳到新行。
    if (!row->content->text().trimmed().isEmpty() || row->fee->value() > 0) {
        focusInput(row->fee);
        return;
    }
    // 空行：跳到下一个内容输入框（下一板块），最后一行则跳到备件搜索区
    QList<QWidget*> chain = navFullChain();
    const int idx = chain.indexOf(row->content);
    if (idx >= 0) {
        for (int i = idx + 1; i < chain.size(); i++)
            if (qobject_cast<QLineEdit*>(chain[i])) { focusInput(chain[i]); return; }
    }
    focusInput(m_partSearch);
}

void FrontDeskPage::navNextRepairFee(ItemRow *row)
{
    // 下一行内容（若自动新增了空行，按类型顺序自然排在下一位）；最后一行则备件搜索区
    QList<QWidget*> chain = navFullChain();
    const int idx = chain.indexOf(row->fee);
    if (idx >= 0 && idx + 1 < chain.size()) { focusInput(chain[idx + 1]); return; }
    focusInput(m_partSearch);
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
    m_dispColor->setFixedWidth(80);
    m_dispFuel  = new QComboBox; m_dispFuel->setEditable(true);
    m_dispFuel->addItems({"","汽油","柴油","电动","混动","天然气"});
    m_dispFuel->setFixedWidth(90);
    m_dispTrans = new QComboBox; m_dispTrans->setEditable(true);
    m_dispTrans->addItems({"","自动","手动","无级变速","双离合","AMT"});
    m_dispTrans->setFixedWidth(110);
    m_dispPurchase = new QDateEdit;
    m_dispPurchase->setCalendarPopup(true);
    m_dispPurchase->setDisplayFormat("yyyy-MM-dd");
    m_dispPurchase->setStyleSheet(sEdit);
    // 固定宽度（车牌/VIN/车主/电话/颜色/油类/变速箱 加长）
    m_dispPlate->setFixedWidth(130);
    m_dispVin->setFixedWidth(210);
    m_dispEngine->setFixedWidth(150);
    m_dispModel->setFixedWidth(150);
    m_dispOwner->setFixedWidth(90);
    m_dispPhone->setFixedWidth(140);
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
    m_nModel = new QComboBox;   m_nModel->setEditable(true);
    m_nModel->lineEdit()->setPlaceholderText("*必填");
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
                    static_cast<QWidget*>(m_nModel),static_cast<QWidget*>(m_nColor),static_cast<QWidget*>(m_nFuel),
                    static_cast<QWidget*>(m_nTrans),static_cast<QWidget*>(m_nOwner),static_cast<QWidget*>(m_nPhone),
                    static_cast<QWidget*>(m_nAddress),static_cast<QWidget*>(m_nPurchase)})
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
    m_editOrderNo->setFixedWidth(200);
    m_cmbAdvisor  = new QComboBox;   m_cmbAdvisor->setFixedWidth(115);
    m_spinMileage = new QSpinBox;    m_spinMileage->setRange(0,9999999); m_spinMileage->setSuffix(" km");
    m_spinMileage->setFixedWidth(140);
    m_dateRepair = new QDateEdit;    m_dateRepair->setCalendarPopup(true);
    m_dateRepair->setDisplayFormat("yyyy-MM-dd"); m_dateRepair->setDate(QDate::currentDate());
    m_dateRepair->setFixedWidth(150);
    m_dateEstimated = new QDateEdit; m_dateEstimated->setCalendarPopup(true);
    m_dateEstimated->setDisplayFormat("yyyy-MM-dd"); m_dateEstimated->setDate(QDate::currentDate());
    m_dateEstimated->setFixedWidth(150);
    m_cmbShift = new QComboBox;      m_cmbShift->addItems({"","白班","夜班"});
    m_cmbShift->setFixedWidth(80);
    m_textContent = new QTextEdit;   m_textContent->setPlaceholderText("选填");
    m_textContent->setFixedHeight(40);  // 约两行高度
    m_textContent->setFixedWidth(400);
    m_textContent->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 安装回车导航事件过滤器
    m_editOrderNo->installEventFilter(this);
    m_spinMileage->installEventFilter(this);
    m_dateRepair->installEventFilter(this);
    m_dateEstimated->installEventFilter(this);

    // 第0行：工单号 + 顾问 + 公里数 + 报修日期 + 预估完工 + 班别
    {
        QHBoxLayout *row0 = new QHBoxLayout;
        row0->setSpacing(3);
        row0->addWidget(L("工单号:")); row0->addWidget(m_editOrderNo);
        row0->addWidget(L("顾问:"));   row0->addWidget(m_cmbAdvisor);
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

    // ==================== 5. 报修内容（左边） + 预计部件选择（右边） ====================
    m_repairGroup = new QGroupBox("报修内容");
    {
        QHBoxLayout *repairMain = new QHBoxLayout(m_repairGroup);
        repairMain->setContentsMargins(4,4,4,4);
        repairMain->setSpacing(8);

        // ---- 左侧：技工选择 + 报修内容滚动区 (~2/3) ----
        QVBoxLayout *leftSide = new QVBoxLayout;
        leftSide->setSpacing(2);

        // 三种技工主修人选择器（每类一个）
        QHBoxLayout *techHeader = new QHBoxLayout;
        techHeader->setSpacing(4);
        m_mechTech = new QComboBox; m_mechTech->setMaximumWidth(100);
        m_bodyTech = new QComboBox; m_bodyTech->setMaximumWidth(100);
        m_paintTech = new QComboBox; m_paintTech->setMaximumWidth(100);
        techHeader->addWidget(new QLabel("机电主修:")); techHeader->addWidget(m_mechTech);
        techHeader->addWidget(new QLabel("钣金主修:")); techHeader->addWidget(m_bodyTech);
        techHeader->addWidget(new QLabel("喷漆主修:")); techHeader->addWidget(m_paintTech);
        techHeader->addStretch();
        leftSide->addLayout(techHeader);

        m_repairScroll = new QScrollArea;
        m_repairScroll->setWidgetResizable(true);
        m_repairScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        m_repairScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_repairScroll->setFixedHeight(270);

        m_repairColumns = new QWidget;
        m_columnsLayout = new QHBoxLayout(m_repairColumns);
        m_columnsLayout->setContentsMargins(0,0,0,0);
        m_columnsLayout->setSpacing(8);

        m_repairScroll->setWidget(m_repairColumns);
        leftSide->addWidget(m_repairScroll);
        repairMain->addLayout(leftSide, 2);

        // ---- 右侧：预计备件选择区 (~1/3) ----
        QVBoxLayout *rightSide = new QVBoxLayout;
        rightSide->setSpacing(3);

        QLabel *partTitle = new QLabel("预计备件选择");
        partTitle->setStyleSheet("font-weight:bold;color:#2c3e50;");
        rightSide->addWidget(partTitle);

        // 备件搜索（模糊搜索+下拉）
        m_partSearch = new QLineEdit;
        m_partSearch->setPlaceholderText("搜索备件(编号/名称/供应商/型号)");
        rightSide->addWidget(m_partSearch);

        // 定价输入
        QHBoxLayout *priceRow = new QHBoxLayout;
        priceRow->setSpacing(3);
        priceRow->addWidget(new QLabel("定价:"));
        m_partPrice = new QLineEdit;
        m_partPrice->setPlaceholderText("手动输入价格");
        m_partPrice->setFixedWidth(80);
        priceRow->addWidget(m_partPrice);
        priceRow->addStretch();
        rightSide->addLayout(priceRow);

        // 备件搜索/定价纳入键盘导航（SearchCompleter 的过滤器后装、先执行，
        // 下拉展开时仍由它接管方向键/回车，这里只管收起后的跳转）
        m_partSearch->installEventFilter(this);
        m_partPrice->installEventFilter(this);

        // 已选备件列表（占据更多空间）
        m_partTable = new QTableWidget(0, 3);
        m_partTable->setHorizontalHeaderLabels({"名称", "型号", "单价"});
        m_partTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_partTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_partTable->verticalHeader()->setVisible(false);
        m_partTable->horizontalHeader()->setStretchLastSection(false);
        m_partTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_partTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_partTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_partTable->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");
        rightSide->addWidget(m_partTable, 1);

        // 确定 + 删除选中备件 同一行
        QHBoxLayout *partBtnRow = new QHBoxLayout;
        partBtnRow->setSpacing(4);
        m_btnAddPart = new QPushButton("确定");
        m_btnAddPart->setStyleSheet(S_BTN2H);
        m_btnAddPart->setMinimumHeight(26);
        partBtnRow->addWidget(m_btnAddPart);

        QPushButton *btnRemovePart = new QPushButton("删除选中备件");
        btnRemovePart->setStyleSheet(S_BTNGH);
        btnRemovePart->setMinimumHeight(26);
        connect(btnRemovePart, &QPushButton::clicked, [this]() {
            int row = m_partTable->currentRow();
            if (row < 0) return;
            m_partTable->removeRow(row);
            m_selectedParts.removeAt(row);
            onFeeChanged();
        });
        partBtnRow->addWidget(btnRemovePart);
        rightSide->addLayout(partBtnRow);

        repairMain->addLayout(rightSide, 1);
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
    m_spinOther = new QDoubleSpinBox; m_spinOther->setRange(0,999999.99); m_spinOther->setPrefix("¥ "); m_spinOther->setDecimals(2); m_spinOther->setMaximumWidth(110);
    m_spinMgmt  = new QDoubleSpinBox; m_spinMgmt->setRange(0,999999.99);  m_spinMgmt->setPrefix("¥ "); m_spinMgmt->setDecimals(2); m_spinMgmt->setMaximumWidth(110);

    // 安装回车导航
    m_spinOther->installEventFilter(this);
    m_spinMgmt->installEventFilter(this);

    connect(m_spinOther, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    connect(m_spinMgmt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);

    // 工时费合计
    m_lblFormulaFee = new QLabel("¥ 0.00");
    m_lblFormulaFee->setStyleSheet("font-size:14px;font-weight:bold;color:#2980b9;");
    // 材料费（自动从部件列表计算）
    m_lblMatFee = new QLabel("¥ 0.00");
    m_lblMatFee->setStyleSheet("font-size:14px;font-weight:bold;color:#2980b9;");
    // 第0行：费用输入 + 合计，同一行
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(3);
        row->addWidget(L("其它费:")); row->addWidget(m_spinOther);
        row->addWidget(L("管理费:")); row->addWidget(m_spinMgmt);
        row->addWidget(L("工时费合计:")); row->addWidget(m_lblFormulaFee);
        row->addWidget(L("材料费:")); row->addWidget(m_lblMatFee);
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

    // 部件选择区信号
    connect(m_partSearch, &QLineEdit::textChanged, this, &FrontDeskPage::onPartSearchTextChanged);
    connect(m_btnAddPart, &QPushButton::clicked, this, &FrontDeskPage::onAddPart);

    // 搜索：输入/删除即时搜索刷新下拉；失焦/回车时才走最终处理（无匹配→新车录入）
    QList<QLineEdit*> searchFields = {m_sPlate, m_sVin, m_sEngine, m_sOwner, m_sPhone, m_sModel};
    for (auto *e : searchFields) {
        connect(e, &QLineEdit::textChanged, this, [this, e]() {
            m_searchAnchorField = e;   // 记录来源，多结果下拉锚定该输入框
            onVehicleLiveSearch();
        });
        connect(e, &QLineEdit::editingFinished, this, &FrontDeskPage::onVehicleSearchFinalize);
    }

    // 多结果下拉选择器：车辆 / 备件
    m_searchAnchorField = m_sPlate;
    m_vehicleCompleter = new SearchCompleter(this);
    connect(m_vehicleCompleter, &SearchCompleter::selected, this, [this](int vid) {
        lockVehicle(vid);
    });

    m_partCompleter = new SearchCompleter(this);
    m_partCompleter->setEdit(m_partSearch);
    connect(m_partCompleter, &SearchCompleter::selected, this, [this](int idx) {
        if (idx >= 0 && idx < m_partRows.size())
            selectPart(m_partRows[idx][0], m_partRows[idx][4]); // name, priceRaw
    });

    loadCombos(); resetForm();
    setState(STATE_SEARCH);   // 初始：仅显示车辆查找
}

// ============================================================
// addRepairRow — 添加报修内容行（仅内容和费用）
// ============================================================
void FrontDeskPage::addRepairRow(const QString &type)
{
    ItemRow row;
    row.container = nullptr;
    row.type = type;
    row.content = new QLineEdit;
    row.content->setPlaceholderText("内容");
    row.content->setFixedWidth(230);
    // 放大报修内容行的高度（覆盖页面级 S_COMPACT 的 min-height:18px）
    row.content->setStyleSheet("QLineEdit{min-height:30px;padding:2px 4px;}");
    row.fee = new QDoubleSpinBox;
    row.fee->setRange(0, 999999.99);
    row.fee->setDecimals(0);
    row.fee->setFixedWidth(105);
    row.fee->setStyleSheet("QDoubleSpinBox{min-height:30px;padding:2px 2px;}");

    // 费用变化 → 刷新合计
    connect(row.fee, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FrontDeskPage::onFeeChanged);

    // 内容编辑完成 → 如果同类最后一行有内容，自动新增同类型空行（无行数上限，多列自动换列）
    QString typeCapture = type;
    connect(row.content, &QLineEdit::editingFinished, this, [this, typeCapture]() {
        // 获取该类型最后一行，如果它有内容则自动新增空行（无上限，由多列布局自动换列）
        ItemRow *lastOfType = nullptr;
        for (int i = m_allRows.size() - 1; i >= 0; i--) {
            if (m_allRows[i].type == typeCapture) {
                lastOfType = &m_allRows[i];
                break;
            }
        }
        if (!lastOfType) return;
        bool lastHasContent = !lastOfType->content->text().trimmed().isEmpty()
                           || lastOfType->fee->value() > 0;
        if (lastHasContent)
            addRepairRow(typeCapture);
    });

    // 安装回车导航事件过滤器
    row.content->installEventFilter(this);
    row.fee->installEventFilter(this);

    m_allRows.append(row);
    rebuildRepairLayout();
}

// ============================================================
// rebuildRepairLayout — 多列布局，每列最多 MAX_ROWS_PER_COLUMN 行
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

        // 列头：仅内容 + 费用
        {
            QHBoxLayout *hdr = new QHBoxLayout;
            QLabel *lblContent = new QLabel("内容");
            lblContent->setStyleSheet("font-weight:bold;");
            lblContent->setFixedWidth(230);
            QLabel *lblFee = new QLabel("费用");
            lblFee->setStyleSheet("font-weight:bold;");
            lblFee->setFixedWidth(105);
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
    RemoteQuery q;
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

    // 加载三类技工主修人选择器（所有活跃员工）
    auto loadTechCombo = [&](QComboBox *cb) {
        cb->clear(); cb->addItem("", 0);
        RemoteQuery qq;
        qq.exec("SELECT id,name FROM t_employee WHERE is_active=1 ORDER BY name");
        while (qq.next()) cb->addItem(qq.value(1).toString(), qq.value(0).toInt());
    };
    loadTechCombo(m_mechTech);
    loadTechCombo(m_bodyTech);
    loadTechCombo(m_paintTech);
}

// ============================================================
// 辅助
// ============================================================
QString FrontDeskPage::generateOrderNo()
{
    RemoteQuery q;
    // 编码规则: WO + 年月(6位) + 本月流水(3位)，如 WO202607001
    QString p = QDate::currentDate().toString("yyyyMM");
    // 前缀匹配（内部生成 WO{年月}，% 只在尾部），故意不用全模糊 likePattern，避免跨月计数
    q.prepare("SELECT COUNT(*) FROM t_workorder WHERE " + SqlUtil::likeCond("order_no", ":p"));
    q.bindValue(":p", "WO" + p + "%");
    q.exec();
    int c = 0; if (q.next()) c = q.value(0).toInt();
    return QString("WO%1%2").arg(p).arg(c+1,5,10,QChar('0'));
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
    m_mechTech->setCurrentIndex(0); m_bodyTech->setCurrentIndex(0);
    m_paintTech->setCurrentIndex(0); m_textContent->clear();
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

    // 清空部件选择区
    m_selectedParts.clear();
    m_partTable->setRowCount(0);
    m_partSearch->clear();
    m_partPrice->clear();

    m_spinOther->setValue(0); m_spinMgmt->setValue(0);
    m_lblTotal->setText("¥ 0.00");
    m_lblFormulaFee->setText("¥ 0.00");
    m_lblMatFee->setText("¥ 0.00");

    // 重置新车录入表单
    resetNewCarForm();
}

double FrontDeskPage::calcRepairFee()
{
    double t = 0;
    for (auto &row : m_allRows)
        t += row.fee->value();
    return t;
}

double FrontDeskPage::calcPartTotal() const
{
    double t = 0;
    for (auto &p : m_selectedParts)
        t += p.price;
    return t;
}

double FrontDeskPage::calcTotalFee()
{
    return calcRepairFee() + calcPartTotal() + m_spinOther->value() + m_spinMgmt->value();
}

void FrontDeskPage::onFeeChanged()
{
    m_lblFormulaFee->setText(QString("¥ %1").arg(calcRepairFee(),0,'f',2));
    m_lblMatFee->setText(QString("¥ %1").arg(calcPartTotal(),0,'f',2));
    m_lblTotal->setText(QString("¥ %1").arg(calcTotalFee(),0,'f',2));
}

// ============================================================
// 多字段模糊搜索
// ============================================================
void FrontDeskPage::onVehicleLiveSearch()
{
    // 车辆已锁定时，不响应搜索字段的自动变化，防止误清除已显示的车辆信息
    // 用户需先点击"解锁"按钮才能搜索其他车辆
    if (m_lockedVid > 0)
        return;
    m_vehicleCompleter->hideDropdown();   // 先收起上次残留，本次有匹配会在下面重新展开

    QString f[6] = {m_sPlate->text().trimmed(), m_sVin->text().trimmed(), m_sEngine->text().trimmed(),
                    m_sOwner->text().trimmed(), m_sPhone->text().trimmed(), m_sModel->text().trimmed()};
    bool any = false; for (auto &s : f) if (!s.isEmpty()) { any = true; break; }
    if (!any) {
        m_lastVehicleCount = 0;
        m_foundVid = 0; clearGhost(); m_lblStatus->setText("未锁定");
        m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
        m_vehicleCompleter->hideDropdown();
        return;
    }

    QString sql = "SELECT DISTINCT v.id, v.plate_number, v.vin, v.engine_number, v.model, "
                  "v.owner_name, v.owner_phone FROM t_vehicle v WHERE ";
    QStringList conds;
    if (!f[0].isEmpty()) conds << SqlUtil::likeCond("v.plate_number", ":p0");
    if (!f[1].isEmpty()) conds << SqlUtil::likeCond("v.vin", ":p1");
    if (!f[2].isEmpty()) conds << SqlUtil::likeCond("v.engine_number", ":p2");
    if (!f[3].isEmpty()) conds << SqlUtil::likeCond("v.owner_name", ":p3");
    if (!f[4].isEmpty()) conds << SqlUtil::likeCond("v.owner_phone", ":p4");
    if (!f[5].isEmpty()) conds << SqlUtil::likeCond("v.model", ":p5");

    if (conds.isEmpty()) return;
    sql += "(" + conds.join(" OR ") + ") ORDER BY v.id DESC LIMIT 30";

    RemoteQuery q;
    q.prepare(sql);
    if (!f[0].isEmpty()) q.bindValue(":p0", SqlUtil::likePattern(f[0]));
    if (!f[1].isEmpty()) q.bindValue(":p1", SqlUtil::likePattern(f[1]));
    if (!f[2].isEmpty()) q.bindValue(":p2", SqlUtil::likePattern(f[2]));
    if (!f[3].isEmpty()) q.bindValue(":p3", SqlUtil::likePattern(f[3]));
    if (!f[4].isEmpty()) q.bindValue(":p4", SqlUtil::likePattern(f[4]));
    if (!f[5].isEmpty()) q.bindValue(":p5", SqlUtil::likePattern(f[5]));

    if (!q.exec()) {
        qWarning() << "[FrontDesk] 模糊搜索失败:" << q.lastError().text();
        return;
    }

    struct VItem { int id; QString plate, engine, model, owner, phone; };
    QList<VItem> items;
    while (q.next()) {
        VItem vi;
        vi.id = q.value(0).toInt();
        vi.plate = q.value(1).toString();
        vi.engine = q.value(3).toString();
        vi.model  = q.value(4).toString();
        vi.owner  = q.value(5).toString();
        vi.phone  = q.value(6).toString();
        items << vi;
    }

    m_lastVehicleCount = items.size();

    if (items.isEmpty()) {
        m_foundVid = 0; clearGhost();
        m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
        m_vehicleCompleter->hideDropdown();
        return;
    }

    // 无论 1 条还是多条，都在触发来源输入框下方自动展开下拉供鼠标/键盘选择，
    // 仅在用户主动选择时才锁定车辆
    QList<QStringList> rows;
    QList<QVariant> ids;
    for (const auto &vi : items) {
        ids << vi.id;
        rows << QStringList{vi.plate, vi.engine, vi.model, vi.owner, vi.phone};
    }
    if (!m_searchAnchorField)
        m_searchAnchorField = m_sPlate;
    m_vehicleCompleter->setEdit(m_searchAnchorField);
    m_vehicleCompleter->setResults(rows, ids);
    m_vehicleCompleter->showDropdown();
}

// ============================================================
// onVehicleSearchFinalize — 失焦/回车收尾：仅当实时搜索无匹配且有关键字时进入新车录入
// ============================================================
void FrontDeskPage::onVehicleSearchFinalize()
{
    if (m_lockedVid > 0)
        return;
    QString f[6] = {m_sPlate->text().trimmed(), m_sVin->text().trimmed(), m_sEngine->text().trimmed(),
                    m_sOwner->text().trimmed(), m_sPhone->text().trimmed(), m_sModel->text().trimmed()};
    bool any = false; for (auto &s : f) if (!s.isEmpty()) { any = true; break; }
    if (!any)
        return;
    if (m_lastVehicleCount != 0)
        return;                       // 有匹配 → 用户应从下拉选择，不自动进新车录入

    resetNewCarForm();        // 保证每次打开新车录入时全部字段清空
    refreshCarModelList();
    setState(STATE_NEW_CAR);
}

// ============================================================
// lockVehicle — 锁定车辆（唯一匹配与下拉选择共用同一套流程）
// ============================================================
void FrontDeskPage::lockVehicle(int vid)
{
    // 锁定前检查该车辆是否有在派工中的工单（只查本车、状态=已派工）
    if (!confirmLockWithPendingOrders(vid)) {
        m_lockedVid = 0;
        m_foundVid = 0;
        clearGhost();
        m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
        setState(STATE_SEARCH);
        return;
    }
    m_lockedVid = vid;
    m_foundVid = 0;
    clearGhost();
    fillVehicleData(vid);
    m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
    m_btnMaintenanceHistory->setVisible(true);
    m_editOrderNo->setText(generateOrderNo());
    emit orderNoChanged(m_editOrderNo->text());
    setState(STATE_DISPATCH);
}

// ============================================================
// fillVehicleData
// ============================================================
void FrontDeskPage::fillVehicleData(int vid)
{
    qDebug() << "[fillVehicleData] 被调用, vid =" << vid;

    RemoteQuery q;
    q.prepare("SELECT v.plate_number,v.vin,v.engine_number,v.model,"
              "v.color,v.fuel_type,v.transmission,v.current_mileage,"
              "v.purchase_date,v.owner_name,v.owner_phone,v.owner_address "
              "FROM t_vehicle v "
              "WHERE v.id=:id");
    q.bindValue(":id", vid);
    bool ok = q.exec();
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
        onVehicleLiveSearch();
        // 不再自动锁定：无论 1 条还是多条都由用户从下拉中选择后锁定
        return;
    }

    QMessageBox::warning(this, "提示", "请先输入车牌号等信息搜索车辆");
}

void FrontDeskPage::onClearVehicle()
{
    m_lockedVid = 0; m_foundVid = 0; m_lastVehicleCount = 0;
    m_vehicleCompleter->hideDropdown();
    m_mergeTargetWoid = 0; m_mergeTargetOrderNo.clear();
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
// 锁定前检查：该车辆是否存在在派工中的工单（只查本车、状态=已派工）
// 存在则弹窗让使用者选择"叠加到已有工单 / 创建新工单 / 取消"，并把选择
// 记录到 m_mergeTargetWoid，供保存并派工时直接执行（不再二次询问）。
// 返回 true=继续锁定，false=取消锁定
// ============================================================
bool FrontDeskPage::confirmLockWithPendingOrders(int vehicleId)
{
    RemoteQuery q;
    q.prepare("SELECT id, order_no FROM t_workorder "
              "WHERE vehicle_id=:vid AND status='已派工' "
              "ORDER BY created_at DESC");
    q.bindValue(":vid", vehicleId);
    if (!q.exec()) {
        // 查询失败时不阻断锁定，也不叠加
        m_mergeTargetWoid = 0;
        m_mergeTargetOrderNo.clear();
        return true;
    }

    QList<QPair<int,QString>> pending;   // <workorder_id, order_no>
    while (q.next())
        pending << qMakePair(q.value(0).toInt(), q.value(1).toString());

    if (pending.isEmpty()) {
        m_mergeTargetWoid = 0;
        m_mergeTargetOrderNo.clear();
        return true;
    }

    // 有多条派工中工单时取最新的一条（已按 created_at DESC 排序）
    const int    latestWoid    = pending.first().first;
    const QString latestOrderNo = pending.first().second;

    QString msg;
    if (pending.size() == 1)
        msg = QString("该车辆已有在派工中的工单 %1。\n"
                      "是否将本次维修项目叠加到该工单？\n\n"
                      "选择「叠加到已有工单」：保存并派工时不另建工单，直接并入该工单；\n"
                      "选择「创建新工单」：另建一份新工单。")
                  .arg(latestOrderNo);
    else
        msg = QString("该车辆存在 %1 个在派工中的工单，最新为 %2。\n"
                      "是否将本次维修项目叠加到最新工单？\n\n"
                      "选择「叠加到已有工单」：保存并派工时不另建工单，直接并入该工单；\n"
                      "选择「创建新工单」：另建一份新工单。")
                  .arg(pending.size()).arg(latestOrderNo);

    QMessageBox box(this);
    box.setWindowTitle("派工确认");
    box.setIcon(QMessageBox::Question);
    box.setText(msg);
    QPushButton *btnMerge  = box.addButton("叠加到已有工单", QMessageBox::DestructiveRole);
    QPushButton *btnNew    = box.addButton("创建新工单", QMessageBox::AcceptRole);
    QPushButton *btnCancel = box.addButton("取消", QMessageBox::RejectRole);
    box.setDefaultButton(btnNew);
    box.exec();

    if (box.clickedButton() == btnMerge) {
        m_mergeTargetWoid    = latestWoid;
        m_mergeTargetOrderNo = latestOrderNo;
        return true;
    }
    if (box.clickedButton() == btnCancel) {
        m_mergeTargetWoid = 0;
        m_mergeTargetOrderNo.clear();
        return false;
    }
    // 「创建新工单」
    m_mergeTargetWoid = 0;
    m_mergeTargetOrderNo.clear();
    return true;
}

// ============================================================
// 刷新新车录入的车型下拉列表（去重加载历史登记中出现过的车型）
// ============================================================
void FrontDeskPage::refreshCarModelList()
{
    m_nModel->clear();
    RemoteQuery q;
    q.prepare("SELECT DISTINCT model FROM t_vehicle "
              "WHERE model IS NOT NULL AND TRIM(model) <> '' "
              "ORDER BY model");
    if (q.exec()) {
        while (q.next()) {
            const QString m = q.value(0).toString().trimmed();
            if (!m.isEmpty())
                m_nModel->addItem(m);
        }
    }
    m_nModel->clearEditText();
    m_nModel->setCurrentIndex(-1);
}

// ============================================================
// 重置新车录入表单（每次打开新车录入时调用，保证全部字段清空）
// ============================================================
void FrontDeskPage::resetNewCarForm()
{
    m_nPlate->clear(); m_nVin->clear(); m_nEngine->clear();
    m_nModel->clearEditText(); m_nModel->setCurrentIndex(-1);
    m_nOwner->clear(); m_nPhone->clear(); m_nAddress->clear();
    m_nColor->setCurrentIndex(0); m_nFuel->setCurrentIndex(0); m_nTrans->setCurrentIndex(0);
    m_nPurchase->setDate(QDate::currentDate());
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

    // 每次进入"工单信息录入"（派工）状态前，清空报修内容，
    // 避免锁定下一辆车时残留上一辆的报修条目。
    if (s == STATE_DISPATCH) {
        m_textContent->clear();
        for (auto &row : m_allRows) {
            if (row.container)
                delete row.container;
        }
        m_allRows.clear();
        addRepairRow("机电");
        addRepairRow("钣金");
        addRepairRow("喷漆");
    }

    // 进入状态后，焦点自动落到该状态的默认输入框
    // （延后一个事件循环，等控件可见/布局稳定后再聚焦）
    QTimer::singleShot(0, this, [this, s]() {
        if (m_state != s)
            return;
        switch (s) {
        case STATE_SEARCH:
            m_sPlate->setFocus(Qt::OtherFocusReason);
            m_sPlate->selectAll();
            break;
        case STATE_NEW_CAR:
            m_nPlate->setFocus(Qt::OtherFocusReason);
            break;
        case STATE_DISPATCH:
            if (!m_allRows.isEmpty())
                focusInput(m_allRows.first().content);   // 报修内容第一行
            break;
        }
    });
}

// ============================================================
// 新车录入 → 保存并锁定
// ============================================================
void FrontDeskPage::onSaveNewCar()
{
    QString np = m_nPlate->text().trimmed();
    QString ow = m_nOwner->text().trimmed();
    QString ph = m_nPhone->text().trimmed();
    QString md = m_nModel->currentText().trimmed();
    if (np.isEmpty() || ow.isEmpty() || ph.isEmpty() || md.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写车牌号、车主、电话、车型等必填信息");
        return;
    }

    // 新车+车主 在一个事务内写入（经 4s-server 的 transaction 命令原子执行）
    QJsonArray steps;
    steps.append(RemoteDb::step(
        "INSERT INTO t_vehicle (plate_number,vin,engine_number,model,purchase_date,"
        "color,fuel_type,transmission) "
        "VALUES (:p,:v,:e,:m,:pd,:col,:fuel,:trans)",
        QJsonObject{
            { ":p", np },
            { ":v", RemoteDb::v(m_nVin->text().trimmed().isEmpty() ? QVariant(QString()) : m_nVin->text().trimmed()) },
            { ":e", RemoteDb::v(m_nEngine->text().trimmed().isEmpty() ? QVariant(QString()) : m_nEngine->text().trimmed()) },
            { ":m", md },
            { ":pd", RemoteDb::v(m_nPurchase->date()) },
            { ":col", RemoteDb::v(m_nColor->currentText().isEmpty() ? QVariant(QString()) : m_nColor->currentText()) },
            { ":fuel", RemoteDb::v(m_nFuel->currentText().isEmpty() ? QVariant(QString()) : m_nFuel->currentText()) },
            { ":trans", RemoteDb::v(m_nTrans->currentText().isEmpty() ? QVariant(QString()) : m_nTrans->currentText()) },
        }, "vid"));
    steps.append(RemoteDb::step(
        "UPDATE t_vehicle SET owner_name=:n, owner_phone=:p, owner_address=:addr WHERE id=:vid",
        QJsonObject{
            { ":vid", "@vid" },
            { ":n", ow },
            { ":p", ph },
            { ":addr", RemoteDb::v(m_nAddress->text().trimmed().isEmpty() ? QVariant(QString()) : m_nAddress->text().trimmed()) },
        }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "保存失败", txn.value("error").toString());
        return;
    }
    m_lockedVid = txn.value("data").toObject().value("vars").toObject().value("vid").toInt();

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
    resetNewCarForm();
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

    // 车辆+车主信息更新在一个事务内原子执行（经 4s-server）
    QJsonArray steps;
    steps.append(RemoteDb::step(
        "UPDATE t_vehicle SET plate_number=:p, vin=:v, engine_number=:e, "
        "model=:m, color=:col, fuel_type=:fuel, transmission=:trans, "
        "purchase_date=:pd WHERE id=:id",
        QJsonObject{
            { ":p", m_dispPlate->text().trimmed() },
            { ":v", RemoteDb::v(m_dispVin->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispVin->text().trimmed()) },
            { ":e", RemoteDb::v(m_dispEngine->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispEngine->text().trimmed()) },
            { ":m", m_dispModel->text().trimmed() },
            { ":col", RemoteDb::v(m_dispColor->currentText().isEmpty() ? QVariant(QString()) : m_dispColor->currentText()) },
            { ":fuel", RemoteDb::v(m_dispFuel->currentText().isEmpty() ? QVariant(QString()) : m_dispFuel->currentText()) },
            { ":trans", RemoteDb::v(m_dispTrans->currentText().isEmpty() ? QVariant(QString()) : m_dispTrans->currentText()) },
            { ":pd", RemoteDb::v(m_dispPurchase->date()) },
            { ":id", m_lockedVid },
        }));
    steps.append(RemoteDb::step(
        "UPDATE t_vehicle SET owner_name=:n, owner_phone=:ph, owner_address=:addr WHERE id=:vid",
        QJsonObject{
            { ":n", m_dispOwner->text().trimmed() },
            { ":ph", RemoteDb::v(m_dispPhone->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispPhone->text().trimmed()) },
            { ":addr", RemoteDb::v(m_dispAddress->text().trimmed().isEmpty() ? QVariant(QString()) : m_dispAddress->text().trimmed()) },
            { ":vid", m_lockedVid },
        }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        QMessageBox::warning(this, "保存失败", txn.value("error").toString());
        return;
    }
    QMessageBox::information(this, "保存成功", "车辆信息已更新");
}

// ============================================================
// 部件选择区 — 模糊搜索 + 下拉
// ============================================================
void FrontDeskPage::onPartSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        m_partRows.clear();
        m_partCompleter->hideDropdown();
        return;
    }

    RemoteQuery q;
    QString kw = text.trimmed();
    q.prepare(QString(
        "SELECT p.name, COALESCE(NULLIF(p.spec,''), '') AS spec, "
        "COALESCE(p.sale_price, 0) AS price, p.part_no, p.supplier "
        "FROM t_parts p "
        "JOIN t_part_instance i ON i.part_id = p.id "
        "WHERE i.status = '在库' "
        "AND %1 "
        "GROUP BY p.id, p.part_no, p.name, p.spec, p.sale_price, p.supplier "
        "HAVING COUNT(CASE WHEN i.status='在库' THEN 1 END) > 0 "
        "ORDER BY p.name LIMIT 15").arg(SqlUtil::likeConds(
            {"p.part_no", "p.name", "p.supplier", "p.spec"}, ":kw")));
    q.bindValue(":kw", SqlUtil::likePattern(kw));
    q.exec();

    // 收集全部匹配行（name,spec,priceDisp,supplier,priceRaw）
    m_partRows.clear();
    while (q.next()) {
        QStringList row;
        row << q.value(0).toString()  // name
            << q.value(1).toString()  // spec
            << QString("¥%1").arg(q.value(2).toDouble(), 0, 'f', 2)  // price display
            << q.value(4).toString()  // supplier
            << q.value(2).toString(); // price raw
        m_partRows << row;
    }

    if (m_partRows.isEmpty()) {
        m_partCompleter->hideDropdown();
        return;                       // 无匹配 → 收起下拉，用户输入即为部件名称
    }

    // 无论 1 条还是多条，都在输入框下方自动展开下拉，仅在用户主动选择时回填
    QList<QVariant> ids;
    for (int i = 0; i < m_partRows.size(); ++i)
        ids << i;                     // 用行号作为 id，选中后按 m_partRows 回填
    m_partCompleter->setResults(m_partRows, ids);
    m_partCompleter->showDropdown();
}

// ============================================================
// selectPart — 回填选中的备件名称与定价（唯一匹配/下拉选择共用）
// ============================================================
void FrontDeskPage::selectPart(const QString &name, const QString &priceRaw)
{
    {   // 回填名称时抑制 textChanged，避免触发二次搜索/二次下拉
        QSignalBlocker blocker(m_partSearch);
        m_partSearch->setText(name);
    }
    m_partPrice->setText(priceRaw);
}

// ============================================================
// 部件选择区 — 添加到列表
// ============================================================
void FrontDeskPage::onAddPart()
{
    QString name = m_partSearch->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入备件名称");
        return;
    }
    bool ok = false;
    double price = m_partPrice->text().trimmed().toDouble(&ok);
    if (!ok || price < 0) {
        QMessageBox::warning(this, "提示", "请输入有效的定价");
        return;
    }
    SelectedPart sp;
    sp.name = name;
    sp.spec = "";
    sp.price = price;
    m_selectedParts.append(sp);
    refreshPartList();
    onFeeChanged();
    m_partSearch->clear();
    m_partPrice->clear();
    m_partSearch->setFocus();
}

// ============================================================
// 刷新已选部件列表
// ============================================================
void FrontDeskPage::refreshPartList()
{
    m_partTable->setRowCount(0);
    m_partTable->setRowCount(m_selectedParts.size());
    for (int i = 0; i < m_selectedParts.size(); i++) {
        m_partTable->setItem(i, 0, new QTableWidgetItem(m_selectedParts[i].name));
        m_partTable->setItem(i, 1, new QTableWidgetItem(m_selectedParts[i].spec));
        m_partTable->setItem(i, 2, new QTableWidgetItem(
            QString("¥%1").arg(m_selectedParts[i].price, 0, 'f', 2)));
    }
}

// ============================================================
// 创建工单
// ============================================================
void FrontDeskPage::onCreateWorkOrder()
{
    if (m_lockedVid == 0) { QMessageBox::warning(this,"提示","请先锁定车辆"); return; }

    // ========== 1. 公里数验证：不能小于数据库中之前保存的公里数 ==========
    {
        RemoteQuery q;
        q.prepare("SELECT current_mileage FROM t_vehicle WHERE id=:vid");
        q.bindValue(":vid", m_lockedVid);
        if (q.exec() && q.next()) {
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
        if (!row.content->text().trimmed().isEmpty()
            || row.fee->value() > 0)
        { hasItem = true; break; }
    }
    if (!hasItem) { QMessageBox::warning(this,"提示","请至少填写一个报修内容条目（机电/钣金/喷漆）"); return; }

    QString orderNo = m_editOrderNo->text();
    if (orderNo.isEmpty()) { orderNo = generateOrderNo(); m_editOrderNo->setText(orderNo); }

    double labor = calcRepairFee();
    double mat = calcPartTotal(), oth = m_spinOther->value(), mgmt = m_spinMgmt->value();
    double quoteTotal = labor + mat + oth + mgmt;           // 报价单总价（含预计材料，仅用于打印报价单）
    double orderTotal = labor + oth + mgmt;                 // 工单金额（不含预计材料）

    // 读取三类技工ID
    int mechTechId = m_mechTech->currentData().toInt();
    int bodyTechId = m_bodyTech->currentData().toInt();
    int paintTechId = m_paintTech->currentData().toInt();
    // 兼容字段：取第一个非空技工
    int compatTechId = mechTechId ? mechTechId : (bodyTechId ? bodyTechId : paintTechId);
    QString compatTechName;
    if (mechTechId) compatTechName = m_mechTech->currentText();
    else if (bodyTechId) compatTechName = m_bodyTech->currentText();
    else if (paintTechId) compatTechName = m_paintTech->currentText();

    // ---- 调试: 打印即将插入的工单关键字段 ----
    qDebug() << "========== [onCreateWorkOrder] 准备插入工单 ==========";
    qDebug() << "  工单号:" << orderNo;
    qDebug() << "  车辆ID:" << m_lockedVid;
    qDebug() << "  机电主修人ID:" << mechTechId << " 姓名:" << m_mechTech->currentText();
    qDebug() << "  钣金主修人ID:" << bodyTechId << " 姓名:" << m_bodyTech->currentText();
    qDebug() << "  喷漆主修人ID:" << paintTechId << " 姓名:" << m_paintTech->currentText();
    qDebug() << "  兼容主修人ID:" << compatTechId << " 姓名:" << compatTechName;
    qDebug() << "  顾问ID:" << (m_cmbAdvisor->currentData().toInt() > 0 ? m_cmbAdvisor->currentData().toInt() : 0);
    qDebug() << "  公里数:" << m_spinMileage->value();
    qDebug() << "  报修日期:" << m_dateRepair->date().toString("yyyy-MM-dd");
    qDebug() << "  预估完工:" << m_dateEstimated->date().toString("yyyy-MM-dd");
    qDebug() << "  班别:" << m_cmbShift->currentText();
    qDebug() << "  报修内容:" << m_textContent->toPlainText();
    qDebug() << "  工时费:" << labor << " 预计材料费(仅报价):" << mat << " 其它费:" << oth
             << " 管理费:" << mgmt;
    qDebug() << "  报价单总金额:" << quoteTotal << "  工单金额(不含材料):" << orderTotal;
    qDebug() << "  状态: 已派工";
    qDebug() << "  创建人ID:" << Session::instance().userId();
    qDebug() << "==========================================================";

    // ========== 3. 按锁定车辆时的选择执行：叠加到已有工单 ==========
    // 锁定车辆时若选了「叠加到已有工单」，m_mergeTargetWoid 即为目标工单；
    // 保存并派工不再弹窗询问，直接按该选择处理。
    if (m_mergeTargetWoid > 0) {
        const int    latestWoid    = m_mergeTargetWoid;
        const QString latestOrderNo = m_mergeTargetOrderNo;

        qDebug() << "[onCreateWorkOrder] 叠加到已有工单, woid:" << latestWoid << " orderNo:" << latestOrderNo;

                // 叠加操作在一个事务内原子执行（经 4s-server）
                QJsonArray steps;
                steps.append(RemoteDb::step(
                    "UPDATE t_workorder SET "
                    "labor_fee = labor_fee + :lf, "
                    "other_fee = other_fee + :of, "
                    "management_fee = management_fee + :mgf, "
                    "total_amount = total_amount + :total "
                    "WHERE id=:woid",
                    QJsonObject{
                        { ":lf", labor }, { ":of", oth }, { ":mgf", mgmt },
                        { ":total", orderTotal },
                        { ":woid", latestWoid },
                    }));

                // 插入新的维修项目明细
                for (auto &row : m_allRows) {
                    QString cont = row.content->text().trimmed();
                    double fee = row.fee->value();
                    if (cont.isEmpty() && fee == 0) continue;
                    int techId = 0;
                    QString techName;
                    if (row.type == "机电")      { techId = mechTechId;  techName = m_mechTech->currentText(); }
                    else if (row.type == "钣金") { techId = bodyTechId;  techName = m_bodyTech->currentText(); }
                    else if (row.type == "喷漆") { techId = paintTechId; techName = m_paintTech->currentText(); }

                    steps.append(RemoteDb::step(
                        "INSERT INTO t_workorder_repair_item (workorder_id,item_type,repair_person,repair_content,fee) "
                        "VALUES (:woid,:type,:person,:cont,:fee)",
                        QJsonObject{
                            { ":woid", latestWoid }, { ":type", row.type },
                            { ":person", RemoteDb::v(techName.isEmpty() ? QString() : techName) },
                            { ":cont", cont }, { ":fee", fee },
                        }));

                    if (techId > 0) {
                        steps.append(RemoteDb::step(
                            "INSERT INTO t_technician_work_record (workorder_id,technician_id,item_type,work_content,fee) "
                            "VALUES (:woid,:tid,:type,:cont,:fee)",
                            QJsonObject{
                                { ":woid", latestWoid }, { ":tid", techId }, { ":type", row.type },
                                { ":cont", cont }, { ":fee", fee },
                            }));
                    }
                }

                // 更新维修历史：费用相加，追记维修项目
                steps.append(RemoteDb::step(
                    "UPDATE t_maintenance_history SET "
                    "labor_fee = labor_fee + :lf, "
                    "other_fee = other_fee + :of, "
                    "management_fee = management_fee + :mgf, "
                    "total_amount = total_amount + :total, "
                    "cumulative_amount = cumulative_amount + :total "
                    "WHERE workorder_id=:woid",
                    QJsonObject{
                        { ":lf", labor }, { ":of", oth }, { ":mgf", mgmt },
                        { ":total", orderTotal },
                        { ":woid", latestWoid },
                    }));

                // 记录车辆交易
                steps.append(RemoteDb::step(
                    "INSERT INTO t_vehicle_transaction (vehicle_id,workorder_id,transaction_type,description,operator_id) "
                    "VALUES (:vid,:woid,'进厂维修',:desc,:op)",
                    QJsonObject{
                        { ":vid", m_lockedVid }, { ":woid", latestWoid },
                        { ":desc", QString("叠加项目到工单 %1").arg(latestOrderNo) },
                        { ":op", Session::instance().userId() },
                    }));

                // 更新车辆公里数
                steps.append(RemoteDb::step(
                    "UPDATE t_vehicle SET current_mileage=:mile WHERE id=:vid",
                    QJsonObject{ { ":mile", m_spinMileage->value() }, { ":vid", m_lockedVid } }));

                QJsonObject txn = RemoteDb::transaction(steps);
                if (!txn.value("ok").toBool()) {
                    QMessageBox::warning(this, "失败", "叠加操作失败: " + txn.value("error").toString());
                    return;
                }

                QMessageBox::information(this, "叠加成功",
                    QString("本次项目已叠加到工单 %1\n新增工时费: ¥%2\n新增其它费: ¥%3\n新增管理费: ¥%4\n新增合计: ¥%5")
                        .arg(latestOrderNo)
                        .arg(labor, 0, 'f', 2)
                        .arg(oth, 0, 'f', 2)
                        .arg(mgmt, 0, 'f', 2)
                        .arg(orderTotal, 0, 'f', 2));
        emit workOrderCreated(latestWoid, latestOrderNo);
        onClearVehicle();
        return;
    }

    // 创建工单 + 维修历史 + 明细 + 交易记录在一个事务内原子执行（经 4s-server）
    QJsonArray steps;

    steps.append(RemoteDb::step(
        "INSERT INTO t_workorder (order_no,vehicle_id,technician_id,mechanic_tech_id,body_tech_id,paint_tech_id,"
        "customer_service_id,mileage,"
        "repair_content,repair_date,estimated_date,shift,main_technician,"
        "material_fee,other_fee,management_fee,labor_fee,total_amount,status,created_by) "
        "VALUES (:no,:vid,:tid,:mtid,:btid,:ptid,:csi,:mile,:cont,:rd,:ed,:sh,:mtech,"
        ":mf,:of,:mgf,:lf,:total,'已派工',:creator)",
        QJsonObject{
            { ":no", orderNo }, { ":vid", m_lockedVid },
            { ":tid", compatTechId > 0 ? QJsonValue(compatTechId) : QJsonValue(QJsonValue::Null) },
            { ":mtid", mechTechId > 0 ? QJsonValue(mechTechId) : QJsonValue(QJsonValue::Null) },
            { ":btid", bodyTechId > 0 ? QJsonValue(bodyTechId) : QJsonValue(QJsonValue::Null) },
            { ":ptid", paintTechId > 0 ? QJsonValue(paintTechId) : QJsonValue(QJsonValue::Null) },
            { ":csi", m_cmbAdvisor->currentData().toInt() > 0
                          ? QJsonValue(m_cmbAdvisor->currentData().toInt()) : QJsonValue(QJsonValue::Null) },
            { ":mile", m_spinMileage->value() },
            { ":cont", m_textContent->toPlainText() },
            { ":rd", RemoteDb::v(m_dateRepair->date()) },
            { ":ed", RemoteDb::v(m_dateEstimated->date()) },
            { ":sh", m_cmbShift->currentText() },
            { ":mtech", RemoteDb::v(compatTechName.isEmpty() ? QString() : compatTechName) },
            { ":mf", 0 }, { ":of", oth }, { ":mgf", mgmt },
            { ":lf", labor }, { ":total", orderTotal },
            { ":creator", Session::instance().userId() },
        }, "woid"));

    // 写入维修历史（创建时）— 客户端只计算摘要文本，写入在服务端事务内
    QStringList techNames, repairJsonParts, repairSummaryParts;
    for (auto &row : m_allRows) {
        QString cont = row.content->text().trimmed();
        double fee = row.fee->value();
        if (cont.isEmpty() && fee == 0) continue;
        QString techName;
        if (row.type == "机电")      techName = m_mechTech->currentText();
        else if (row.type == "钣金") techName = m_bodyTech->currentText();
        else if (row.type == "喷漆") techName = m_paintTech->currentText();
        if (!techName.isEmpty() && !techNames.contains(techName)) techNames << techName;
        repairJsonParts << QString("{\"type\":\"%1\",\"person\":\"%2\",\"content\":\"%3\",\"fee\":%4}")
            .arg(row.type, techName, cont, QString::number(fee, 'f', 2));
        repairSummaryParts << QString("[%1] %2 ¥%3").arg(row.type, cont, QString::number(fee, 'f', 2));
    }
    const QString rj = "[" + repairJsonParts.join(",") + "]";
    const QString rs = repairSummaryParts.join("; ");

    steps.append(RemoteDb::step(
        "INSERT INTO t_maintenance_history "
        "(vehicle_id, workorder_id, status, maintenance_date, entry_date, mileage, service_advisor, "
        "technicians, labor_fee, material_fee, other_fee, management_fee, "
        "total_amount, cumulative_amount, repair_summary, repair_items) "
        "VALUES (:vid, :woid, '已派工', :md, :entry, :mile, :svc, :tech, "
        ":labor, 0, :other, :mgmt, :total, :total, :repair, :ritems) "
        "ON DUPLICATE KEY UPDATE status='已派工', entry_date=VALUES(entry_date), "
        "mileage=VALUES(mileage), service_advisor=VALUES(service_advisor), "
        "technicians=VALUES(technicians), labor_fee=VALUES(labor_fee), "
        "other_fee=VALUES(other_fee), management_fee=VALUES(management_fee), "
        "total_amount=VALUES(total_amount), "
        "repair_summary=VALUES(repair_summary), repair_items=VALUES(repair_items)",
        QJsonObject{
            { ":vid", m_lockedVid }, { ":woid", "@woid" },
            { ":md", RemoteDb::v(m_dateRepair->date()) },
            { ":entry", RemoteDb::v(m_dateRepair->date()) }, { ":mile", m_spinMileage->value() },
            { ":svc", RemoteDb::v(m_cmbAdvisor->currentText().isEmpty() ? QString() : m_cmbAdvisor->currentText()) },
            { ":tech", RemoteDb::v(techNames.isEmpty() ? QString() : techNames.join(", ")) },
            { ":labor", labor }, { ":other", oth }, { ":mgmt", mgmt },
            { ":total", orderTotal },
            { ":repair", RemoteDb::v(rs.isEmpty() ? QString() : rs) },
            { ":ritems", RemoteDb::v(rj == "[]" ? QString() : rj) },
        }));

    for (auto &row : m_allRows) {
        QString cont = row.content->text().trimmed();
        double fee = row.fee->value();
        if (cont.isEmpty() && fee == 0) continue;
        // 根据行类型获取对应的技工
        int techId = 0;
        QString techName;
        if (row.type == "机电")      { techId = mechTechId;  techName = m_mechTech->currentText(); }
        else if (row.type == "钣金") { techId = bodyTechId;  techName = m_bodyTech->currentText(); }
        else if (row.type == "喷漆") { techId = paintTechId; techName = m_paintTech->currentText(); }

        steps.append(RemoteDb::step(
            "INSERT INTO t_workorder_repair_item (workorder_id,item_type,repair_person,repair_content,fee) "
            "VALUES (:woid,:type,:person,:cont,:fee)",
            QJsonObject{
                { ":woid", "@woid" }, { ":type", row.type },
                { ":person", RemoteDb::v(techName.isEmpty() ? QString() : techName) },
                { ":cont", cont }, { ":fee", fee },
            }));
        if (techId > 0) {
            steps.append(RemoteDb::step(
                "INSERT INTO t_technician_work_record (workorder_id,technician_id,item_type,work_content,fee) "
                "VALUES (:woid,:tid,:type,:cont,:fee)",
                QJsonObject{
                    { ":woid", "@woid" }, { ":tid", techId }, { ":type", row.type },
                    { ":cont", cont }, { ":fee", fee },
                }));
        }
    }

    // 预计材料仅用于报价单打印，不与工单绑定、不计入工单金额
    steps.append(RemoteDb::step(
        "INSERT INTO t_vehicle_transaction (vehicle_id,workorder_id,transaction_type,description,operator_id) "
        "VALUES (:vid,:woid,'进厂维修',:desc,:op)",
        QJsonObject{
            { ":vid", m_lockedVid }, { ":woid", "@woid" },
            { ":desc", QString("创建工单 %1").arg(orderNo) },
            { ":op", Session::instance().userId() },
        }));

    // 更新车辆档案中的公里数（同时记录为上次保养公里数）
    steps.append(RemoteDb::step(
        "UPDATE t_vehicle SET current_mileage=:mile, last_maintenance_mileage=:mile2 WHERE id=:vid",
        QJsonObject{
            { ":mile", m_spinMileage->value() }, { ":mile2", m_spinMileage->value() },
            { ":vid", m_lockedVid },
        }));

    QJsonObject txn = RemoteDb::transaction(steps);
    if (!txn.value("ok").toBool()) {
        qWarning() << "[onCreateWorkOrder] 工单创建事务失败:" << txn.value("error").toString();
        QMessageBox::warning(this, "失败", txn.value("error").toString());
        return;
    }
    const int woid = txn.value("data").toObject().value("vars").toObject().value("woid").toInt();
    qDebug() << "[onCreateWorkOrder] 工单创建成功, woid:" << woid;

    QMessageBox::information(this,"派工成功",QString("工单 %1 已创建\n工单金额: ¥%2\n报价金额(含预计材料): ¥%3")
        .arg(orderNo).arg(orderTotal,0,'f',2).arg(quoteTotal,0,'f',2));
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
            QString cont = row.content->text().trimmed();
            double fee = row.fee->value();
            if (cont.isEmpty() && fee==0) continue;
            QString techName;
            if (row.type == "机电")      techName = m_mechTech->currentText();
            else if (row.type == "钣金") techName = m_bodyTech->currentText();
            else if (row.type == "喷漆") techName = m_paintTech->currentText();
            rows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>¥%4</td></tr>")
                    .arg(row.type, techName, cont).arg(fee, 0, 'f', 2);
        }
        double total = calcTotalFee();
        QString techSummary = QString("机电:%1 钣金:%2 喷漆:%3")
            .arg(m_mechTech->currentText().isEmpty() ? "-" : m_mechTech->currentText(),
                 m_bodyTech->currentText().isEmpty() ? "-" : m_bodyTech->currentText(),
                 m_paintTech->currentText().isEmpty() ? "-" : m_paintTech->currentText());
        QString html = QString(
            "<div style='text-align:center;'><h2>维修工单</h2><hr></div>"
            "<p><b>工单号：</b>%1</p><p><b>车牌号：</b>%2</p><p><b>主修人：</b>%3</p>"
            "<p><b>公里数：</b>%4 km</p><p><b>报修日期：</b>%5</p>"
            "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
            "<tr style='background:#34495e;color:white;'><th>类别</th><th>维修人</th><th>内容</th><th>费用</th></tr>%6</table>"
            "<p style='font-weight:bold;text-align:right;'>合计：¥%7</p>"
            "<hr><p style='color:#7f8c8d;font-size:12px;'>打印时间：%8</p>"
        ).arg(no, m_dispPlate->text(), techSummary)
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
        QString cont = row.content->text().trimmed();
        double fee = row.fee->value();
        if (cont.isEmpty() && fee == 0) continue;
        hasItems = true;
        laborTotal += fee;
        QString techName;
        if (row.type == "机电")      techName = m_mechTech->currentText();
        else if (row.type == "钣金") techName = m_bodyTech->currentText();
        else if (row.type == "喷漆") techName = m_paintTech->currentText();
        repairRows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td align='right'>¥%4</td></tr>")
                .arg(row.type, techName.isEmpty() ? "-" : techName,
                     cont.isEmpty() ? "-" : cont)
                .arg(fee, 0, 'f', 2);
    }

    // ============ 2. 费用数据 ============
    double mat  = calcPartTotal();
    double oth  = m_spinOther->value();
    double mgmt = m_spinMgmt->value();
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
        RemoteQuery q;
        q.prepare("SELECT v.plate_number, v.vin, v.engine_number, v.model, "
                  "v.color, v.fuel_type, v.transmission, v.purchase_date, "
                  "v.owner_name, v.owner_phone, v.owner_address "
                  "FROM t_vehicle v "
                  "WHERE v.id=:id");
        q.bindValue(":id", m_lockedVid);
        q.exec();
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

        // ---- 第4.5部分：预计部件明细 ----
        "%27"

        // ---- 第五部分：费用汇总 ----
        "<div class='sect-title'>五、费用汇总</div>"
        "<table class='summary'>"
        "<tr><td class='sum-label'>工时费合计：</td><td class='sum-value'>%22</td></tr>"
        "<tr><td class='sum-label'>材料费：</td><td class='sum-value'>%23</td></tr>"
        "<tr><td class='sum-label'>其它费：</td><td class='sum-value'>%24</td></tr>"
        "<tr><td class='sum-label'>管理费：</td><td class='sum-value'>%25</td></tr>"
        "<tr class='total'><td class='sum-label'>合  计：</td><td class='sum-value'>%26</td></tr>"
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
    .arg(QString("机电:%1 钣金:%2 喷漆:%3")
         .arg(F(m_mechTech->currentText()),
              F(m_bodyTech->currentText()),
              F(m_paintTech->currentText())))          // %11 主修人
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
    .arg(Y(total))                                     // %26 合计
    .arg([&]() {                                       // %27 预计部件明细
        if (m_selectedParts.isEmpty()) return QString();
        QString partsHtml = "<div class='sect-title'>四-B、预计部件明细</div>"
                            "<table class='repair'>"
                            "<tr><th style='width:40%;'>部件名称</th><th style='width:30%;'>型号</th>"
                            "<th style='width:30%;'>单价</th></tr>";
        for (auto &sp : m_selectedParts) {
            partsHtml += QString("<tr><td>%1</td><td>%2</td><td align='right'>¥%3</td></tr>")
                .arg(sp.name, sp.spec.isEmpty() ? "-" : sp.spec)
                .arg(sp.price, 0, 'f', 2);
        }
        partsHtml += "</table>";
        return partsHtml;
    }());
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
// 维修历史数据结构
// ============================================================
struct MhOrder {
    int workorderId;
    QString orderNo, status, repairDate, completionDate, advisor, technicians;
    int mileage;
    double laborFee, materialFee, otherFee, mgmtFee, totalAmount, cumulativeAmount;
    QString partsSummary, repairItemsJson, settlementTime;
};

// ============================================================
// 查看维修历史
// ============================================================
void FrontDeskPage::onShowMaintenanceHistory()
{
    if (m_lockedVid == 0) return;

    RemoteQuery q;
    // 以 t_workorder 为主表 LEFT JOIN t_maintenance_history，显示所有状态的工单
    q.prepare("SELECT w.id AS workorder_id, w.order_no, w.status, "
              "COALESCE(mh.entry_date, w.repair_date, w.created_at) AS entry_date, "
              "mh.completion_date, "
              "COALESCE(mh.mileage, w.mileage) AS mileage, "
              "COALESCE(mh.service_advisor, '') AS service_advisor, "
              "COALESCE(mh.technicians, '') AS technicians, "
              "COALESCE(mh.total_amount, w.total_amount, 0) AS total_amount, "
              "COALESCE(mh.cumulative_amount, 0) AS cumulative_amount, "
              "COALESCE(mh.labor_fee, w.labor_fee, 0) AS labor_fee, "
              "COALESCE(mh.material_fee, w.material_fee, 0) AS material_fee, "
              "COALESCE(mh.other_fee, w.other_fee, 0) AS other_fee, "
              "COALESCE(mh.management_fee, w.management_fee, 0) AS management_fee, "
              "COALESCE(mh.parts_summary, '') AS parts_summary, "
              "COALESCE(mh.repair_items, '') AS repair_items, "
              "COALESCE(mh.maintenance_date, w.created_at) AS maintenance_date "
              "FROM t_workorder w "
              "LEFT JOIN t_maintenance_history mh ON mh.workorder_id = w.id "
              "WHERE w.vehicle_id = :vid "
              "ORDER BY w.created_at ASC, w.id ASC");
    q.bindValue(":vid", m_lockedVid);
    q.exec();

    QList<MhOrder> orders;
    double allLabor = 0, allMat = 0, allTotal = 0;
    while (q.next()) {
        MhOrder o;
        o.workorderId      = q.value(0).toInt();
        o.orderNo          = q.value(1).toString();
        o.status           = q.value(2).toString();
        o.repairDate       = q.value(3).isNull() ? "" : q.value(3).toDate().toString("yyyy-MM-dd");
        o.completionDate   = q.value(4).isNull() ? "" : q.value(4).toDate().toString("yyyy-MM-dd");
        o.mileage          = q.value(5).toInt();
        o.advisor          = q.value(6).toString();
        o.technicians      = q.value(7).toString();
        o.totalAmount      = q.value(8).toDouble();
        o.cumulativeAmount = q.value(9).toDouble();
        o.laborFee         = q.value(10).toDouble();
        o.materialFee      = q.value(11).toDouble();
        o.otherFee         = q.value(12).toDouble();
        o.mgmtFee          = q.value(13).toDouble();
        o.partsSummary     = q.value(14).toString();
        o.repairItemsJson  = q.value(15).toString();
        o.settlementTime   = q.value(16).toDateTime().toString("yyyy-MM-dd HH:mm");
        orders << o;
        allLabor += o.laborFee; allMat += o.materialFee;
        allTotal += o.totalAmount;
    }

    if (orders.isEmpty()) {
        QMessageBox::information(this, "维修历史",
            QString("车辆 %1 暂无维修历史记录。").arg(m_dispPlate->text()));
        return;
    }
    int totalCount = orders.size();

    // ==================== 对话框 ====================
    QDialog dlg(this);
    dlg.setWindowTitle(QString("维修历史 — %1").arg(m_dispPlate->text()));
    dlg.resize(1050, 620);

    QHBoxLayout *mainHL = new QHBoxLayout(&dlg);
    mainHL->setContentsMargins(8, 8, 8, 8); mainHL->setSpacing(8);

    int selectedIdx = -1;
    int displayMode = 0; // 0=工时, 1=材料

    auto getCur = [&]() -> const MhOrder* {
        if (selectedIdx < 0 || selectedIdx >= orders.size()) return nullptr;
        return &orders[selectedIdx];
    };

    // ---- 左侧面板：工单列表 ----
    QVBoxLayout *leftL = new QVBoxLayout; leftL->setSpacing(4);
    QLabel *lt = new QLabel("维修工单列表");
    lt->setStyleSheet("font-weight:bold;font-size:13px;color:#2c3e50;");
    leftL->addWidget(lt);

    QTableWidget *ot = new QTableWidget(orders.size(), 6);
    ot->setHorizontalHeaderLabels({"工号","状态","报修日期","完工日期","里程","服务顾问"});
    ot->setSelectionBehavior(QAbstractItemView::SelectRows);
    ot->setSelectionMode(QAbstractItemView::SingleSelection);
    ot->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ot->setAlternatingRowColors(true); ot->verticalHeader()->setVisible(false);
    ot->horizontalHeader()->setStretchLastSection(true);
    ot->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:4px;font-size:11px;}");
    ot->setMaximumWidth(520);

    for (int i = 0; i < orders.size(); i++) {
        const auto &o = orders[i];
        ot->setItem(i, 0, new QTableWidgetItem(o.orderNo));
        QTableWidgetItem *stItem = new QTableWidgetItem(o.status);
        if (o.status == "已结算") stItem->setForeground(QColor("#27ae60"));
        else if (o.status == "已提单") stItem->setForeground(QColor("#8e44ad"));
        else if (o.status == "待提单") stItem->setForeground(QColor("#e67e22"));
        else stItem->setForeground(QColor("#2980b9"));
        ot->setItem(i, 1, stItem);
        ot->setItem(i, 2, new QTableWidgetItem(o.repairDate));
        ot->setItem(i, 3, new QTableWidgetItem(o.completionDate));
        ot->setItem(i, 4, new QTableWidgetItem(o.mileage > 0 ? QString::number(o.mileage) : ""));
        ot->setItem(i, 5, new QTableWidgetItem(o.advisor));
    }
    leftL->addWidget(ot, 1);

    // ---- 右侧面板 ----
    QVBoxLayout *rightL = new QVBoxLayout; rightL->setSpacing(6);

    // 统计卡片
    QGroupBox *statsG = new QGroupBox("车辆总览统计");
    QHBoxLayout *statsL = new QHBoxLayout(statsG); statsL->setSpacing(8);
    auto card = [](const QString &l, const QString &v, const QString &c) {
        QWidget *w = new QWidget;
        w->setStyleSheet(QString("background:%1;border-radius:6px;padding:8px;").arg(c));
        QVBoxLayout *cl = new QVBoxLayout(w); cl->setContentsMargins(10,6,10,6);
        QLabel *vl = new QLabel(v);
        vl->setStyleSheet("font-size:18px;font-weight:bold;color:#fff;"); vl->setAlignment(Qt::AlignCenter);
        QLabel *ll = new QLabel(l); ll->setStyleSheet("font-size:11px;color:rgba(255,255,255,0.85);"); ll->setAlignment(Qt::AlignCenter);
        cl->addWidget(vl); cl->addWidget(ll); return w;
    };
    statsL->addWidget(card("总修车费", QString("¥%1").arg(allTotal, 0, 'f', 0), "#e74c3c"));
    statsL->addWidget(card("总工时费", QString("¥%1").arg(allLabor, 0, 'f', 0), "#2980b9"));
    statsL->addWidget(card("总材料费", QString("¥%1").arg(allMat, 0, 'f', 0), "#27ae60"));
    statsL->addWidget(card("总次数", QString("%1 次").arg(totalCount), "#8e44ad"));
    rightL->addWidget(statsG);

    // 当前工单详情
    QGroupBox *detG = new QGroupBox("当前工单详情");
    QVBoxLayout *detL = new QVBoxLayout(detG); detL->setSpacing(4);
    QLabel *lblFees = new QLabel("请选择左侧工单");
    lblFees->setStyleSheet("font-size:12px;padding:4px;background:#f8f9fa;border-radius:4px;");
    lblFees->setWordWrap(true); detL->addWidget(lblFees);

    QHBoxLayout *chkR = new QHBoxLayout;
    QCheckBox *cbM = new QCheckBox("机电"); cbM->setChecked(true);
    QCheckBox *cbB = new QCheckBox("钣金"); cbB->setChecked(true);
    QCheckBox *cbP = new QCheckBox("喷漆"); cbP->setChecked(true);
    chkR->addWidget(cbM); chkR->addWidget(cbB); chkR->addWidget(cbP); chkR->addStretch();
    detL->addLayout(chkR);
    rightL->addWidget(detG);

    // 维修明细
    QGroupBox *itemG = new QGroupBox("维修明细");
    QVBoxLayout *itemL = new QVBoxLayout(itemG); itemL->setSpacing(4);
    QHBoxLayout *modeR = new QHBoxLayout;
    modeR->addWidget(new QLabel("方式:"));
    QRadioButton *rb1 = new QRadioButton("工时"); rb1->setChecked(true);
    QRadioButton *rb2 = new QRadioButton("材料");
    QButtonGroup *bg = new QButtonGroup(this); bg->addButton(rb1, 0); bg->addButton(rb2, 1);
    modeR->addWidget(rb1); modeR->addWidget(rb2); modeR->addStretch();
    itemL->addLayout(modeR);

    QTableWidget *dt = new QTableWidget;
    dt->setColumnCount(4);
    dt->setHorizontalHeaderLabels({"类别","主修人/维修内容","费用",""});
    dt->setSelectionBehavior(QAbstractItemView::SelectRows);
    dt->setEditTriggers(QAbstractItemView::NoEditTriggers);
    dt->verticalHeader()->setVisible(false);
    dt->horizontalHeader()->setStretchLastSection(true);
    dt->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    dt->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    dt->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    dt->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    dt->setStyleSheet("QHeaderView::section{background:#34495e;color:#fff;padding:3px;font-size:11px;}");
    itemL->addWidget(dt, 1);
    rightL->addWidget(itemG, 1);

    // ---- 刷新右侧面板 ----
    auto refresh = [&]() {
        const MhOrder *o = getCur();
        if (!o) { lblFees->setText("请选择左侧工单"); dt->setRowCount(0); return; }
        double owe = (o->status == "已结算") ? 0.0 : o->totalAmount;
        lblFees->setText(
            QString("工时费: ¥%1 | 材料费: ¥%2 | 费用总计: ¥%3 | 欠款: ¥%4 | "
                    "工时优惠: ¥0.00 | 材料优惠: ¥0.00 | 结算时间: %5")
            .arg(o->laborFee,0,'f',2).arg(o->materialFee,0,'f',2)
            .arg(o->totalAmount,0,'f',2).arg(owe,0,'f',2).arg(o->settlementTime));

        if (displayMode == 0) {
            // ========== 工时模式 ==========
            dt->setHorizontalHeaderLabels({"类别","主修人/维修内容","费用",""});
            dt->setColumnHidden(3, true);

            QSet<QString> types;
            if (cbM->isChecked()) types.insert("机电");
            if (cbB->isChecked()) types.insert("钣金");
            if (cbP->isChecked()) types.insert("喷漆");

            QList<QStringList> items;
            // 优先从实表 t_workorder_repair_item 读取：始终完整，含叠加合并后的条目
            {
                RemoteQuery rq;
                rq.prepare("SELECT item_type, repair_person, repair_content, fee "
                           "FROM t_workorder_repair_item "
                           "WHERE workorder_id = :oid ORDER BY item_type, id");
                rq.bindValue(":oid", o->workorderId);
                rq.exec();
                while (rq.next()) {
                    QString typ = rq.value(0).toString();
                    if (!types.contains(typ)) continue;
                    QString person = rq.value(1).toString();
                    QString content = rq.value(2).toString();
                    double fee = rq.value(3).toDouble();
                    QString detail = QString("%1 — %2")
                                    .arg(person.isEmpty() ? "-" : person,
                                         content.isEmpty() ? "(无内容)" : content);
                    items << QStringList({typ, detail, QString::number(fee, 'f', 2)});
                }
            }

            // 兜底：实表无数据时退回 repair_items JSON 快照
            if (items.isEmpty()) {
                QString jsonStr = o->repairItemsJson.trimmed();
                if (!jsonStr.isEmpty()) {
                    QJsonArray arr = QJsonDocument::fromJson(jsonStr.toUtf8()).array();
                    for (const auto &v : arr) {
                        QJsonObject obj = v.toObject();
                        QString typ = obj["type"].toString();
                        if (!types.contains(typ)) continue;
                        QString person = obj["person"].toString();
                        QString content = obj["content"].toString();
                        double fee = obj["fee"].toDouble();
                        QString detail = QString("%1 — %2")
                                        .arg(person.isEmpty() ? "-" : person,
                                             content.isEmpty() ? "(无内容)" : content);
                        items << QStringList({typ, detail, QString::number(fee, 'f', 2)});
                    }
                }
            }

            dt->setRowCount(items.size());
            for (int i = 0; i < items.size(); i++) {
                dt->setItem(i, 0, new QTableWidgetItem(items[i][0]));
                dt->setItem(i, 1, new QTableWidgetItem(items[i][1]));
                dt->setItem(i, 2, new QTableWidgetItem(items[i][2]));
                dt->removeCellWidget(i, 3); // 清除可能的旧widget
            }
        } else {
            // ========== 材料模式 ==========
            dt->setHorizontalHeaderLabels({"备件名称","数量","单价","总价"});
            dt->setColumnHidden(3, false);

            RemoteQuery mq;
            mq.prepare("SELECT part_name, COUNT(*) AS qty, unit_price, SUM(subtotal) AS subtotal "
                       "FROM t_workorder_item "
                       "WHERE workorder_id = :oid AND item_type = '材料' "
                       "GROUP BY part_name, unit_price ORDER BY part_name");
            mq.bindValue(":oid", o->workorderId);
            mq.exec();

            int r = 0;
            while (mq.next()) {
                dt->setRowCount(r + 1);
                dt->setItem(r, 0, new QTableWidgetItem(mq.value(0).toString()));
                dt->setItem(r, 1, new QTableWidgetItem(QString::number(mq.value(1).toInt())));
                dt->setItem(r, 2, new QTableWidgetItem(QString("¥%1").arg(mq.value(2).toDouble(), 0, 'f', 2)));
                dt->setItem(r, 3, new QTableWidgetItem(QString("¥%1").arg(mq.value(3).toDouble(), 0, 'f', 2)));
                r++;
            }
            if (r == 0) {
                dt->setRowCount(1);
                dt->setItem(0, 0, new QTableWidgetItem("（无材料记录）"));
            }
        }
    };

    connect(ot, &QTableWidget::currentCellChanged, [&](int r, int, int, int) {
        if (r >= 0 && r < orders.size()) { selectedIdx = r; refresh(); }
    });
    connect(cbM, &QCheckBox::toggled, [&]() { refresh(); });
    connect(cbB, &QCheckBox::toggled, [&]() { refresh(); });
    connect(cbP, &QCheckBox::toggled, [&]() { refresh(); });
    connect(bg, QOverload<int>::of(&QButtonGroup::idClicked), [&](int id) { displayMode = id; refresh(); });

    mainHL->addLayout(leftL, 3);
    mainHL->addLayout(rightL, 7);

    if (!orders.isEmpty()) { ot->selectRow(0); selectedIdx = 0; refresh(); }
    dlg.exec();
}

// ============================================================
// 打印结算单（面向客户）
// ============================================================
void FrontDeskPage::onPrintSettlement()
{
    QMessageBox::information(this,"提示","结算单打印请到「结算管理-工单结算」界面操作");
}
