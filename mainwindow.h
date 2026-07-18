#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateEdit>
#include <QScrollArea>
#include <QFrame>
#include <QPixmap>
#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QStringList>
#include "basedataapi.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void setState(int n);   // 切换到指定页面（state 编号）
    ~MainWindow();

private slots:
    /* 检索界面（state=1）的输入变化处理 —— 查库、显示结果/跳转 */
    void onSearchInputChanged();

private:
    /* ==================== 基础控件 ==================== */
    int state;                      // 当前页面状态码
    Ui::MainWindow *ui;
    QStackedWidget *book;           // 页面栈容器
    QPropertyAnimation *fadeAnimation;
    QGraphicsOpacityEffect *opacotyEffect;

    /* ==================== 页面与布局 ==================== */
    // state→page 映射：0→[0], 1→[1], 2→[2], 3→[3], 4→[4], 9→[5], 10→[6]
    QWidget* page[7];
    QGridLayout *layout[7];

    /* ==================== 页面间传递的数据缓存 ==================== */
    // 从 state=1 检索输入中保存的车牌号（用于预填充 state=2）
    QString m_searchPlate;

    // 当前选中的车辆/车主信息（用于在 state=3 显示）
    QString m_selectedPlate;
    QString m_selectedVin;
    QString m_selectedOwnerName;
    QString m_selectedOwnerPhone;

    /* ==================== State 0 — 主界面 ==================== */
    QPushButton *btnToRepair;       // 保修 → state=1
    QPushButton *btnToWarehouse;    // 库房管理 → state=4
    QPushButton *btnToSettle;       // 结算 → state=9
    QPushButton *btnToQuery;        // 查询 → state=10
    QTextEdit  *textMainHint;       // 右侧提示文本区域（只读）

    /* ==================== State 1 — 保修检索界面 ==================== */
    QLineEdit  *editSearchPlate;        // 车牌号输入
    QScrollArea *scrollSearchResult;    // 右侧结果滚动区
    QWidget    *widgetSearchResult;     // 结果容器
    QVBoxLayout *layoutSearchResult;    // 结果垂直布局
    QVector<QWidget*> m_resultRows;     // 动态生成的"确定"按钮行，方便清理

    /* ==================== State 2 — 车辆首次入库登记界面 ==================== */
    QLineEdit  *editRegPlate;           // 车牌号（必填）
    QLineEdit  *editRegVin;             // 车架号（VIN，选填）
    QLineEdit  *editRegEngine;          // 发动机号（选填）
    QDateEdit  *dateRegPurchase;        // 购车日期（选填）
    QDateEdit  *dateRegInsurance;       // 保险日期（选填）
    QLineEdit  *editRegOwnerName;       // 车主姓名（选填）
    QLineEdit  *editRegOwnerPhone;      // 车主电话（选填）
    QPushButton *btnRegCancel;          // 取消（返回 state=0）
    QPushButton *btnRegSave;            // 保存（存库后跳转 state=3）

    /* ==================== State 3 — 报修界面 ==================== */
    QTextEdit  *textRepairInfo;         // 右侧显示车辆/车主信息
    QLineEdit  *editRepairPerson;       // 维修责任人
    QLineEdit  *editRepairContent;      // 报修内容
    QLineEdit  *editRepairMileage;      // 行驶公里数
    QLineEdit  *editRepairCost;         // 工时费
    QLineEdit  *editRepairDriverName;   // 驾驶员姓名
    QLineEdit  *editRepairDriverPhone;  // 驾驶员电话
    QPushButton *btnRepairSave;         // 保存（存 ser.db → state=0）
    QPushButton *btnRepairCancel;       // 取消（确认弹窗 → state=0）

    /* ==================== State 4 — 库房管理（占位） ==================== */
    QLabel *lblWarehousePlaceholder;

    /* ==================== State 9 — 结算（占位） ==================== */
    QLabel *lblSettlePlaceholder;

    /* ==================== State 10 — 查询（占位） ==================== */
    QLabel *lblQueryPlaceholder;

    /* ==================== 工具方法 ==================== */
    int stateToPageIndex(int n);            // state 编号 → page[] 索引
    void clearSearchResults();              // 清除检索结果列表中的动态控件
    void doSearch();                        // 执行检索（被 onSearchInputChanged 调用）

    /* 日期选择控件的"未设置"标记值 */
    static const QDate UNSET_DATE;
};

#endif // MAINWINDOW_H
