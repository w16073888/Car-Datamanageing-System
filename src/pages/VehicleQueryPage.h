#ifndef VEHICLEQUERYPAGE_H
#define VEHICLEQUERYPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTableView>
#include <QSqlQueryModel>
#include <QLabel>

class VehicleQueryPage : public QWidget
{
    Q_OBJECT

public:
    explicit VehicleQueryPage(QWidget *parent = nullptr);
    ~VehicleQueryPage();
    void refreshData();

private slots:
    void onSearch();
    void onDoubleClicked(const QModelIndex &index);

private:
    void setupUI();

    QLineEdit *m_searchInput;
    QComboBox *m_searchType;
    QPushButton *m_btnSearch;
    QTableView *m_tableView;
    QSqlQueryModel *m_model;
    QLabel *m_resultCount;
};

#endif // VEHICLEQUERYPAGE_H
