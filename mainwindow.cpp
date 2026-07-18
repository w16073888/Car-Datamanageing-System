#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
 * ============================================================
 *  MainWindow — 汽修公司数据记录及管理系统 主窗口
 *
 *  页面状态说明：
 *    state=0 ：主界面（四个功能入口 + 右侧提示）
 *    state=1 ：保修检索界面（输入车牌号，检索后跳转）
 *    state=2 ：车辆首次入库登记界面（填写车辆+车主信息）
 *    state=3 ：报修界面（填写维修信息，关联车辆/车主）
 *    state=4 ：库房管理（占位）
 *    state=9 ：结算（占位）
 *    state=10：查询（占位）
 *
 *  布局结构（三段式注释标记）：
 *    1. -- 基础页面信息 --   窗口基本属性、QSS、页面创建、动画
 *    2. -- 按钮配置 -       所有按钮的创建和布局添加
 *    3. -- 逻辑管控 --       所有信号槽的 connect
 *    4. -- 每一页编辑 --/     各页面的样式微调
 * ============================================================
 */
/* 定义静态常量：日期选择控件的"未设置"标记值 */
const QDate MainWindow::UNSET_DATE(1900, 1, 1);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

/*-----------------基础页面信息-----------------------*/

    // ---------- 窗口基本属性 ----------
    this->setMinimumSize(1050, 600);
    state = 0;
    book = new QStackedWidget(this);

    // 窗口图标
    this->setWindowIcon(QIcon(basedataapi::getInstance().getRootPath() + "/logo_256x256.ico"));
    qDebug() << "文件是否存在：" << QFile::exists(basedataapi::getInstance().getRootPath() + "/logo_256x256.ico");
    qDebug() << "是否为空：" << QIcon(basedataapi::getInstance().getRootPath() + "/logo_256x256.ico").isNull();
    qDebug() << "可用尺寸：" << QIcon(basedataapi::getInstance().getRootPath() + "/logo_256x256.ico").availableSizes();
    this->setWindowTitle("Manager System");

    // ---------- 全局 QSS（保持原有设计不变） ----------
    // 注意：不要在此处修改 QSS，每页的附加样式请在最后一节"每一页编辑"中添加
    this->setStyleSheet(
        "QMainWindow {"
        "    background-image: url(" + basedataapi::getInstance().getRootPath() + "/BackGround.png);"
        "    background-repeat: no-repeat;"
        "    background-position: center;"
        "    background-attachment: fixed;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #5fa88f;"
        "    border: 2px solid #5fa88f;"
        "    border-radius: 2px;"
        "    padding: 14px 28px;"
        "    font-size: 14px;"
        "    letter-spacing: 2px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #a8e6cf;"
        "    color: #0a0a1a;"
        "}"
    );

    // ---------- 创建 7 个页面（对应 state 0,1,2,3,4,9,10） ----------
    for (int i = 0; i < 7; i++) {
        page[i] = new QWidget;
        layout[i] = new QGridLayout(page[i]);
        book->addWidget(page[i]);

        // 每个页面第 0 行放标题
        QLabel *titleLabel = new QLabel(page[i]);

        if (i == 0) {
            // 主界面使用图片标题
            QPixmap pixmap(basedataapi::getInstance().getRootPath() + "/SysTitle.png");
            if (!pixmap.isNull()) {
                int targetWidth = 1200;
                QPixmap scaledPixmap = pixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation);
                titleLabel->setPixmap(scaledPixmap);
                titleLabel->setScaledContents(true);
            } else {
                titleLabel->setText("SysTitle.png 加载失败");
                titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #5fa88f;");
            }
        } else {
            // 子页面使用文字标题
            titleLabel->setText("Manager System — 汽修管理");
            titleLabel->setAlignment(Qt::AlignCenter);
            titleLabel->setStyleSheet(
                "font-size: 20px; font-weight: bold; color: #5fa88f; "
                "background: transparent; padding: 8px;"
            );
        }
        layout[i]->addWidget(titleLabel, 0, 0, 1, 7);
        layout[i]->setRowStretch(0, 0);   // 标题行不伸缩
    }

    // ---------- 淡入淡出动画 ----------
    this->opacotyEffect = new QGraphicsOpacityEffect(this);
    this->opacotyEffect->setOpacity(1.0);
    this->fadeAnimation = new QPropertyAnimation(this->opacotyEffect, "opacity");
    this->fadeAnimation->setDuration(600);
    this->fadeAnimation->setStartValue(1.0);
    this->fadeAnimation->setEndValue(0);
    book->setGraphicsEffect(this->opacotyEffect);


