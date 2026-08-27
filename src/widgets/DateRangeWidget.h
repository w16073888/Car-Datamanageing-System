#ifndef DATERANGEWIDGET_H
#define DATERANGEWIDGET_H

#include <QWidget>
#include <QDateEdit>
#include <QPushButton>
#include <QDate>
#include <QHBoxLayout>

class DateRangeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DateRangeWidget(QWidget *parent = nullptr);
    ~DateRangeWidget();

    QDate startDate() const;
    QDate endDate() const;
    void setDateRange(const QDate &start, const QDate &end);

signals:
    void dateRangeChanged(const QDate &start, const QDate &end);

private slots:
    void onTodayClicked();
    void onThisMonthClicked();
    void onThisYearClicked();
    void onDateChanged();

private:
    QDateEdit *m_startDate;
    QDateEdit *m_endDate;
    QPushButton *m_btnToday;
    QPushButton *m_btnThisMonth;
    QPushButton *m_btnThisYear;
};

#endif // DATERANGEWIDGET_H
