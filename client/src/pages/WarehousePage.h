#ifndef WAREHOUSEPAGE_H
#define WAREHOUSEPAGE_H

#include <QWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTableWidget>
#include <QStandardItemModel>
#include <QJsonArray>
#include "remote/RemoteModel.h"
#include "widgets/SearchCompleter.h"

// 采购入库 - 批次清单中的单项
struct PurchaseItem {
    QString partNo;
    QString partName;
    QString spec;
    QString supplier;
    QString applicableModel;   // 适用车型（选填，参与备件合并）
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
    // 材料结算：保存单价编辑并重算/写回 material_fee
    void saveBillingPriceEdits();
    // 材料结算：按「锁定工单 + 工单状态」控制 确认提单/取消提单 按钮显示
    void updateBillButtons(int workorderId, const QString &status);
    // 采购入库批次清单
    void refreshPurchaseList();
    // 构建单条备件入库的事务步骤（写入由 4s-server 事务命令原子执行）
    bool buildPurchaseInboundSteps(const PurchaseItem &item, QJsonArray &steps, int idx);

    // 工具函数
    /// 工单搜索（按工单号/车牌模糊搜索，可指定状态过滤）：
    /// 无论 1 条还是多条都在输入框下方展开下拉，仅在用户主动选择时回填；
    /// 无匹配时收起下拉（不弹"未找到"）
    void showWorkOrderSearchPopup(QLineEdit *targetField, SearchCompleter *completer,
                                  const QString &statusFilter = "已派工");
    /// 备件领取：刷新锁定工单的状态栏（车牌号）
    void updateIssueOrderStatus();
    /// 备件退库：锁定工单ID + 刷新状态栏（车牌号）
    void updateReturnOrderStatus();
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
    SearchCompleter *m_issueWoCompleter;  // 备件领取工单多结果下拉
    QLineEdit *m_issuePartSearch;
    QPushButton *m_btnIssueSearch;
    QTableView *m_issueTable;
    RemoteModel *m_issueModel;
    QLabel *m_lblIssuePartInfo;
    QSpinBox *m_spinIssueQty;
    QPushButton *m_btnIssue;
    QLabel *m_issueStatusBar;   // 状态栏：显示锁定工单号和车牌号
    int m_issuePartId;          // 选中的备件目录ID (t_parts.id)

    // ==================== Tab 1: 材料结算/提单 (Stage 3) ====================
    QWidget *m_tabBilling;
    QLineEdit *m_billingOrderNo;
    SearchCompleter *m_billingWoCompleter; // 结算提单工单多结果下拉
    QList<QStringList> m_woRows;           // 工单搜索结果（工单号/状态/车牌/报修内容/创建时间）
    QLabel *m_lblBillingInfo;
    QTableWidget *m_billingTable;   // 材料明细（单价可编辑）
    QLabel *m_lblBillingTotal;
    QPushButton *m_btnConfirmBill;
    QPushButton *m_btnCancelBill;
    int m_billingOrderId;

    // ==================== Tab 2: 采购入库（按批进货） ====================
    QWidget *m_tabPurchase;
    QLineEdit *m_purPartSearch;     // 模糊搜索输入（位于输入区域内）
    QPushButton *m_btnPurSearch;
    SearchCompleter *m_purCompleter; // 采购入库备件多结果下拉
    QList<QStringList> m_purRows;    // 备件搜索结果（no,name,spec,supplier）
    QTableView *m_purTable;         // 显示本批入库清单
    QStandardItemModel *m_purModel;
    QLabel *m_lblPurTotal;          // 清单合计
    QPushButton *m_btnPurRemoveItem; // 从清单移除选中
    QLineEdit *m_purPartNo;
    QLineEdit *m_purPartName;
    QLineEdit *m_purSpec;
    QLineEdit *m_purSupplier;
    QLineEdit *m_purApplicableModel; // 适用车型（选填）
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
    RemoteModel *m_stockModel;

    // ==================== Tab 4: 备件退库 ====================
    QWidget *m_tabReturn;
    QLineEdit *m_retOrderNo;
    QLineEdit *m_retPartSearch;
    SearchCompleter *m_retWoCompleter; // 备件退库工单多结果下拉
    QPushButton *m_btnRetSearch;
    QTableView *m_retTable;
    RemoteModel *m_retModel;
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
    RemoteModel *m_purRetModel;
    QSpinBox *m_purRetQty;
    QPushButton *m_btnPurRetConfirm;
    int m_purRetPartId;         // 选中的备件目录ID
};

#endif // WAREHOUSEPAGE_H
