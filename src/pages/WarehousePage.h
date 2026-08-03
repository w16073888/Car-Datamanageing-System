#ifndef WAREHOUSEPAGE_H
#define WAREHOUSEPAGE_H

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSqlQueryModel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTableWidget>
#include <QStandardItemModel>

// 采购入库 - 批次清单中的单项
struct PurchaseItem {
    QString partNo;
    QString partName;
    QString spec;
    QString supplier;
    double cost = 0.0;
    double price = 0.0;
    int    qty = 1;
};

class WarehousePage : public QWidget
{
    Q_OBJECT

public:
    explicit WarehousePage(QWidget *parent = nullptr);
    ~WarehousePage();
    void refreshData();

private slots:
    // 备件领取 (Stage 2)
    void onPartsSearch();
    void onPartsIssue();
    void onIssueOrderSearchTextChanged(const QString &text);

    // 材料结算/提单 (Stage 3)
    void onBillingSearchOrder();
    void onBillingOrderSearchTextChanged(const QString &text);
    void onCompareAndBill();
    void onCancelBill();

    // 采购入库（按批进货）
    void onPurchaseSearch();
    void onPurchaseAddItem();
    void onPurchaseRemoveItem();
    void onPurchaseConfirm();

    // 库存查询
    void onStockSearch();

    // 备件退库
    void onReturnSearch();
    void onReturnConfirm();

    // 采购退货（只能操作已领出的备件）
    void onPurchaseReturnSearch();
    void onPurchaseReturnConfirm();

    // Tab 切换自动刷新
    void onTabChanged(int index);

private:
    void setupUI();
    void loadTechCombos();
    void loadPartCombos();
    // 采购入库批次清单
    void refreshPurchaseList();
    bool doPurchaseInbound(const PurchaseItem &item);

    // 工具函数
    /// 弹出工单搜索选择框（按工单号/车牌模糊搜索，可指定状态过滤）
    bool showWorkOrderSearchPopup(QLineEdit *targetField, const QString &statusFilter = "已派工");
    /// 生成合并查询SQL: 从 t_parts JOIN t_part_instance，按(part_no,name,spec,supplier)分组
    /// spec为空时使用 part_no 生成唯一标识防止误合并
    QString mergedSelectSQL(const QString &extraCols, const QString &whereClause,
                            const QString &groupBy, const QString &orderBy) const;
    /// 生成新实例SN: {part_no}-{序号}
    QString generateInstanceSN(const QString &partNo, int partId) const;
    /// 获取某备件的在库实例ID列表(前N个)
    QList<int> getInStockInstanceIds(int partId, int count) const;
    /// 获取某备件的已领出实例ID列表(前N个)
    QList<int> getCheckedOutInstanceIds(int partId, int count) const;
    /// 批量更新实例状态
    bool updateInstanceStatus(const QList<int> &instanceIds, const QString &newStatus,
                              int vehicleId = -1, int workorderId = -1,
                              const QString &recipient = QString());

    QTabWidget *m_tabWidget;

    // ==================== Tab 0: 备件领取 (Stage 2) ====================
    QWidget *m_tabIssue;
    QLineEdit *m_issueOrderNo;
    QLineEdit *m_issueRecipient;
    QLineEdit *m_issuePartSearch;
    QPushButton *m_btnIssueSearch;
    QTableView *m_issueTable;
    QSqlQueryModel *m_issueModel;
    QLabel *m_lblIssuePartInfo;
    QSpinBox *m_spinIssueQty;
    QPushButton *m_btnIssue;
    QLabel *m_issueStatusBar;   // 状态栏：显示锁定工单号和车牌号
    int m_issuePartId;          // 选中的备件目录ID (t_parts.id)

    // ==================== Tab 1: 材料结算/提单 (Stage 3) ====================
    QWidget *m_tabBilling;
    QLineEdit *m_billingOrderNo;
    QLabel *m_lblBillingInfo;
    QTableView *m_billingTable;
    QSqlQueryModel *m_billingModel;
    QLabel *m_lblBillingTotal;
    QPushButton *m_btnConfirmBill;
    QPushButton *m_btnCancelBill;
    int m_billingOrderId;

    // ==================== Tab 2: 采购入库（按批进货） ====================
    QWidget *m_tabPurchase;
    QLineEdit *m_purPartSearch;     // 模糊搜索输入（位于输入区域内）
    QPushButton *m_btnPurSearch;
    QTableView *m_purTable;         // 显示本批入库清单
    QStandardItemModel *m_purModel;
    QLabel *m_lblPurTotal;          // 清单合计
    QPushButton *m_btnPurRemoveItem; // 从清单移除选中
    QLineEdit *m_purPartNo;
    QLineEdit *m_purPartName;
    QLineEdit *m_purSpec;
    QLineEdit *m_purSupplier;
    QDoubleSpinBox *m_purCost;
    QDoubleSpinBox *m_purPrice;
    QSpinBox *m_purQty;
    QPushButton *m_btnPurAddItem;    // 加入清单
    QPushButton *m_btnPurConfirm;    // 确认入库
    QList<PurchaseItem> m_purchaseList; // 本批入库清单

    // ==================== Tab 3: 库存查询 ====================
    QWidget *m_tabStock;
    QLineEdit *m_stockKeyword;
    QPushButton *m_btnStockSearch;
    QTableView *m_stockTable;
    QSqlQueryModel *m_stockModel;

    // ==================== Tab 4: 备件退库 ====================
    QWidget *m_tabReturn;
    QLineEdit *m_retOrderNo;
    QLineEdit *m_retPartSearch;
    QPushButton *m_btnRetSearch;
    QTableView *m_retTable;
    QSqlQueryModel *m_retModel;
    QSpinBox *m_retQty;
    QPushButton *m_btnRetConfirm;
    QLabel *m_retStatusBar;       // 状态栏：显示锁定工单号和车牌号
    int m_retPartId;              // 选中的备件目录ID
    int m_retLockedWorkOrderId;   // 锁定的工单ID (0=未锁定)

    // ==================== Tab 5: 采购退货 ====================
    QWidget *m_tabPurRet;
    QLineEdit *m_purRetPartSearch;
    QPushButton *m_btnPurRetSearch;
    QTableView *m_purRetTable;
    QSqlQueryModel *m_purRetModel;
    QSpinBox *m_purRetQty;
    QPushButton *m_btnPurRetConfirm;
    int m_purRetPartId;         // 选中的备件目录ID
};

#endif // WAREHOUSEPAGE_H