/*----------------孩子们这一坨是按钮配置------------------------*/
    /* 注意：
     *   所有按钮在此处创建，并添加到对应页面的布局（layout[i]）中。
     *   部分页面需要的标签（QLabel）和输入控件也在此创建，
     *   因为它们的"创建"和"布局添加"是强耦合的。
     *   信号槽连接请到下一节"逻辑管控"中统一编写。
     */

    // ==========================================================
    // State 0 — 主界面
    //  左侧：四个功能按钮（纵向排列）
    //  右侧：提示文本区域（只读）
    // ==========================================================
    btnToRepair    = new QPushButton("保修",     page[0]);
    btnToWarehouse = new QPushButton("库房管理", page[0]);
    btnToSettle    = new QPushButton("结算",     page[0]);
    btnToQuery     = new QPushButton("查询",     page[0]);

    textMainHint   = new QTextEdit(page[0]);
    textMainHint->setReadOnly(true);                     // 只读
    textMainHint->setText("提示：\n当前暂无提示！");       // 默认显示

    // 布局：按钮占第 2~5 行第 0 列；提示区占第 2~5 行第 2~6 列
    layout[0]->addWidget(btnToRepair,    2, 0, 1, 1);
    layout[0]->addWidget(btnToWarehouse, 3, 0, 1, 1);
    layout[0]->addWidget(btnToSettle,    4, 0, 1, 1);
    layout[0]->addWidget(btnToQuery,     5, 0, 1, 1);
    layout[0]->addWidget(textMainHint,   2, 2, 4, 5);

    // 让左侧按钮列和右侧文本区之间有自然间隔
    layout[0]->setColumnStretch(0, 1);   // 按钮列
    layout[0]->setColumnStretch(1, 1);   // 间隔
    layout[0]->setColumnStretch(2, 3);   // 文本区

    // ==========================================================
    // State 1 — 保修检索界面
    //  左侧：车牌号输入框
    //  右侧：检索结果显示区（含"确定"按钮）
    // ==========================================================
    QLabel *lblSearchPlate = new QLabel("请输入车牌号：",     page[1]);

    editSearchPlate      = new QLineEdit(page[1]);

    // 左侧输入区布局（第 1 行，第 0~1 列）
    layout[1]->addWidget(lblSearchPlate,     1, 0, 1, 1);
    layout[1]->addWidget(editSearchPlate,     1, 1, 1, 1);

    // 右侧检索结果滚动区（第 1~6 行，第 2~6 列）
    scrollSearchResult = new QScrollArea(page[1]);
    widgetSearchResult = new QWidget;
    layoutSearchResult = new QVBoxLayout(widgetSearchResult);
    scrollSearchResult->setWidget(widgetSearchResult);
    scrollSearchResult->setWidgetResizable(true);
    layout[1]->addWidget(scrollSearchResult, 1, 2, 6, 5);

    // 左侧列宽比例
    layout[1]->setColumnStretch(0, 2);   // 标签列
    layout[1]->setColumnStretch(1, 3);   // 输入框列
    layout[1]->setColumnStretch(2, 5);   // 结果区

    // ==========================================================
    // State 2 — 车辆首次入库登记界面
    //  左侧：车辆信息（车牌号必填 + 选填项）
    //  右侧：车主信息（选填）+ 取消/保存按钮
    // ==========================================================
    // 左侧 — 标题
    QLabel *lblRegCarTitle = new QLabel("车辆首次入库，请输入车辆信息", page[2]);
    lblRegCarTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #5fa88f;");
    // 左侧 — 输入控件
    QLabel *lblRegPlate   = new QLabel("车牌号（必填）：",   page[2]);
    QLabel *lblRegVin     = new QLabel("车架号（VIN）：",    page[2]);
    QLabel *lblRegEngine  = new QLabel("发动机号：",         page[2]);
    QLabel *lblRegPurDate = new QLabel("购车日期：",         page[2]);
    QLabel *lblRegInsDate = new QLabel("保险日期：",         page[2]);
    editRegPlate   = new QLineEdit(page[2]);
    editRegVin     = new QLineEdit(page[2]);
    editRegEngine  = new QLineEdit(page[2]);
    // 日期选择控件（特殊值文本"未设置"表示未选）
    dateRegPurchase  = new QDateEdit(page[2]);
    dateRegInsurance = new QDateEdit(page[2]);
    dateRegPurchase->setSpecialValueText("未设置");
    dateRegPurchase->setMinimumDate(UNSET_DATE);
    dateRegPurchase->setDate(UNSET_DATE);
    dateRegPurchase->setCalendarPopup(true);
    dateRegInsurance->setSpecialValueText("未设置");
    dateRegInsurance->setMinimumDate(UNSET_DATE);
    dateRegInsurance->setDate(UNSET_DATE);
    dateRegInsurance->setCalendarPopup(true);

    // 左侧布局（第 1~6 行，第 0~2 列）
    layout[2]->addWidget(lblRegCarTitle, 1, 0, 1, 3);        // 标题跨 3 列
    layout[2]->addWidget(lblRegPlate,    2, 0, 1, 1);
    layout[2]->addWidget(editRegPlate,    2, 1, 1, 2);
    layout[2]->addWidget(lblRegVin,      3, 0, 1, 1);
    layout[2]->addWidget(editRegVin,      3, 1, 1, 2);
    layout[2]->addWidget(lblRegEngine,   4, 0, 1, 1);
    layout[2]->addWidget(editRegEngine,   4, 1, 1, 2);
    layout[2]->addWidget(lblRegPurDate,  5, 0, 1, 1);
    layout[2]->addWidget(dateRegPurchase,5, 1, 1, 2);
    layout[2]->addWidget(lblRegInsDate,  6, 0, 1, 1);
    layout[2]->addWidget(dateRegInsurance,6, 1, 1, 2);

    // 右侧 — 标题
    QLabel *lblRegCusTitle = new QLabel("请输入车主信息", page[2]);
    lblRegCusTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #5fa88f;");
    // 右侧 — 输入控件
    QLabel *lblRegOwnerName  = new QLabel("车主姓名：",  page[2]);
    QLabel *lblRegOwnerPhone = new QLabel("车主电话：",  page[2]);
    editRegOwnerName  = new QLineEdit(page[2]);
    editRegOwnerPhone = new QLineEdit(page[2]);

    // 右侧 — 按钮
    btnRegCancel = new QPushButton("取消", page[2]);
    btnRegSave   = new QPushButton("保存", page[2]);

    // 右侧布局（第 1~7 行，第 3~6 列）
    layout[2]->addWidget(lblRegCusTitle,  1, 3, 1, 4);
    layout[2]->addWidget(lblRegOwnerName,  2, 3, 1, 1);
    layout[2]->addWidget(editRegOwnerName, 2, 4, 1, 3);
    layout[2]->addWidget(lblRegOwnerPhone, 3, 3, 1, 1);
    layout[2]->addWidget(editRegOwnerPhone,3, 4, 1, 3);
    // 按钮放在第 7 行右侧
    layout[2]->addWidget(btnRegCancel, 7, 4, 1, 1);
    layout[2]->addWidget(btnRegSave,   7, 5, 1, 1);

    // 列宽比例
    layout[2]->setColumnStretch(0, 2);
    layout[2]->setColumnStretch(1, 2);
    layout[2]->setColumnStretch(2, 1);   // 左右间隔
    layout[2]->setColumnStretch(3, 2);
    layout[2]->setColumnStretch(4, 2);
    layout[2]->setColumnStretch(5, 1);

    // ==========================================================
    // State 3 — 报修界面
    //  左侧：维修信息输入（责任人/内容/工时费/公里数/驾驶员信息）
    //  右侧：显示本次关联的车辆+车主信息（只读）+ 保存/取消按钮
    // ==========================================================
    // 左侧 — 输入控件
    QLabel *lblRepairPerson  = new QLabel("维修责任人：",  page[3]);
    QLabel *lblRepairContent = new QLabel("报修内容：",    page[3]);
    QLabel *lblRepairCost    = new QLabel("维修工时费：",   page[3]);
    QLabel *lblRepairMileage = new QLabel("行驶公里数：",  page[3]);
    QLabel *lblRepairDrvName = new QLabel("驾驶员姓名：",  page[3]);
    QLabel *lblRepairDrvPhone= new QLabel("驾驶员电话：",  page[3]);

    editRepairPerson    = new QLineEdit(page[3]);
    editRepairContent   = new QLineEdit(page[3]);
    editRepairCost      = new QLineEdit(page[3]);
    editRepairMileage   = new QLineEdit(page[3]);
    editRepairDriverName  = new QLineEdit(page[3]);
    editRepairDriverPhone = new QLineEdit(page[3]);

    // 左侧布局（第 1~6 行，第 0~1 列）
    layout[3]->addWidget(lblRepairPerson,   1, 0, 1, 1);
    layout[3]->addWidget(editRepairPerson,   1, 1, 1, 1);
    layout[3]->addWidget(lblRepairContent,   2, 0, 1, 1);
    layout[3]->addWidget(editRepairContent,  2, 1, 1, 1);
    layout[3]->addWidget(lblRepairCost,      3, 0, 1, 1);
    layout[3]->addWidget(editRepairCost,     3, 1, 1, 1);
    layout[3]->addWidget(lblRepairMileage,   4, 0, 1, 1);
    layout[3]->addWidget(editRepairMileage,  4, 1, 1, 1);
    layout[3]->addWidget(lblRepairDrvName,   5, 0, 1, 1);
    layout[3]->addWidget(editRepairDriverName,5, 1, 1, 1);
    layout[3]->addWidget(lblRepairDrvPhone,  6, 0, 1, 1);
    layout[3]->addWidget(editRepairDriverPhone,6,1, 1, 1);

    // 右侧 — 信息显示区（只读）
    textRepairInfo = new QTextEdit(page[3]);
    textRepairInfo->setReadOnly(true);
    textRepairInfo->setText("请先检索或登记车辆信息");
    layout[3]->addWidget(textRepairInfo, 1, 3, 6, 4);   // 跨第 1~6 行、第 3~6 列

    // 右侧 — 按钮（放在信息区下方第 7 行）
    btnRepairSave   = new QPushButton("保存", page[3]);
    btnRepairCancel = new QPushButton("取消", page[3]);
    layout[3]->addWidget(btnRepairSave,   7, 4, 1, 1);
    layout[3]->addWidget(btnRepairCancel, 7, 5, 1, 1);

    // 列宽
    layout[3]->setColumnStretch(0, 2);
    layout[3]->setColumnStretch(1, 3);
    layout[3]->setColumnStretch(2, 1);   // 间隔
    layout[3]->setColumnStretch(3, 5);   // 信息显示区

    // ==========================================================
    // State 4 — 库房管理（占位）
    // ==========================================================
    lblWarehousePlaceholder = new QLabel("库房管理 —— 功能开发中……", page[4]);
    lblWarehousePlaceholder->setAlignment(Qt::AlignCenter);
    lblWarehousePlaceholder->setStyleSheet("font-size: 24px; color: #5fa88f;");
    layout[4]->addWidget(lblWarehousePlaceholder, 3, 0, 1, 7);

    // ==========================================================
    // State 9 — 结算（占位）
    // ==========================================================
    lblSettlePlaceholder = new QLabel("结算 —— 功能开发中……", page[5]);
    lblSettlePlaceholder->setAlignment(Qt::AlignCenter);
    lblSettlePlaceholder->setStyleSheet("font-size: 24px; color: #5fa88f;");
    layout[5]->addWidget(lblSettlePlaceholder, 3, 0, 1, 7);

    // ==========================================================
    // State 10 — 查询（占位）
    // ==========================================================
    lblQueryPlaceholder = new QLabel("查询 —— 功能开发中……", page[6]);
    lblQueryPlaceholder->setAlignment(Qt::AlignCenter);
    lblQueryPlaceholder->setStyleSheet("font-size: 24px; color: #5fa88f;");
    layout[6]->addWidget(lblQueryPlaceholder, 3, 0, 1, 7);


