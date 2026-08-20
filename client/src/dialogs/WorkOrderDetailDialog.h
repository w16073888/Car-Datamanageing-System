#ifndef WORKORDERDETAILDIALOG_H
#define WORKORDERDETAILDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QTableWidget>

// 工单详情弹窗（业务流水双击打开）
// 展示格式与「前台业务-工单查询」一致，并提供打印 / 保存PDF
class WorkOrderDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorkOrderDetailDialog(const QString &orderNo, QWidget *parent = nullptr);
    ~WorkOrderDetailDialog();

private slots:
    void onPrint();
    void onSavePdf();

private:
    void setupUI();
    void loadDetail(const QString &orderNo);

    QLabel *m_lblInfo;
    QTableWidget *m_laborTable;
    QTableWidget *m_partsTable;
    QTableWidget *m_summaryTable;

    int m_orderId;
    QString m_orderNo;
    QString m_status;
};

#endif // WORKORDERDETAILDIALOG_H
