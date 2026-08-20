#include "DateRangeWidget.h"
#include <QLabel>

DateRangeWidget::DateRangeWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 起始日期
    m_startDate = new QDateEdit;
    m_startDate->setCalendarPopup(true);
    m_startDate->setDisplayFormat("yyyy-MM-dd");
    m_startDate->setDate(QDate::currentDate().addDays(-30));
    layout->addWidget(new QLabel("起始:"));
    layout->addWidget(m_startDate);

    // 结束日期
    m_endDate = new QDateEdit;
    m_endDate->setCalendarPopup(true);
    m_endDate->setDisplayFormat("yyyy-MM-dd");
    m_endDate->setDate(QDate::currentDate());
    layout->addWidget(new QLabel("结束:"));
    layout->addWidget(m_endDate);

    // 快捷按钮
    m_btnToday = new QPushButton("今天");
    m_btnThisMonth = new QPushButton("本月");
    m_btnThisYear = new QPushButton("本年");

    m_btnToday->setFixedWidth(60);
    m_btnThisMonth->setFixedWidth(60);
    m_btnThisYear->setFixedWidth(60);

    layout->addWidget(m_btnToday);
    layout->addWidget(m_btnThisMonth);
    layout->addWidget(m_btnThisYear);
    layout->addStretch();

    // 按钮样式
    QString btnStyle =
        "QPushButton { padding: 4px 8px; border: 1px solid #bdc3c7;"
        "  border-radius: 3px; background-color: #ecf0f1; font-size: 13px; }"
        "QPushButton:hover { background-color: #3498db; color: white; }";
    m_btnToday->setStyleSheet(btnStyle);
    m_btnThisMonth->setStyleSheet(btnStyle);
    m_btnThisYear->setStyleSheet(btnStyle);

    // 连接信号
    connect(m_btnToday, &QPushButton::clicked, this, &DateRangeWidget::onTodayClicked);
    connect(m_btnThisMonth, &QPushButton::clicked, this, &DateRangeWidget::onThisMonthClicked);
    connect(m_btnThisYear, &QPushButton::clicked, this, &DateRangeWidget::onThisYearClicked);
    connect(m_startDate, &QDateEdit::dateChanged, this, &DateRangeWidget::onDateChanged);
    connect(m_endDate, &QDateEdit::dateChanged, this, &DateRangeWidget::onDateChanged);
}

DateRangeWidget::~DateRangeWidget()
{
}

QDate DateRangeWidget::startDate() const
{
    return m_startDate->date();
}

QDate DateRangeWidget::endDate() const
{
    return m_endDate->date();
}

void DateRangeWidget::setDateRange(const QDate &start, const QDate &end)
{
    m_startDate->blockSignals(true);
    m_endDate->blockSignals(true);
    m_startDate->setDate(start);
    m_endDate->setDate(end);
    m_startDate->blockSignals(false);
    m_endDate->blockSignals(false);
}

void DateRangeWidget::onTodayClicked()
{
    QDate today = QDate::currentDate();
    setDateRange(today, today);
    emit dateRangeChanged(today, today);
}

void DateRangeWidget::onThisMonthClicked()
{
    QDate today = QDate::currentDate();
    QDate firstDay(today.year(), today.month(), 1);
    setDateRange(firstDay, today);
    emit dateRangeChanged(firstDay, today);
}

void DateRangeWidget::onThisYearClicked()
{
    QDate today = QDate::currentDate();
    QDate firstDay(today.year(), 1, 1);
    setDateRange(firstDay, today);
    emit dateRangeChanged(firstDay, today);
}

void DateRangeWidget::onDateChanged()
{
    emit dateRangeChanged(m_startDate->date(), m_endDate->date());
}