/*----------------孩子们这一坨是逻辑管控------------------------*/
    /* 注意：
     *   所有信号槽的 connect 调用统一写在此处。
     *   不同页面的连接之间用注释分隔。
     */

    // ==========================================================
    // State 0 — 主界面按钮
    // ==========================================================
    connect(btnToRepair, &QPushButton::clicked, this, [=]() {
        setState(1);
    });
    connect(btnToWarehouse, &QPushButton::clicked, this, [=]() {
        setState(4);
    });
    connect(btnToSettle, &QPushButton::clicked, this, [=]() {
        setState(9);
    });
    connect(btnToQuery, &QPushButton::clicked, this, [=]() {
        setState(10);
    });

    // ==========================================================
    // State 1 — 检索输入（按 Enter 或点击左侧外部触发检索）
    // ==========================================================
    connect(editSearchPlate, &QLineEdit::editingFinished, this, &MainWindow::onSearchInputChanged);

    // ==========================================================
    // State 2 — 入库登记
    // ==========================================================
    connect(btnRegCancel, &QPushButton::clicked, this, [=]() {
        setState(0);   // 取消 → 回到主界面
    });
    connect(btnRegSave, &QPushButton::clicked, this, [=]() {
        // ---- 必填校验：车牌号 ----
        QString plate = editRegPlate->text().trimmed();
        if (plate.isEmpty()) {
            QMessageBox::warning(this, "保存失败", "车牌号不能为空！");
            return;
        }

        // ---- 收集选填数据，空值替换为"无" ----
        QString vin      = editRegVin->text().trimmed();
        QString engine   = editRegEngine->text().trimmed();
        QString purDate  = (dateRegPurchase->date() == UNSET_DATE)
                           ? "无" : dateRegPurchase->date().toString("yyyy-MM-dd");
        QString insDate  = (dateRegInsurance->date() == UNSET_DATE)
                           ? "无" : dateRegInsurance->date().toString("yyyy-MM-dd");
        QString ownerName = editRegOwnerName->text().trimmed();
        QString ownerPhone= editRegOwnerPhone->text().trimmed();

        // 将空字符串替换为"无"
        if (vin.isEmpty())        vin = "无";
        if (engine.isEmpty())     engine = "无";
        if (ownerName.isEmpty())  ownerName = "无";
        if (ownerPhone.isEmpty()) ownerPhone = "无";

        // ---- 保存到数据库 ----
        // 保存车主信息到 cus.db，返回新生成的车主号
        int cusId = basedataapi::getInstance().saveCus(ownerName, ownerPhone);
        if (cusId < 0) {
            QMessageBox::warning(this, "保存失败", "车主信息保存失败，请重试！");
            return;
        }
        // 保存车辆信息到 car.db（年审日期暂用"无"）
        if (!basedataapi::getInstance().saveCar(cusId, plate, vin, engine, purDate,
                                                 "无", insDate)) {
            QMessageBox::warning(this, "保存失败", "车辆信息保存失败，请重试！");
            return;
        }

        // ---- 设置选中信息，跳转到 state=3 ----
        m_selectedPlate      = plate;
        m_selectedVin        = vin;
        m_selectedOwnerName  = ownerName;
        m_selectedOwnerPhone = ownerPhone;

        setState(3);
    });

    // ==========================================================
    // State 3 — 报修
    // ==========================================================
    connect(btnRepairCancel, &QPushButton::clicked, this, [=]() {
        // 弹出确认框
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "确认取消", "确认取消当前保存？",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            setState(0);
        }
        // 否则不跳转，留在当前页面
    });
    connect(btnRepairSave, &QPushButton::clicked, this, [=]() {
        // 收集报修信息（所有字段均为选填，空值替换为"无"）
        QString person  = editRepairPerson->text().trimmed();
        QString content = editRepairContent->text().trimmed();
        QString mileageStr = editRepairMileage->text().trimmed();
        QString costStr    = editRepairCost->text().trimmed();
        QString drvName    = editRepairDriverName->text().trimmed();
        QString drvPhone   = editRepairDriverPhone->text().trimmed();

        if (person.isEmpty())   person = "无";
        if (content.isEmpty())  content = "无";
        int mileage = mileageStr.isEmpty() ? 0 : mileageStr.toInt();
        double cost = costStr.isEmpty() ? 0.0 : costStr.toDouble();
        if (drvName.isEmpty())  drvName = "无";
        if (drvPhone.isEmpty()) drvPhone = "无";

        // 自动生成保修时间（当前时间）
        QString reportTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        // 保存到 ser.db（is_settled = 1 表示"未结算"）
        if (!basedataapi::getInstance().saveSer(person, content, mileage, cost,
                                                 drvName, drvPhone,
                                                 1, reportTime)) {
            QMessageBox::warning(this, "保存失败", "报修信息保存失败，请重试！");
            return;
        }

        QMessageBox::information(this, "保存成功", "报修信息已保存！");
        setState(0);
    });


