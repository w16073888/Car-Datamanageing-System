#ifndef PURCHASEPAGE_H
#define PURCHASEPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

class PurchasePage : public QWidget
{
    Q_OBJECT

public:
    explicit PurchasePage(QWidget *parent = nullptr);
    ~PurchasePage();
    void refreshData();

private slots:
    void onSave();

private:
    void setupUI();
    void loadApplicableModelOptions();

    QLineEdit *m_editPartNo;
    QLineEdit *m_editName;
    QLineEdit *m_editSpec;
    QSpinBox *m_spinQuantity;
    QDoubleSpinBox *m_spinPurchasePrice;
    QDoubleSpinBox *m_spinSalePrice;
    QLineEdit *m_editSupplier;
    QLineEdit *m_editWarranty;
    QComboBox *m_cmbApplicableModel;
    QPushButton *m_btnSave;
};

#endif // PURCHASEPAGE_H
