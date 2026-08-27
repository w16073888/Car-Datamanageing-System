#ifndef SETTLEMENTEDITPAGE_H
#define SETTLEMENTEDITPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QStyledItemDelegate>
#include <QList>

#include "widgets/SearchCompleter.h"
#include "utils/PrintUtil.h"

class QComboBox;
class QButtonGroup;

// ============================================================
// 类别下拉编辑代理（工时条目：机电/钣金/喷漆）
// ============================================================
class LaborTypeDelegate : public QStyledItemDelegate
{
public:
    explicit LaborTypeDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};

// ============================================================
// 金额列显示代理：单元格存原始数值，显示时加 ¥ 前缀
// ============================================================
class MoneyDelegate : public QStyledItemDelegate
{
public:
    explicit MoneyDelegate(QObject *parent = nullptr);
    QString displayText(const QVariant &value, const QLocale &locale) const override;
};

// ============================================================
// 前台业务-结算修改页
//   模糊查询所有「已结算」工单，修改工时条目/费用、备件数量/售价、
//   优惠/管理费/其他费。修改结果只写入 overlay 表
//   （t_settlement_edit / _repair / _item），绝不覆盖原结算数据，
//   仅在本页显示修改版；其他入口一律读原表。
// ============================================================
class SettlementEditPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettlementEditPage(QWidget *parent = nullptr);
    void refreshData();

private slots:
    void onOrderSearch();
    void onFeeEditChanged();
    void onLaborCellChanged(int row, int column);
    void onPartsCellChanged(int row, int column);
    void onAddLabor();
    void onAddPart();
    void onSaveEdit();
    void onSaveToPdf();
    void onPrintSettlement();

private:
    void setupUI();
    void loadOrderInfo(const QString &orderNo);
    void clearDisplay();
    void setLaborRow(int row, const QString &typ, const QString &person,
                     const QString &content, double fee);
    void setPartRow(int row, int partId, const QString &name, double qty, double price);
    void updateSummary();
    // 新版式结算单分区块（方案B：分区块+QPainter拼版，纵向铺满）
    QList<SettlementSection> buildEditSettlementSections() const;

    // ---- 查询工单 ----
    QLineEdit *m_searchOrder;
    SearchCompleter *m_orderCompleter;   // 工单多结果下拉
    QList<QStringList> m_orderRows;      // 工单搜索结果（工单号/状态/车牌/报修内容/创建时间）

    // ---- 显示区域 ----
    QLabel *m_lblVehicleInfo;            // 车辆 + 车主信息
    QTableWidget *m_laborTable;          // 工时明细表（可编辑）
    QTableWidget *m_partsTable;          // 材料明细表（可编辑）
    QTableWidget *m_summaryTable;        // 费用总计表

    // ---- 操作按钮 ----
    QPushButton *m_btnAddLabor;          // 新增工时
    QPushButton *m_btnAddPart;           // 新增材料
    QPushButton *m_btnSaveEdit;          // 保存修改
    QPushButton *m_btnPrint;             // 打印结算单
    QPushButton *m_btnSavePdf;           // 保存到PDF

    // ---- 费用编辑控件 ----
    QDoubleSpinBox *m_editOtherFee;      // 其他费编辑
    QDoubleSpinBox *m_editMgmtFee;       // 管理费编辑
    QDoubleSpinBox *m_editDiscount;      // 优惠金额编辑

    // ---- 行删除按钮组 ----
    QButtonGroup *m_laborBtnGroup;
    QButtonGroup *m_partBtnGroup;

    // ---- 内部状态 ----
    int m_currentOrderId;
    QString m_currentOrderNo;
    bool m_hasOverlay;                   // 当前工单是否已有修改快照(overlay)

    // ---- 车辆信息缓存（打印用）----
    QString m_plate, m_vin, m_model, m_engine;
    QString m_ownerName, m_ownerPhone;
    int m_mileage;
    QString m_entryDate;
};

#endif // SETTLEMENTEDITPAGE_H
