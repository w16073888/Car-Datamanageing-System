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
    void onShowMaintenanceHistory();   // 查看维修历史
    void onExportQuotePdf();           // 导出报价单PDF

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
    void addRepairRow(int catIndex);
    QString buildQuoteHtml();   // 构建报价单HTML内容

    // ==================== 多字段搜索锁定区 ====================
    QLineEdit   *m_sPlate, *m_sVin, *m_sEngine;    // 搜索
    QLineEdit   *m_sOwner, *m_sPhone, *m_sModel;
    QPushButton *m_btnLock, *m_btnUnlock;
    QPushButton *m_btnMaintenanceHistory;     // 维修历史按钮
    QLabel      *m_lblStatus;
    QTimer      *m_searchTimer; // unused now, kept for ABI
    int          m_lockedVid;

    // 搜索结果暂存（选中前）
    int          m_foundVid;        // >0 表示搜索结果
    QSet<QWidget*> m_ghostFields;

    // ==================== 车辆信息展示（锁定后只读） ====================
    QLabel   *m_dispPlate, *m_dispVin, *m_dispEngine;
    QLabel   *m_dispModel, *m_dispOwner, *m_dispPhone;
    QLabel   *m_dispAddress;
    QComboBox   *m_dispColor, *m_dispFuel, *m_dispTrans;
    QLabel      *m_dispPurchase;
    QWidget     *m_infoGroup;

    // ==================== 新车录入 ====================
    QLineEdit   *m_nPlate, *m_nVin, *m_nEngine;
    QLineEdit   *m_nModel, *m_nOwner, *m_nPhone, *m_nAddress;
    QComboBox   *m_nColor, *m_nFuel, *m_nTrans;
    QDateEdit   *m_nPurchase;
    QWidget     *m_newGroup;

    // ==================== 派工 ====================
    QLineEdit   *m_editOrderNo;
    QComboBox   *m_cmbAdvisor, *m_cmbMainTech;
    QSpinBox    *m_spinMileage;
    QTextEdit   *m_textContent;
    QDateEdit   *m_dateRepair, *m_dateEstimated;
    QComboBox   *m_cmbShift;

    // ==================== 报修内容（动态行） ====================
    struct ItemRow {
        QComboBox *tech;
        QLineEdit *content;
        QDoubleSpinBox *fee;
        QWidget *container;
    };
    struct RepairCategory {
        QVBoxLayout *rowsLayout;
        QList<ItemRow> rows;
    };
    RepairCategory m_repairCats[3];

    QDoubleSpinBox *m_spinMat, *m_spinOther, *m_spinMgmt, *m_spinDep;
    QLabel         *m_lblTotal;
    QLabel         *m_lblFormulaFee;
    QPushButton    *m_btnCreate, *m_btnPrint;
};

#endif // FRONTDESKPAGE_H
