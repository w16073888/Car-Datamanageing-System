#ifndef EXPORTPAGE_H
#define EXPORTPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

class ExportPage : public QWidget
{
    Q_OBJECT

public:
    explicit ExportPage(QWidget *parent = nullptr);
    ~ExportPage();

private slots:
    void onExportCustomer();
    void onExportVehicle();

private:
    void setupUI();

    QCheckBox *m_chkCustomer;
    QCheckBox *m_chkVehicle;
    QPushButton *m_btnExport;
    QLabel *m_lblStatus;
};

#endif // EXPORTPAGE_H