/*----------------孩子们这一坨每一页编辑更是重量级------------------------*/
    /* 注意：
     *   各页面的专属样式（QSS）写在此处，
     *   不要改动顶部的全局 QSS。
     */

    // ==========================================================
    // State 1 — 检索界面的标签样式
    // ==========================================================
    QString labelStyle = "font-size: 14px; color: #333; background: transparent;";
    lblSearchPlate->setStyleSheet(labelStyle);

    // ==========================================================
    // State 2 — 登记界面的标签样式
    // ==========================================================
    QString regLabelStyle = "font-size: 14px; color: #333; background: transparent;";
    lblRegPlate->setStyleSheet(regLabelStyle);
    lblRegVin->setStyleSheet(regLabelStyle);
    lblRegEngine->setStyleSheet(regLabelStyle);
    lblRegPurDate->setStyleSheet(regLabelStyle);
    lblRegInsDate->setStyleSheet(regLabelStyle);
    lblRegOwnerName->setStyleSheet(regLabelStyle);
    lblRegOwnerPhone->setStyleSheet(regLabelStyle);

    // 必填项稍微强调
    lblRegPlate->setStyleSheet("font-size: 14px; color: #c0392b; background: transparent; font-weight: bold;");

    // ==========================================================
    // State 3 — 报修界面的标签样式
    // ==========================================================
    QString repLabelStyle = "font-size: 14px; color: #333; background: transparent;";
    lblRepairPerson->setStyleSheet(repLabelStyle);
    lblRepairContent->setStyleSheet(repLabelStyle);
    lblRepairCost->setStyleSheet(repLabelStyle);
    lblRepairMileage->setStyleSheet(repLabelStyle);
    lblRepairDrvName->setStyleSheet(repLabelStyle);
    lblRepairDrvPhone->setStyleSheet(repLabelStyle);

    // ==========================================================
    // 所有 QLineEdit 统一风格
    // ==========================================================
    QString lineEditStyle =
        "QLineEdit {"
        "    background-color: rgba(255, 255, 255, 200);"
        "    border: 1px solid #5fa88f;"
        "    border-radius: 4px;"
        "    padding: 6px 10px;"
        "    font-size: 14px;"
        "    color: #333;"
        "}";
    QList<QLineEdit*> allEdits = this->findChildren<QLineEdit*>();
    for (QLineEdit* edit : allEdits) {
        edit->setStyleSheet(lineEditStyle);
    }

    // ==========================================================
    // QTextEdit 风格
    // ==========================================================
    QString textEditStyle =
        "QTextEdit {"
        "    background-color: rgba(255, 255, 255, 200);"
        "    border: 1px solid #5fa88f;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-size: 14px;"
        "    color: #333;"
        "}";
    textMainHint->setStyleSheet(textEditStyle);
    textRepairInfo->setStyleSheet(textEditStyle);

    // ==========================================================
    // QDateEdit 风格
    // ==========================================================
    QString dateEditStyle =
        "QDateEdit {"
        "    background-color: rgba(255, 255, 255, 200);"
        "    border: 1px solid #5fa88f;"
        "    border-radius: 4px;"
        "    padding: 6px 10px;"
        "    font-size: 14px;"
        "    color: #333;"
        "}";
    dateRegPurchase->setStyleSheet(dateEditStyle);
    dateRegInsurance->setStyleSheet(dateEditStyle);

    // ==========================================================
    // 最终设置
    // ==========================================================
    setCentralWidget(book);
    book->setCurrentIndex(0);   // 默认显示主界面
}


