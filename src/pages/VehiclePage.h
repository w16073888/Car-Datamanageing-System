#ifndef VEHICLEPAGE_H
#define VEHICLEPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>

class VehiclePage : public QWidget
{
    Q_OBJECT

public:
    explicit VehiclePage(QWidget *parent = nullptr);
    ~VehiclePage();

    // 预填充车牌号（从查询页跳转过来时使用）
    void setPlateNumber(const QString &plate);

signals:
    void vehicleSaved(const QString &plateNumber);

private slots:
    void onSave();
    void onClear();

private:
    void setupUI();
    bool validateInput();

    // 车辆信息
    QLineEdit *m_editPlate;
    QLineEdit *m_editVin;
    QLineEdit *m_editEngine;
    QDateEdit *m_datePurchase;
    QDateEdit *m_dateInspection;
    QDateEdit *m_dateInsurance;

    // 车主信息
    QLineEdit *m_editOwnerName;
    QLineEdit *m_editOwnerPhone;

    // 驾驶员信息
    QLineEdit *m_editDriverName;
    QLineEdit *m_editDriverPhone;

    QPushButton *m_btnSave;
    QPushButton *m_btnClear;
};

#endif // VEHICLEPAGE_H
