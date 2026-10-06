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
#include <QPair>
#include <QSet>
#include <QPointer>
#include <QElapsedTimer>
#include <QMenu>
#include "remote/RemoteModel.h"
#include "widgets/SearchCompleter.h"

class QEvent;
class QShowEvent;

// 采购入库 - 批次清单中的单项
struct PurchaseItem {
    QString partNo;
    QString partName;
    QString spec;
    QString supplier;
    QString applicableModel;   // 适用车型（选填，参与备件合并）
    double cost = 0.0;
    double price = 0.0;
    double qty = 1.0;          // 支持小数（如 2.5 米 / 1.5 升）
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
    void onViewUsageLog();      // 查看备件去向（各实例 usage_log 字符串列表）

    // 备件退库
    void onReturnSearch();
    void onReturnConfirm();

    // 采购退货（只能操作已领出的备件）
    void onPurchaseReturnSearch();
    void onPurchaseReturnConfirm();

    // Tab 切换自动刷新
    void onTabChanged(int index);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

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
    /// 备件领取：由 m_issueOrderNo 解析工单ID（0=未锁定）
    int issueWorkOrderId() const;
    /// 备件领取：鼠标悬停在「已出库备件」按钮上时，弹出下拉列出本工单已绑定备件
    /// fromClick=true 表示点击触发（未锁定工单时会弹提示；悬停时静默）
    void showBoundPartsMenu(bool fromClick = false);
    /// 备件领取：刷新"本工单已出库备件"ID集合（供销售价列判断可否编辑，避免在
    /// flags() 里查库——那里每个单元格都会被调用，查库会把表格拖垮）
    void refreshIssueBoundParts();
    /// 备件领取：该行备件是否已出库给当前锁定的本工单（决定销售价能否双击改）
    bool issueRowPartBoundToWorkOrder(int row) const;
    /// 备件领取：销售价列提交 → 只改本工单的明细单价，不碰备件目录价
    void onIssuePriceEdited(int row, const QVariant &value);
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
    /// 实例剩余量 = 1 - 该实例已出库且未退库的 t_workorder_item.quantity 合计
    double instanceRemaining(int instanceId) const;
    /// 某备件的在库实例(id, 剩余量)列表，按 id 升序（剩余量>0）
    QList<QPair<int,double>> inStockInstancesWithRemaining(int partId) const;
    /// 事务内追加实例去向记录（usage_log 每行一条，追加式）
    void appendUsageLog(QJsonArray &steps, const QString &instanceRef, const QString &line);
    /// 事务步骤：重算某建档的可出库库存 = 在库实例剩余量之和
    void recalcStock(QJsonArray &steps, const QString &partRef);
    /// 批量更新实例状态
    bool updateInstanceStatus(const QList<int> &instanceIds, const QString &newStatus,
                              int vehicleId = -1, int workorderId = -1,
                              const QString &recipient = QString());

    // ==================== 键盘导航（Tab 选择 + 输入链跳转） ====================
    void installNavFilter(QWidget *w);              // 安装过滤：自旋框一并过滤其内部行编辑
    QList<QWidget*> chainForTab(int index) const;   // 当前 tab 输入链（从左到右、再从上到下）
    QWidget *navWidgetOf(QObject *obj) const;       // 自旋框内部行编辑 → 归一为自旋框本体
    bool handleEnter(QObject *obj);                 // 回车：搜索框→列表 / 列表→下一输入区 / 其它→下一输入区
    bool handleTab(QObject *obj);                   // Tab：结果列表→确认+前进，其它→下一输入区
    bool searchToFirstRow(QWidget *searchField);    // 搜索框回车：有结果跳列表第一行，无结果不跳
    bool confirmRowAndNext(QWidget *table);         // 列表回车：确认选中备件并跳下一输入区
    void focusListFirstRow(QTableView *t);          // 聚焦列表第一行
    void navNext(QWidget *w);                       // 跳到链中下一可见输入区
    void focusWidget(QWidget *w);                   // 聚焦 + 全选
    void enterTab();                                // 进入当前 tab 第一个输入区
    void focusTabBar();                             // 焦点放 tab 栏（进入 tab 选择态）

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
    QDoubleSpinBox *m_spinIssueQty;
    QPushButton *m_btnIssue;
    QLabel *m_issueStatusBar;   // 状态栏：显示锁定工单号和车牌号
    QPushButton *m_btnBoundParts;   // 「本工单已出库备件」：鼠标悬停即弹出下拉
    QPointer<QMenu> m_boundPartsMenu;      // 悬停下拉（开着时不再重复弹出）
    QElapsedTimer   m_boundPartsCooldown;  // 下拉刚关闭后的冷却，避免"关闭→悬停"反复弹
    QSet<int> m_issueBoundPartIds;   // 本工单已出库的备件目录ID（销售价列据此判断可否改）
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
    QDoubleSpinBox *m_purQty;   // 入库数量（支持 3 位小数）
    QPushButton *m_btnPurAddItem;    // 加入清单
    QPushButton *m_btnPurConfirm;    // 确认入库
    QList<PurchaseItem> m_purchaseList; // 本批入库清单

    // ==================== Tab 3: 库存查询 ====================
    QWidget *m_tabStock;
    QLineEdit *m_stockKeyword;
    QPushButton *m_btnStockSearch;
    QPushButton *m_btnStockUsage;   // 查看去向
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
    QDoubleSpinBox *m_retQty;
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
    QDoubleSpinBox *m_purRetQty;   // 退货数量（支持小数）
    QPushButton *m_btnPurRetConfirm;
    int m_purRetPartId;         // 选中的备件目录ID
};

#endif // WAREHOUSEPAGE_H