/* ============================================================
 *  setState — 切换到指定编号的页面
 *
 *  功能：
 *    - 根据 state 编号找到对应的 page 索引
 *    - 在淡出动画期间完成目标页面的初始化
 *    - 切换后再淡入
 *
 *  参数 n：state 编号（0,1,2,3,4,9,10）
 * ============================================================ */
void MainWindow::setState(int n) {
    state = n;
    int pageIdx = stateToPageIndex(n);

    // ---- 目标页面初始化（在切换前设置好数据） ----
    switch (n) {
    case 0:
        // 主界面 —— 无需额外初始化
        break;

    case 1: {
        // 保修检索 —— 清空输入和结果
        editSearchPlate->blockSignals(true);
        editSearchPlate->clear();
        editSearchPlate->blockSignals(false);

        // 清空检索缓存
        m_searchPlate.clear();

        // 清除旧的动态结果，显示默认提示
        clearSearchResults();
        QLabel *hintLabel = new QLabel("请输入车辆相关信息！");
        hintLabel->setAlignment(Qt::AlignCenter);
        hintLabel->setStyleSheet("font-size: 16px; color: #999; background: transparent;");
        layoutSearchResult->addWidget(hintLabel);
        m_resultRows.append(hintLabel);
        break;
    }

    case 2:
        // 首次入库 —— 用 state=1 的检索结果预填充车牌号
        editRegPlate->setText(m_searchPlate);
        editRegVin->clear();
        editRegEngine->clear();
        dateRegPurchase->setDate(UNSET_DATE);
        dateRegInsurance->setDate(UNSET_DATE);
        editRegOwnerName->clear();
        editRegOwnerPhone->clear();
        break;

    case 3: {
        // 报修 —— 右侧显示当前选中的车辆 + 车主信息
        QString info;
        info += "车辆信息：\n";
        info += "  车牌号：" + m_selectedPlate + "\n";
        info += "  车架号：" + m_selectedVin + "\n";
        info += "\n车主信息：\n";
        info += "  车主姓名：" + m_selectedOwnerName + "\n";
        info += "  车主电话：" + m_selectedOwnerPhone + "\n";
        info += "\n——————————————\n";
        info += "请左侧填写报修信息后点击\"保存\"。";
        textRepairInfo->setText(info);

        // 清空报修输入
        editRepairPerson->clear();
        editRepairContent->clear();
        editRepairMileage->clear();
        editRepairCost->clear();
        editRepairDriverName->clear();
        editRepairDriverPhone->clear();
        break;
    }

    case 4:
        // 库房管理（占位）—— 无初始化
        break;
    case 9:
        // 结算（占位）—— 无初始化
        break;
    case 10:
        // 查询（占位）—— 无初始化
        break;
    }

    // ---- 淡出 → 切换 → 淡入 ----
    connect(fadeAnimation, &QPropertyAnimation::finished, this, [=]() {
        book->setCurrentIndex(pageIdx);
        fadeAnimation->setDirection(QAbstractAnimation::Backward);
        fadeAnimation->start();
        disconnect(fadeAnimation, &QPropertyAnimation::finished, nullptr, nullptr);
    });
    fadeAnimation->setDirection(QAbstractAnimation::Forward);
    fadeAnimation->start();
}


