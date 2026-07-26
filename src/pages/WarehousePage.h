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

    // 材料结算/提单 (Stage 3)
    void onBillingSearchOrder();
    void onCompareAndBill();

    // 采购入库
    void onPurchaseSearch();
    void onPurchaseConfirm();

    // 库存查询
    void onStockSearch();

    // 备件退库
    void onReturnSearch();
    void onReturnConfirm();

    // 采购退货
    void onPurchaseReturnSearch();
    void onPurchaseReturnConfirm();

private:
    void setupUI();
    void loadTechCombos();
    void loadPartCombos();

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
    int m_issuePartId;

    // ==================== Tab 1: 材料结算/提单 (Stage 3) ====================
    QWidget *m_tabBilling;
    QLineEdit *m_billingOrderNo;
    QPushButton *m_btnBillingSearch;
    QLabel *m_lblBillingInfo;
    QTableView *m_billingTable;
    QSqlQueryModel *m_billingModel;
    QLabel *m_lblBillingTotal;
    QPushButton *m_btnConfirmBill;
    int m_billingOrderId;

    // ==================== Tab 2: 采购入库 ====================
    QWidget *m_tabPurchase;
    QLineEdit *m_purPartSearch;
    QPushButton *m_btnPurSearch;
    QTableView *m_purTable;
    QSqlQueryModel *m_purModel;
    QLineEdit *m_purPartNo;
    QLineEdit *m_purPartName;
    QLineEdit *m_purSpec;
    QLineEdit *m_purSupplier;
    QDoubleSpinBox *m_purCost;
    QDoubleSpinBox *m_purPrice;
    QSpinBox *m_purQty;
    QPushButton *m_btnPurConfirm;
    int m_purPartId;

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
    int m_retPartId;

    // ==================== Tab 5: 采购退货 ====================
    QWidget *m_tabPurRet;
    QLineEdit *m_purRetPartSearch;
    QPushButton *m_btnPurRetSearch;
    QTableView *m_purRetTable;
    QSqlQueryModel *m_purRetModel;
    QSpinBox *m_purRetQty;
    QPushButton *m_btnPurRetConfirm;
    int m_purRetPartId;
};

#endif // WAREHOUSEPAGE_H
