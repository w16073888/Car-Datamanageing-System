#ifndef WORKORDERPAGE_H
#define WORKORDERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSqlQueryModel>

class WorkOrderPage : public QWidget
{
    Q_OBJECT

public:
    explicit WorkOrderPage(QWidget *parent = nullptr);
    ~WorkOrderPage();

private slots:
    void onSelectVehicle();
    void onCreateOrder();
    void onChangeStatus();
    void onOrderSelected(const QModelIndex &index);

private:
    void setupUI();
    void refreshOrderList();
    QString generateOrderNo();

    // 车辆选择区域
    QLineEdit *m_editPlateSearch;
    QPushButton *m_btnSearchVehicle;
    QLabel *m_lblVehicleInfo;

    // 工单信息
    QLineEdit *m_editOrderNo;
    QComboBox *m_cmbTechnician;
    QLineEdit *m_editMileage;
    QTextEdit *m_textRepairContent;
    QLineEdit *m_editLaborFee;
    QLabel *m_lblStatus;

    // 按钮
    QPushButton *m_btnCreate;
    QPushButton *m_btnStartRepair;
    QPushButton *m_btnComplete;
    QPushButton *m_btnRefresh;

    // 工单列表
    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    int m_currentVehicleId = 0;
    int m_currentOrderId = 0;
};

#endif // WORKORDERPAGE_H