/* ============================================================
 *  stateToPageIndex — 将 state 编号映射到 page[] 索引
 *
 *  映射表：
 *    state 0 → 0
 *    state 1 → 1
 *    state 2 → 2
 *    state 3 → 3
 *    state 4 → 4
 *    state 9 → 5
 *    state 10 → 6
 * ============================================================ */
int MainWindow::stateToPageIndex(int n) {
    if (n >= 0 && n <= 4) return n;      // state 0~4 直映射
    if (n == 9)  return 5;
    if (n == 10) return 6;
    return 0;   // 容错
}


/* ============================================================
 *  onSearchInputChanged — 检索输入变化处理
 *
 *  逻辑：
 *    当用户在车牌号输入框中按 Enter 或点击输入框外部时，
 *    触发此方法。如果车牌号为空则显示提示，否则执行检索。
 * ============================================================ */
void MainWindow::onSearchInputChanged() {
    QString plate = editSearchPlate->text().trimmed();

    if (plate.isEmpty()) {
        clearSearchResults();
        QLabel *hintLabel = new QLabel("请输入车辆相关信息！");
        hintLabel->setAlignment(Qt::AlignCenter);
        hintLabel->setStyleSheet("font-size: 16px; color: #999; background: transparent;");
        layoutSearchResult->addWidget(hintLabel);
        m_resultRows.append(hintLabel);
        return;
    }

    // ---- 执行检索 ----
    doSearch();
}


