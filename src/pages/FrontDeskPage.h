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
    void triggerFuzzySearch();
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
    QLineEdit   *m_nModel, *m_nOwner, *m_nPhone, *m_nAddress;
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
    QPushButton   *m_btnAddPart;     // 添加到列表
    QTableWidget  *m_partTable;      // 已选部件列表
    QLabel        *m_lblMatFee;      // 材料费显示（替代 m_spinMat）
    void refreshPartList();          // 刷新已选部件列表显示
    double calcPartTotal() const;    // 计算已选部件总价

    QDoubleSpinBox *m_spinOther, *m_spinMgmt, *m_spinDep;
    QLabel         *m_lblTotal;
    QLabel         *m_lblFormulaFee;
    QPushButton    *m_btnCreate, *m_btnPrint;
};

#endif // FRONTDESKPAGE_H
