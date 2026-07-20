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

private:
    void setupUI();
    QString tableName() const;

    QComboBox *m_tableSelector;
    QTableView *m_tableView;
    QSqlTableModel *m_model;
    QPushButton *m_btnRefresh;
    QLabel *m_hintLabel;
};

#endif // DATAMANAGERPAGE_H