/* ============================================================
 *  doSearch — 执行检索并处理结果
 *
 *  根据当前输入的车牌号，调用 basedataapi 检索 car.db。
 *  如果找到对应车辆，再根据 customer_id 查 cus.db 获取车主信息，
 *  在右侧显示带"确定"按钮的结果列表；
 *  如果没找到，自动跳转到 state=2 并预填充车牌号。
 * ============================================================ */
void MainWindow::doSearch() {
    QString plate = editSearchPlate->text().trimmed();

    // 清空旧结果
    clearSearchResults();

    // ---- 按车牌号检索车辆 ----
    QVector<QStringList>* carResults = basedataapi::getInstance().inquireCar(1, plate);
    m_searchPlate = plate;

    bool hasCarRes = (carResults != nullptr && !carResults->isEmpty());

    if (!hasCarRes) {
        // ---- 无匹配结果 → 跳转到 state=2（首次入库登记） ----
        setState(2);
        return;
    }

    // ---- 有匹配结果 → 在右侧显示条目，每个条目后带"确定"按钮 ----
    for (const QStringList& row : *carResults) {
        // car row: [customer_id, license_plate, vin, engine_number, purchase_date, inspection_date, insurance_date]
        int customerId = row[0].toInt();
        QString carPlate = row[1];
        QString carVin   = row[2];

        // 根据 customer_id 查 cus.db 获取车主信息
        QString ownerName  = "未知";
        QString ownerPhone = "未知";
        // 查询全部车主，按 ID 匹配（cus row: [id, owner_name, owner_phone]）
        QVector<QStringList>* allCus = basedataapi::getInstance().inquireCus();
        if (allCus != nullptr) {
            for (const QStringList& cusRow : *allCus) {
                if (cusRow[0].toInt() == customerId) {
                    ownerName  = cusRow[1].isEmpty() ? "无" : cusRow[1];
                    ownerPhone = cusRow[2].isEmpty() ? "无" : cusRow[2];
                    break;
                }
            }
        }

        QFrame *rowFrame = new QFrame;
        rowFrame->setFrameShape(QFrame::StyledPanel);
        QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
        rowLayout->setContentsMargins(8, 4, 8, 4);

        QLabel *infoLabel = new QLabel(
            QString("车主：%1    车牌：%2    电话：%3").arg(ownerName, carPlate, ownerPhone)
        );
        infoLabel->setStyleSheet("font-size: 14px; color: #333; background: transparent;");

        QPushButton *btnConfirm = new QPushButton("确定");
        btnConfirm->setFixedWidth(60);

        // 点击"确定"时，记录选中信息并跳转到 state=3
        connect(btnConfirm, &QPushButton::clicked, this, [=]() {
            m_selectedPlate      = carPlate;
            m_selectedVin        = carVin;
            m_selectedOwnerName  = ownerName;
            m_selectedOwnerPhone = ownerPhone;
            setState(3);
        });

        rowLayout->addWidget(infoLabel, 1);
        rowLayout->addWidget(btnConfirm);
        layoutSearchResult->addWidget(rowFrame);
        m_resultRows.append(rowFrame);
    }

    // 底部弹性空间
    layoutSearchResult->addStretch();
}


/* ============================================================
 *  clearSearchResults — 清除检索结果列表中的动态控件
 *
 *  遍历 m_resultRows，从 layout 中移除并 delete，
 *  然后清空 m_resultRows。
 * ============================================================ */
void MainWindow::clearSearchResults() {
    // 先删除所有动态创建的 widget（QWidget 析构时会自动从 layout 解除）
    for (QWidget* w : m_resultRows) {
        w->deleteLater();   // 延迟删除，避免与 layout item 产生冲突
    }
    m_resultRows.clear();

    // 清理 layout 中所有残留的 item（stretch、spacer 等非 widget 项）
    QLayoutItem *child;
    while ((child = layoutSearchResult->takeAt(0)) != nullptr) {
        delete child;
    }
}


/* ============================================================
 *  ~MainWindow — 析构
 * ============================================================ */
MainWindow::~MainWindow() {
    delete ui;
}
