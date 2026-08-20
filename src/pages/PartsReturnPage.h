#ifndef PARTSRETURNPAGE_H
#define PARTSRETURNPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableView>
#include <QSqlQueryModel>

class PartsReturnPage : public QWidget
{
    Q_OBJECT

public:
    explicit PartsReturnPage(QWidget *parent = nullptr);
    ~PartsReturnPage();

private slots:
    void onLoadOutRecords();
    void onReturnPart();

private:
    void setupUI();

    QLineEdit *m_editOrderNo;
    QPushButton *m_btnLoad;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;

    QLabel *m_lblSelectedPart;
    QSpinBox *m_spinReturnQty;
    QPushButton *m_btnReturn;

    int m_selectedItemId = 0;
    int m_selectedPartId = 0;
    int m_maxReturnQty = 0;
};

#endif // PARTSRETURNPAGE_H
