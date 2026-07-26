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

#define S_BTN1 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#3498db;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN1H S_BTN1 "QPushButton:hover{background:#2980b9;}"
#define S_BTN2 "QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#27ae60;color:#fff;font-size:12px;font-weight:bold;}"
#define S_BTN2H S_BTN2 "QPushButton:hover{background:#219a52;}"
#define S_BTNG "QPushButton{padding:6px 14px;border:1px solid #bdc3c7;border-radius:3px;background:#ecf0f1;font-size:12px;}"
#define S_BTNGH S_BTNG "QPushButton:hover{background:#d5dbdb;}"

FrontDeskPage::FrontDeskPage(QWidget *parent)
    : QWidget(parent), m_lockedVid(0), m_foundVid(0) { setupUI(); }
FrontDeskPage::~FrontDeskPage() {}

void FrontDeskPage::refreshData() { resetForm(); }

// ============================================================
// setupUI
// ============================================================
void FrontDeskPage::setupUI()
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(10,6,10,6); outer->setSpacing(4);
    QLabel *title = new QLabel("前台工作台");
    title->setStyleSheet("font-size:17px;font-weight:bold;color:#2c3e50;");
    outer->addWidget(title);

    QScrollArea *sa = new QScrollArea; sa->setWidgetResizable(true); sa->setFrameShape(QFrame::NoFrame);
    QWidget *c = new QWidget; QVBoxLayout *cl = new QVBoxLayout(c);
    cl->setContentsMargins(0,0,0,0); cl->setSpacing(4);
    auto L = [](const QString &t){ return new QLabel(t); };

    // ==================== 1. 车辆搜索（多字段） ====================
    QGroupBox *g1 = new QGroupBox("车辆查找");
    QGridLayout *sg = new QGridLayout(g1);
    sg->setContentsMargins(6,4,6,4); sg->setSpacing(3);
    sg->setColumnStretch(1,1); sg->setColumnStretch(3,1); sg->setColumnStretch(5,1);

    m_sPlate = new QLineEdit;  m_sPlate->setPlaceholderText("车牌号");
    m_sVin   = new QLineEdit;  m_sVin->setPlaceholderText("VIN");
    m_sEngine = new QLineEdit; m_sEngine->setPlaceholderText("发动机号");
    m_sOwner = new QLineEdit;  m_sOwner->setPlaceholderText("车主");
    m_sPhone = new QLineEdit;  m_sPhone->setPlaceholderText("联系电话");
    m_sModel = new QLineEdit;  m_sModel->setPlaceholderText("车型");

    sg->addWidget(L("车牌号:"),0,0); sg->addWidget(m_sPlate,0,1);
    sg->addWidget(L("VIN:"),0,2);    sg->addWidget(m_sVin,0,3);
    sg->addWidget(L("发动机号:"),0,4); sg->addWidget(m_sEngine,0,5);
    sg->addWidget(L("车主:"),1,0);   sg->addWidget(m_sOwner,1,1);
    sg->addWidget(L("电话:"),1,2);   sg->addWidget(m_sPhone,1,3);
    sg->addWidget(L("车型:"),1,4);   sg->addWidget(m_sModel,1,5);

    QHBoxLayout *btnRow = new QHBoxLayout;
    m_btnLock = new QPushButton("锁定车辆");   m_btnLock->setStyleSheet(S_BTN2H);
    m_btnUnlock = new QPushButton("解锁");      m_btnUnlock->setStyleSheet(S_BTNGH);
    m_lblStatus = new QLabel("未锁定");
    m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;font-size:13px;");
    btnRow->addWidget(m_btnLock); btnRow->addWidget(m_btnUnlock);
    btnRow->addWidget(m_lblStatus); btnRow->addStretch();
    sg->addLayout(btnRow,2,0,1,6);

    cl->addWidget(g1);

    // ==================== 2. 车辆信息展示（锁定后只读） ====================
    m_infoGroup = new QGroupBox("当前车辆信息");
    m_infoGroup->setVisible(false);
    QGridLayout *ig = new QGridLayout(m_infoGroup);
    ig->setContentsMargins(6,4,6,4); ig->setSpacing(3);
    ig->setColumnStretch(1,1); ig->setColumnStretch(3,1); ig->setColumnStretch(5,1);

    auto mkRO = [](QLineEdit *&e) {
        e = new QLineEdit; e->setReadOnly(true); e->setStyleSheet("background:#f0f0f0;");
    };
    mkRO(m_dispPlate); mkRO(m_dispVin); mkRO(m_dispEngine); mkRO(m_dispModel);
    mkRO(m_dispOwner); mkRO(m_dispPhone); mkRO(m_dispAddress);
    m_dispColor = new QComboBox; m_dispColor->setEnabled(false);
    m_dispFuel  = new QComboBox; m_dispFuel->setEnabled(false);
    m_dispTrans = new QComboBox; m_dispTrans->setEnabled(false);
    m_dispPurchase = new QLabel("-");

    ig->addWidget(L("车牌:"),0,0); ig->addWidget(m_dispPlate,0,1);
    ig->addWidget(L("VIN:"),0,2);  ig->addWidget(m_dispVin,0,3);
    ig->addWidget(L("发动机:"),0,4); ig->addWidget(m_dispEngine,0,5);
    ig->addWidget(L("车型:"),1,0); ig->addWidget(m_dispModel,1,1);
    ig->addWidget(L("车主:"),1,2); ig->addWidget(m_dispOwner,1,3);
    ig->addWidget(L("电话:"),1,4); ig->addWidget(m_dispPhone,1,5);
    ig->addWidget(L("地址:"),2,0); ig->addWidget(m_dispAddress,2,1);
    ig->addWidget(L("颜色:"),2,2); ig->addWidget(m_dispColor,2,3);
    ig->addWidget(L("油类:"),2,4); ig->addWidget(m_dispFuel,2,5);
    ig->addWidget(L("变速箱:"),3,0); ig->addWidget(m_dispTrans,3,1);
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

    ng->addWidget(L("车牌*:"),0,0); ng->addWidget(m_nPlate,0,1);
    ng->addWidget(L("VIN:"),0,2);   ng->addWidget(m_nVin,0,3);
    ng->addWidget(L("发动机:"),0,4); ng->addWidget(m_nEngine,0,5);
    ng->addWidget(L("车型*:"),1,0); ng->addWidget(m_nModel,1,1);
    ng->addWidget(L("颜色:"),1,2);  ng->addWidget(m_nColor,1,3);
    ng->addWidget(L("油类:"),1,4);  ng->addWidget(m_nFuel,1,5);
    ng->addWidget(L("变速箱:"),2,0); ng->addWidget(m_nTrans,2,1);
    ng->addWidget(L("车主*:"),2,2); ng->addWidget(m_nOwner,2,3);
    ng->addWidget(L("电话*:"),2,4); ng->addWidget(m_nPhone,2,5);
    ng->addWidget(L("地址:"),3,0);  ng->addWidget(m_nAddress,3,1);
    ng->addWidget(L("购车时间:"),3,2); ng->addWidget(m_nPurchase,3,3);
    cl->addWidget(m_newGroup);

    // ==================== 4. 派工区 ====================
    QGroupBox *g4 = new QGroupBox("派工信息");
    QGridLayout *dg = new QGridLayout(g4);
    dg->setContentsMargins(6,4,6,4); dg->setSpacing(3);
    dg->setColumnStretch(1,1); dg->setColumnStretch(3,1); dg->setColumnStretch(5,1);

    m_editOrderNo = new QLineEdit; m_editOrderNo->setReadOnly(true); m_editOrderNo->setStyleSheet("background:#f0f0f0;");
    m_cmbAdvisor  = new QComboBox;   m_cmbAdvisor->setMinimumWidth(100);
    m_cmbMainTech = new QComboBox;   m_cmbMainTech->setMinimumWidth(100);
    m_spinMileage = new QSpinBox;    m_spinMileage->setRange(0,9999999); m_spinMileage->setSuffix(" km");
    m_dateRepair = new QDateEdit;    m_dateRepair->setCalendarPopup(true); m_dateRepair->setDisplayFormat("yyyy-MM-dd"); m_dateRepair->setDate(QDate::currentDate());
    m_dateEstimated = new QDateEdit; m_dateEstimated->setCalendarPopup(true); m_dateEstimated->setDisplayFormat("yyyy-MM-dd"); m_dateEstimated->setDate(QDate::currentDate());
    m_cmbShift = new QComboBox;      m_cmbShift->addItems({"","白班","夜班"});
    m_textContent = new QTextEdit;   m_textContent->setPlaceholderText("选填"); m_textContent->setMaximumHeight(50);

    dg->addWidget(L("工单号:"),0,0); dg->addWidget(m_editOrderNo,0,1);
    dg->addWidget(L("顾问:"),0,2);    dg->addWidget(m_cmbAdvisor,0,3);
    dg->addWidget(L("主修:"),0,4);   dg->addWidget(m_cmbMainTech,0,5);
    dg->addWidget(L("公里数:"),1,0); dg->addWidget(m_spinMileage,1,1);
    dg->addWidget(L("报修日期:"),1,2); dg->addWidget(m_dateRepair,1,3);
    dg->addWidget(L("预估日期:"),1,4); dg->addWidget(m_dateEstimated,1,5);
    dg->addWidget(L("班别:"),2,0);   dg->addWidget(m_cmbShift,2,1);
    dg->addWidget(L("内容:"),2,2,Qt::AlignTop); dg->addWidget(m_textContent,2,3,1,3);
    cl->addWidget(g4);

    // ==================== 5. 维修条目 ====================
    QGroupBox *g5 = new QGroupBox("机电 / 钣金 / 喷漆");
    QGridLayout *rg = new QGridLayout(g5);
    rg->setContentsMargins(6,4,6,4); rg->setSpacing(3);
    rg->setColumnStretch(2,1);
    rg->addWidget(L("类别"),0,0); rg->addWidget(L("维修人"),0,1);
    rg->addWidget(L("内容"),0,2); rg->addWidget(L("费用"),0,3);
    QStringList types = {"机电","钣金","喷漆"};
    for (int i = 0; i < 3; i++) {
        QLabel *tl = new QLabel(types[i]); tl->setStyleSheet("font-weight:bold;color:#2c3e50;");
        rg->addWidget(tl, i+1, 0);
        m_items[i].tech    = new QComboBox;    m_items[i].tech->setMinimumWidth(100);
        m_items[i].content = new QLineEdit;    m_items[i].content->setPlaceholderText("内容");
        m_items[i].fee     = new QDoubleSpinBox; m_items[i].fee->setRange(0,999999.99);
        m_items[i].fee->setPrefix("¥ ");        m_items[i].fee->setDecimals(2);
        connect(m_items[i].fee, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
        rg->addWidget(m_items[i].tech, i+1, 1);
        rg->addWidget(m_items[i].content, i+1, 2);
        rg->addWidget(m_items[i].fee, i+1, 3);
    }
    cl->addWidget(g5);

    // ==================== 6. 费用 ====================
    QGroupBox *g6 = new QGroupBox("费用明细");
    QGridLayout *fg = new QGridLayout(g6);
    fg->setContentsMargins(6,4,6,4); fg->setSpacing(3);
    m_spinMat   = new QDoubleSpinBox; m_spinMat->setRange(0,999999.99);   m_spinMat->setPrefix("¥ "); m_spinMat->setDecimals(2);
    m_spinOther = new QDoubleSpinBox; m_spinOther->setRange(0,999999.99); m_spinOther->setPrefix("¥ "); m_spinOther->setDecimals(2);
    m_spinMgmt  = new QDoubleSpinBox; m_spinMgmt->setRange(0,999999.99);  m_spinMgmt->setPrefix("¥ "); m_spinMgmt->setDecimals(2);
    m_spinDep   = new QDoubleSpinBox; m_spinDep->setRange(0,999999.99);   m_spinDep->setPrefix("¥ ");  m_spinDep->setDecimals(2);
    connect(m_spinMat, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    connect(m_spinOther, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    connect(m_spinMgmt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FrontDeskPage::onFeeChanged);
    fg->addWidget(L("材料费:"),0,0); fg->addWidget(m_spinMat,0,1);
    fg->addWidget(L("其它费:"),0,2); fg->addWidget(m_spinOther,0,3);
    fg->addWidget(L("管理费:"),0,4); fg->addWidget(m_spinMgmt,0,5);
    fg->addWidget(L("订金:"),1,0);   fg->addWidget(m_spinDep,1,1);
    m_lblTotal = new QLabel("¥ 0.00");
    m_lblTotal->setStyleSheet("font-size:20px;font-weight:bold;color:#e74c3c;");
    fg->addWidget(L("合计:"),1,2,Qt::AlignRight|Qt::AlignVCenter); fg->addWidget(m_lblTotal,1,3,1,2);

    QHBoxLayout *br = new QHBoxLayout;
    m_btnPrint = new QPushButton("打印工单(内部)"); m_btnPrint->setStyleSheet("QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#8e44ad;color:#fff;font-size:12px;font-weight:bold;}QPushButton:hover{background:#7d3c98;}");
    m_btnCreate = new QPushButton("保存并派工"); m_btnCreate->setStyleSheet(S_BTN1H);
    m_btnCreate->setMinimumHeight(32); m_btnPrint->setMinimumHeight(32);

    QPushButton *btnPrintQuote = new QPushButton("打印报价单(客户)");
    btnPrintQuote->setStyleSheet("QPushButton{padding:6px 14px;border:none;border-radius:3px;background:#16a085;color:#fff;font-size:12px;font-weight:bold;}QPushButton:hover{background:#138d75;}");
    btnPrintQuote->setMinimumHeight(32);
    connect(btnPrintQuote, &QPushButton::clicked, this, &FrontDeskPage::onPrintQuote);

    br->addStretch(); br->addWidget(btnPrintQuote); br->addWidget(m_btnPrint); br->addWidget(m_btnCreate);
    fg->addLayout(br,2,0,1,6);
    cl->addWidget(g6);
    cl->addStretch();

    sa->setWidget(c); outer->addWidget(sa, 1);

    // ==================== 信号 ====================
    connect(m_btnLock,   &QPushButton::clicked, this, &FrontDeskPage::onLockVehicle);
    connect(m_btnUnlock, &QPushButton::clicked, this, &FrontDeskPage::onClearVehicle);
    connect(m_btnCreate, &QPushButton::clicked, this, &FrontDeskPage::onCreateWorkOrder);
    connect(m_btnPrint,  &QPushButton::clicked, this, &FrontDeskPage::onPrintWorkOrder);

    // 搜索：回车/Tab/失焦触发搜索，不自定计时触发
    QList<QLineEdit*> searchFields = {m_sPlate, m_sVin, m_sEngine, m_sOwner, m_sPhone, m_sModel};
    for (auto *e : searchFields) {
        connect(e, &QLineEdit::editingFinished, this, &FrontDeskPage::triggerFuzzySearch);
    }

    loadCombos(); resetForm();
}

// ============================================================
// loadCombos
// ============================================================
void FrontDeskPage::loadCombos()
{
    m_cmbAdvisor->clear(); m_cmbAdvisor->addItem("",0);
    QSqlQuery q(DbManager::instance().database());
    q.exec("SELECT id,name FROM t_employee WHERE position IN ('服务顾问','总经理') AND is_active=1 ORDER BY name");
    while (q.next()) m_cmbAdvisor->addItem(q.value(1).toString(), q.value(0).toInt());

    m_cmbMainTech->clear(); m_cmbMainTech->addItem("",0);
    q.exec("SELECT id,name,position FROM t_employee WHERE is_active=1 ORDER BY name");
    while (q.next()) m_cmbMainTech->addItem(QString("%1(%2)").arg(q.value(1).toString(),q.value(2).toString()), q.value(0).toInt());

    for (int i = 0; i < 3; i++) { m_items[i].tech->clear(); m_items[i].tech->addItem("",0); }
    q.exec("SELECT id,name FROM t_employee WHERE position='维修技师' AND is_active=1 ORDER BY name");
    while (q.next()) {
        int id = q.value(0).toInt(); QString n = q.value(1).toString();
        for (int i = 0; i < 3; i++) m_items[i].tech->addItem(n, id);
    }
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
    m_infoGroup->setVisible(false); m_newGroup->setVisible(false);

    m_editOrderNo->setText(generateOrderNo());
    m_spinMileage->setValue(0); m_cmbAdvisor->setCurrentIndex(0);
    m_cmbMainTech->setCurrentIndex(0); m_textContent->clear();
    m_dateRepair->setDate(QDate::currentDate()); m_dateEstimated->setDate(QDate::currentDate());
    m_cmbShift->setCurrentIndex(0);
    for (int i = 0; i < 3; i++) { m_items[i].tech->setCurrentIndex(0); m_items[i].content->clear(); m_items[i].fee->setValue(0); }
    m_spinMat->setValue(0); m_spinOther->setValue(0); m_spinMgmt->setValue(0); m_spinDep->setValue(0);
    m_lblTotal->setText("¥ 0.00");
}

double FrontDeskPage::calcTotalFee()
{
    double t = 0; for (int i = 0; i < 3; i++) t += m_items[i].fee->value();
    return t + m_spinMat->value() + m_spinOther->value() + m_spinMgmt->value();
}
void FrontDeskPage::onFeeChanged() { m_lblTotal->setText(QString("¥ %1").arg(calcTotalFee(),0,'f',2)); }

// ============================================================
// 多字段模糊搜索
// ============================================================
void FrontDeskPage::triggerFuzzySearch()
{
    QString f[6] = {m_sPlate->text().trimmed(), m_sVin->text().trimmed(), m_sEngine->text().trimmed(),
                    m_sOwner->text().trimmed(), m_sPhone->text().trimmed(), m_sModel->text().trimmed()};
    bool any = false; for (auto &s : f) if (!s.isEmpty()) { any = true; break; }
    if (!any) {
        m_foundVid = 0; clearGhost(); m_lblStatus->setText("未锁定");
        m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;"); return;
    }

    // 构建查询：6个字段OR匹配 — 只使用老数据库也有的列
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

    m_newGroup->setVisible(false);

    if (items.isEmpty()) {
        // 无匹配 → 显示新车录入
        m_foundVid = 0; clearGhost();
        m_lblStatus->setText("未锁定"); m_lblStatus->setStyleSheet("color:#e74c3c;font-weight:bold;");
        m_infoGroup->setVisible(false); m_newGroup->setVisible(true);
        return;
    }

    if (items.size() == 1) {
        // 唯一匹配 → 自动锁定
        m_lockedVid = items[0].id;
        m_foundVid = 0;
        clearGhost();
        fillVehicleData(items[0].id);
        m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
        m_infoGroup->setVisible(true);
        m_editOrderNo->setText(generateOrderNo());
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
        m_infoGroup->setVisible(true);
        m_editOrderNo->setText(generateOrderNo());
    } else {
        m_lockedVid = 0;
        m_foundVid = 0;
    }
}

// ============================================================
// fillVehicleData — 填充车辆信息展示区（辅助输入）
// ============================================================
void FrontDeskPage::fillVehicleData(int vid)
{
    QSqlQuery q(DbManager::instance().database());
    // 使用新旧两套 schema 都兼容的列
    // 旧: 没有 color/fuel_type/transmission, t_customer 没有 address
    q.prepare("SELECT v.plate_number,v.vin,v.engine_number,v.model,"
              "v.purchase_date,c.name,c.phone "
              "FROM t_vehicle v LEFT JOIN t_customer c ON c.vehicle_id=v.id AND c.type='车主' "
              "WHERE v.id=:id");
    q.bindValue(":id", vid);
    DbManager::instance().executeQuery(q);
    if (!q.next()) return;

    m_dispPlate->setText(q.value(0).toString());
    m_dispVin->setText(q.value(1).toString());
    m_dispEngine->setText(q.value(2).toString());
    m_dispModel->setText(q.value(3).toString());
    m_dispColor->setCurrentText("");     // 旧数据库没有 color 列
    m_dispFuel->setCurrentText("");       // 旧数据库没有 fuel_type 列
    m_dispTrans->setCurrentText("");      // 旧数据库没有 transmission 列
    m_dispPurchase->setText(q.value(4).toDate().toString("yyyy-MM-dd"));
    m_dispOwner->setText(q.value(5).toString());
    m_dispPhone->setText(q.value(6).toString());
}

// ============================================================
// 锁定 — 搜索已自动锁定，此按钮主要处理新车录入保存
// ============================================================
void FrontDeskPage::onLockVehicle()
{
    // 已通过 triggerFuzzySearch 自动锁定
    if (m_lockedVid > 0) {
        QMessageBox::information(this, "已锁定", "车辆已锁定: " + m_dispPlate->text());
        return;
    }

    // 新车录入区已显示且有内容 → 保存新车
    if (m_newGroup->isVisible()) {
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
        q.prepare("INSERT INTO t_vehicle (plate_number,vin,engine_number,model,purchase_date) "
                  "VALUES (:p,:v,:e,:m,:pd)");
        q.bindValue(":p",np); q.bindValue(":v",m_nVin->text().trimmed().isEmpty()?QVariant():m_nVin->text().trimmed());
        q.bindValue(":e",m_nEngine->text().trimmed().isEmpty()?QVariant():m_nEngine->text().trimmed());
        q.bindValue(":m",md);
        q.bindValue(":pd",m_nPurchase->date());
        if (!DbManager::instance().executeQuery(q)) { DbManager::instance().rollbackTransaction(); QMessageBox::warning(this,"保存失败",q.lastError().text()); return; }
        m_lockedVid = q.lastInsertId().toInt();
        q.prepare("INSERT INTO t_customer (vehicle_id,name,phone,type) VALUES (:vid,:n,:p,'车主')");
        q.bindValue(":vid",m_lockedVid); q.bindValue(":n",ow); q.bindValue(":p",ph);
        DbManager::instance().executeQuery(q);
        DbManager::instance().commitTransaction();

        fillVehicleData(m_lockedVid);
        m_sPlate->setText(np);
        m_lblStatus->setText("已锁定"); m_lblStatus->setStyleSheet("color:#27ae60;font-weight:bold;");
        m_infoGroup->setVisible(true); m_newGroup->setVisible(false);
        m_editOrderNo->setText(generateOrderNo());
        QMessageBox::information(this,"成功","新车已保存并锁定 "+np);
        return;
    }

    // 兜底：手动触发一次搜索
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
    m_infoGroup->setVisible(false); m_newGroup->setVisible(false);
    m_editOrderNo->setText(generateOrderNo());
}

void FrontDeskPage::clearGhost()
{
    m_dispPlate->clear(); m_dispVin->clear(); m_dispEngine->clear();
    m_dispModel->clear(); m_dispOwner->clear(); m_dispPhone->clear(); m_dispAddress->clear();
    m_dispColor->setCurrentIndex(0); m_dispFuel->setCurrentIndex(0); m_dispTrans->setCurrentIndex(0);
    m_dispPurchase->setText("-");
}

// ============================================================
// 创建工单
// ============================================================
void FrontDeskPage::onCreateWorkOrder()
{
    if (m_lockedVid == 0) { QMessageBox::warning(this,"提示","请先锁定车辆"); return; }
    if (m_cmbMainTech->currentData().toInt() == 0) { QMessageBox::warning(this,"提示","请选主修人"); return; }
    bool hasItem = false;
    for (int i = 0; i < 3; i++) {
        if (m_items[i].tech->currentData().toInt()>0 || !m_items[i].content->text().trimmed().isEmpty() || m_items[i].fee->value()>0)
        { hasItem = true; break; }
    }
    if (!hasItem) { QMessageBox::warning(this,"提示","请至少填一个维修条目"); return; }

    QString orderNo = m_editOrderNo->text();
    if (orderNo.isEmpty()) { orderNo = generateOrderNo(); m_editOrderNo->setText(orderNo); }

    double labor = 0; for (int i = 0; i < 3; i++) labor += m_items[i].fee->value();
    double mat = m_spinMat->value(), oth = m_spinOther->value(), mgmt = m_spinMgmt->value();
    double total = labor + mat + oth + mgmt;

    DbManager::instance().beginTransaction();
    QSqlQuery q(DbManager::instance().database());
    q.prepare("INSERT INTO t_workorder (order_no,vehicle_id,technician_id,customer_service_id,mileage,"
              "repair_content,repair_date,estimated_date,shift,main_technician,"
              "material_fee,other_fee,management_fee,labor_fee,total_amount,deposit,status,created_by) "
              "VALUES (:no,:vid,:tid,:csi,:mile,:cont,:rd,:ed,:sh,:mtech,"
              ":mf,:of,:mgf,:lf,:total,:dep,'待派工',:creator)");
    q.bindValue(":no",orderNo); q.bindValue(":vid",m_lockedVid);
    q.bindValue(":tid",m_cmbMainTech->currentData().toInt());
    q.bindValue(":csi",m_cmbAdvisor->currentData().toInt()>0?m_cmbAdvisor->currentData().toInt():QVariant());
    q.bindValue(":mile",m_spinMileage->value());
    q.bindValue(":cont",m_textContent->toPlainText());
    q.bindValue(":rd",m_dateRepair->date()); q.bindValue(":ed",m_dateEstimated->date());
    q.bindValue(":sh",m_cmbShift->currentText()); q.bindValue(":mtech",m_cmbMainTech->currentText());
    q.bindValue(":mf",mat); q.bindValue(":of",oth); q.bindValue(":mgf",mgmt);
    q.bindValue(":lf",labor); q.bindValue(":total",total); q.bindValue(":dep",m_spinDep->value());
    q.bindValue(":creator",Session::instance().userId());
    if (!DbManager::instance().executeQuery(q)) { DbManager::instance().rollbackTransaction(); QMessageBox::warning(this,"失败",q.lastError().text()); return; }

    int woid = q.lastInsertId().toInt();
    QStringList types = {"机电","钣金","喷漆"};
    for (int i = 0; i < 3; i++) {
        int techId = m_items[i].tech->currentData().toInt();
        QString cont = m_items[i].content->text().trimmed();
        double fee = m_items[i].fee->value();
        if (techId == 0 && cont.isEmpty() && fee == 0) continue;
        q.prepare("INSERT INTO t_workorder_repair_item (workorder_id,item_type,repair_person,repair_content,fee) "
                  "VALUES (:woid,:type,:person,:cont,:fee)");
        q.bindValue(":woid",woid); q.bindValue(":type",types[i]);
        q.bindValue(":person",m_items[i].tech->currentText()); q.bindValue(":cont",cont); q.bindValue(":fee",fee);
        DbManager::instance().executeQuery(q);
        if (techId > 0) {
            q.prepare("INSERT INTO t_technician_work_record (workorder_id,technician_id,item_type,work_content,fee) "
                      "VALUES (:woid,:tid,:type,:cont,:fee)");
            q.bindValue(":woid",woid); q.bindValue(":tid",techId);
            q.bindValue(":type",types[i]); q.bindValue(":cont",cont); q.bindValue(":fee",fee);
            DbManager::instance().executeQuery(q);
        }
    }
    q.prepare("INSERT INTO t_vehicle_transaction (vehicle_id,workorder_id,transaction_type,description,operator_id) "
              "VALUES (:vid,:woid,'进厂维修',:desc,:op)");
    q.bindValue(":vid",m_lockedVid); q.bindValue(":woid",woid);
    q.bindValue(":desc",QString("创建工单 %1").arg(orderNo));
    q.bindValue(":op",Session::instance().userId());
    DbManager::instance().executeQuery(q);
    DbManager::instance().commitTransaction();

    QMessageBox::information(this,"派工成功",QString("工单 %1 已创建\n总费用: ¥%2").arg(orderNo).arg(total,0,'f',2));
    emit workOrderCreated(woid, orderNo);
    onClearVehicle();
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
        QStringList types = {"机电","钣金","喷漆"};
        for (int i = 0; i < 3; i++) {
            QString tech = m_items[i].tech->currentText(), cont = m_items[i].content->text().trimmed();
            double fee = m_items[i].fee->value();
            if (tech.isEmpty() && cont.isEmpty() && fee==0) continue;
            rows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>¥%4</td></tr>").arg(types[i],tech,cont).arg(fee,0,'f',2);
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
// 打印报价单（面向客户）
// ============================================================
void FrontDeskPage::onPrintQuote()
{
    QString no = m_editOrderNo->text();
    if (no.isEmpty() || m_lockedVid == 0) {
        QMessageBox::warning(this,"提示","请先锁定车辆并创建工单");
        return;
    }
    QPrinter printer; QPrintPreviewDialog pp(&printer, this);
    connect(&pp, &QPrintPreviewDialog::paintRequested, [&](QPrinter *p) {
        QTextDocument doc;
        QString rows;
        QStringList types = {"机电","钣金","喷漆"};
        for (int i = 0; i < 3; i++) {
            QString tech = m_items[i].tech->currentText(), cont = m_items[i].content->text().trimmed();
            double fee = m_items[i].fee->value();
            if (tech.isEmpty() && cont.isEmpty() && fee==0) continue;
            rows += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>¥%4</td></tr>").arg(types[i],tech,cont).arg(fee,0,'f',2);
        }
        double labor = calcTotalFee();
        double mat = m_spinMat->value();
        double oth = m_spinOther->value();
        double mgmt = m_spinMgmt->value();
        double total = labor + mat + oth + mgmt;
        double dep = m_spinDep->value();

        // 先转换所有数值为QString，避免arg()变参模板的类型冲突(Qt 6)
        QString sLabor   = QString::number(labor, 'f', 2);
        QString sMat     = QString::number(mat, 'f', 2);
        QString sOth     = QString::number(oth, 'f', 2);
        QString sMgmt    = QString::number(mgmt, 'f', 2);
        QString sDep     = QString::number(dep, 'f', 2);
        QString sTotal   = QString::number(total, 'f', 2);
        QString sMileage = QString::number(m_spinMileage->value());
        QString sDate    = m_dateRepair->date().toString("yyyy-MM-dd");
        QString sNow     = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

        QString html = QString(
            "<div style='text-align:center;'><h2>维修报价单</h2><hr></div>"
            "<p><b>工单号：</b>%1</p>"
            "<p><b>车牌号：</b>%2 &nbsp;&nbsp; <b>车型：</b>%3</p>"
            "<p><b>车主：</b>%4 &nbsp;&nbsp; <b>电话：</b>%5</p>"
            "<p><b>公里数：</b>%6 km &nbsp;&nbsp; <b>报修日期：</b>%7</p>"
            "<hr><h3>维修项目</h3>"
            "<table border='1' cellpadding='6' style='border-collapse:collapse;width:100%;'>"
            "<tr style='background:#34495e;color:white;'><th>类别</th><th>维修人</th><th>内容</th><th>费用</th></tr>%8</table>"
            "<br><table border='0' cellpadding='4' style='width:100%;'>"
            "<tr><td align='right'><b>工时费合计：</b></td><td align='right' width='120'>¥%9</td></tr>"
            "<tr><td align='right'><b>材料费：</b></td><td align='right'>¥%10</td></tr>"
            "<tr><td align='right'><b>其它费：</b></td><td align='right'>¥%11</td></tr>"
            "<tr><td align='right'><b>管理费：</b></td><td align='right'>¥%12</td></tr>"
            "<tr><td align='right'><b>订金：</b></td><td align='right'>¥%13</td></tr>"
            "<tr style='font-size:16px;color:#e74c3c;'><td align='right'><b>总计：</b></td><td align='right'><b>¥%14</b></td></tr>"
            "</table>"
            "<hr><p style='color:#7f8c8d;font-size:12px;'>本报价单有效期3天，最终价格以实际结算为准。<br>打印时间：%15</p>"
        ).arg(no, m_dispPlate->text(), m_dispModel->text(),
              m_dispOwner->text(), m_dispPhone->text(),
              sMileage, sDate, rows,
              sLabor, sMat, sOth, sMgmt, sDep, sTotal, sNow);
        doc.setHtml(html); doc.print(p);
    });
    pp.exec();
}

// ============================================================
// 打印结算单（面向客户）
// ============================================================
void FrontDeskPage::onPrintSettlement()
{
    // 结算单通常在结算页面打印，这里留空以防后续需要
    QMessageBox::information(this,"提示","结算单打印请到「结算管理-工单结算」界面操作");
}
