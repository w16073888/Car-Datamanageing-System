#ifndef DATAMANAGERPAGE_H
#define DATAMANAGERPAGE_H

#include <QWidget>
#include <QComboBox>
#include <QTableView>
#include <QSqlTableModel>
#include <QPushButton>
#include <QLabel>

class DataManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit DataManagerPage(QWidget *parent = nullptr);
    ~DataManagerPage();
    void refreshData();

private slots:
    void onTableSelected(int index);
    void onCellChanged(int row, int column);
    void onRefresh();
    void onDelete();

private:
    void setupUI();
    QString tableName() const;
    void setChineseHeaders();

    QComboBox *m_tableSelector;
    QTableView *m_tableView;
    QSqlTableModel *m_model;
    QPushButton *m_btnRefresh;
    QPushButton *m_btnDelete;
    QLabel *m_hintLabel;
};

#endif // DATAMANAGERPAGE_H
