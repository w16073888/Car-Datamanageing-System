#ifndef FRONTDESKPAGE_H
#define FRONTDESKPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QTimer>
#include <QSet>
#include <QScrollArea>
#include <QList>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFileDialog>
#include <QPageSize>
#include <QTableWidget>

#include "widgets/SearchCompleter.h"

class FrontDeskPage : public QWidget
{
    Q_OBJECT

public:
    // 界面状态：查找 → 录入(新车) → 派工
    enum FrontDeskState { STATE_SEARCH, STATE_NEW_CAR, STATE_DISPATCH };

    explicit FrontDeskPage(QWidget *parent = nullptr);
    ~FrontDeskPage();
    void refreshData();
    QString currentOrderNo() const { return m_editOrderNo->text(); }

signals:
    void workOrderCreated(int workorderId, const QString &orderNo);
    void orderNoChanged(const QString &orderNo);

private slots:
    void onVehicleLiveSearch();       // 逐键实时搜索：仅刷新下拉，不自动锁定
    void onVehicleSearchFinalize();   // 失焦/回车：仅当实时搜索无匹配时进入新车录入
    void onLockVehicle();
    void onClearVehicle();
    void onFeeChanged();
    void onCreateWorkOrder();
    void onPrintWorkOrder();
    void onPrintQuote();
    void onPrintSettlement();
    void onShowMaintenanceHistory();
    void onExportQuotePdf();
    void onSaveNewCar();          // 新车录入 → 保存并锁定
    void onCancelNewCar();        // 新车录入 → 返回查找
    void onCancelDispatch();      // 派工 → 返回查找
    void onSaveVehicleInfo();     // 保存车辆信息修改
    void onPartSearchTextChanged(const QString &text); // 备件搜索输入变化
    void onAddPart();             // 添加部件到列表

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void loadCombos();
    QString generateOrderNo();
    void resetForm();
    void fillVehicleData(int vehicleId);
    void clearGhost();
    double calcTotalFee();
    double calcRepairFee();
    QString buildQuoteHtml();
    void setState(FrontDeskState s);
    void refreshCarModelList();   // 刷新新车录入的车型下拉列表
    void resetNewCarForm();       // 重置新车录入表单（每次打开时调用，保证清空）
    bool confirmLockWithPendingOrders(int vehicleId); // 锁定前检查该车辆是否有在派工中的工单
    void lockVehicle(int vehicleId);                  // 锁定车辆（唯一匹配与下拉选择共用）
    void selectPart(const QString &name, const QString &priceRaw); // 回填选中备件

    // 当前状态
    FrontDeskState m_state;

    // ==================== 界面分区（按状态显隐） ====================
    QGroupBox *m_searchGroup;     // 车辆查找
    QGroupBox *m_infoGroup;       // 当前车辆信息
    QGroupBox *m_newGroup;        // 新车录入
    QGroupBox *m_dispatchGroup;   // 派工信息
    QGroupBox *m_repairGroup;     // 报修内容
    QGroupBox *m_feeGroup;        // 费用明细

    // ==================== 多字段搜索锁定区 ====================
    QLineEdit   *m_sPlate, *m_sVin, *m_sEngine;
    QLineEdit   *m_sOwner, *m_sPhone, *m_sModel;
    QPushButton *m_btnLock;
    QPushButton *m_btnMaintenanceHistory;
    QLabel      *m_lblStatus;
    QTimer      *m_searchTimer;
    int          m_lockedVid;
    int          m_foundVid;
    QLineEdit   *m_searchAnchorField;   // 本次车辆搜索触发来源输入框（多结果下拉锚定）
    SearchCompleter *m_vehicleCompleter; // 车辆多结果下拉
    int          m_lastVehicleCount;    // 最近一次实时搜索的匹配数（失焦时据此判断是否进新车录入）
    int          m_mergeTargetWoid;     // 锁定车辆时选择的叠加目标工单ID（0=新建工单）
    QString      m_mergeTargetOrderNo;  // 叠加目标工单号
    QSet<QWidget*> m_ghostFields;

    // ==================== 车辆信息展示（锁定后可编辑） ====================
    QLineEdit *m_dispPlate, *m_dispVin, *m_dispEngine;
    QLineEdit *m_dispModel, *m_dispOwner, *m_dispPhone;
    QLineEdit *m_dispAddress;
    QComboBox *m_dispColor, *m_dispFuel, *m_dispTrans;
    QDateEdit *m_dispPurchase;
    QPushButton *m_btnSaveVehicleInfo;

    // ==================== 新车录入 ====================
    QLineEdit   *m_nPlate, *m_nVin, *m_nEngine;
    QLineEdit   *m_nOwner, *m_nPhone, *m_nAddress;
    QComboBox   *m_nModel;   // 车型：可手动输入的下拉列表（历史车型可选）
    QComboBox   *m_nColor, *m_nFuel, *m_nTrans;
    QDateEdit   *m_nPurchase;
    QPushButton *m_btnSaveNewCar;
    QPushButton *m_btnCancelNewCar;

    // ==================== 派工 ====================
    QLineEdit   *m_editOrderNo;
    QComboBox   *m_cmbAdvisor;
    QSpinBox    *m_spinMileage;
    QTextEdit   *m_textContent;
    QDateEdit   *m_dateRepair, *m_dateEstimated;
    QComboBox   *m_cmbShift;
    QPushButton *m_btnCancelDispatch;

    // ==================== 报修内容（动态行，多列布局） ====================
    struct ItemRow {
        QLineEdit *content;
        QDoubleSpinBox *fee;
        QWidget *container;
        QString type;   // "机电"/"钣金"/"喷漆"
    };
    QList<ItemRow> m_allRows;
    QScrollArea *m_repairScroll;
    QWidget *m_repairColumns;
    QHBoxLayout *m_columnsLayout;
    static const int MAX_ROWS_PER_COLUMN = 4;

    // 三类技工主修人选择器（每种类型一个）
    QComboBox *m_mechTech;   // 机电主修人
    QComboBox *m_bodyTech;   // 钣金主修人
    QComboBox *m_paintTech;  // 喷漆主修人

    void addRepairRow(const QString &type);
    void rebuildRepairLayout();

    // ==================== 预计部件选择区 ====================
    struct SelectedPart {
        QString name;      // 部件名称
        QString spec;      // 型号/规格
        double price;      // 定价
    };
    QList<SelectedPart> m_selectedParts;
    QLineEdit     *m_partSearch;     // 备件模糊搜索输入
    QLineEdit     *m_partPrice;      // 手动输入定价
    SearchCompleter *m_partCompleter; // 备件多结果下拉
    QList<QStringList> m_partRows;    // 备件搜索结果（name,spec,priceDisp,supplier,priceRaw）
    QPushButton   *m_btnAddPart;     // 添加到列表
    QTableWidget  *m_partTable;      // 已选部件列表
    QLabel        *m_lblMatFee;      // 材料费显示（替代 m_spinMat）
    void refreshPartList();          // 刷新已选部件列表显示
    double calcPartTotal() const;    // 计算已选部件总价

    QDoubleSpinBox *m_spinOther, *m_spinMgmt;
    QLabel         *m_lblTotal;
    QLabel         *m_lblFormulaFee;
    QPushButton    *m_btnCreate, *m_btnPrint;
};

#endif // FRONTDESKPAGE_H
