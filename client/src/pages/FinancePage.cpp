#include "FinancePage.h"
#include "remote/RemoteQuery.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QShowEvent>
#include <QDateTime>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>

// ============================================================
// 辅助：创建带样式的"指标卡片"（标题 + 大数值）
// ============================================================
static QWidget *createStatCard(const QString &title, QLabel **outValueLabel,
                               const QString &valueColor = "#2c3e50",
                               QWidget *parent = nullptr)
{
    QWidget *card = new QWidget(parent);
    card->setStyleSheet(
        "QWidget { background: #ffffff; border: 1px solid #e0e0e0; "
        "border-radius: 8px; }");
    card->setMinimumSize(180, 90);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(4);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 12px; color: #7f8c8d; border: none;");
    layout->addWidget(titleLabel);

    QLabel *valueLabel = new QLabel("—");
    valueLabel->setStyleSheet(
        QString("font-size: 22px; font-weight: bold; color: %1; border: none;")
            .arg(valueColor));
    layout->addWidget(valueLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

    *outValueLabel = valueLabel;
    return card;
}

FinancePage::FinancePage(QWidget *parent)
    : QWidget(parent), m_tabWidget(nullptr), m_chartView(nullptr), m_chart(nullptr)
{
    setupUI();
}

FinancePage::~FinancePage() {}

// ============================================================
// 每次切换到本页面时，自动刷新所有数据
// ============================================================
void FinancePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshIncome();
    refreshProfit();
}

void FinancePage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 12, 20, 12);
    mainLayout->setSpacing(12);

    // ---- 标题 ----
    QLabel *title = new QLabel("财务管理");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(title);

    // ---- 时间段选择 ----
    m_dateRange = new DateRangeWidget;
    mainLayout->addWidget(m_dateRange);

    // ============================================================
    // Tab 页
    // ============================================================
    m_tabWidget = new QTabWidget;

    // ======================== Tab 1: 收入统计 ========================
    QWidget *incomeTab = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(incomeTab);
        layout->setContentsMargins(0, 8, 0, 0);
        layout->setSpacing(16);

        // ---- 收入明细 ----
        QLabel *incomeHeader = new QLabel("📊 收入明细");
        incomeHeader->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        layout->addWidget(incomeHeader);

        QHBoxLayout *row1 = new QHBoxLayout;
        row1->setSpacing(16);
        row1->addWidget(createStatCard("工时费收入", &m_lblIncomeLabor, "#3498db"));
        row1->addWidget(createStatCard("材料费收入", &m_lblIncomeMaterial, "#27ae60"));
        row1->addWidget(createStatCard("其它收入\n(其它费+管理费)", &m_lblIncomeOther, "#e67e22"));
        row1->addWidget(createStatCard("总收入合计", &m_lblIncomeTotal, "#e74c3c"));
        layout->addLayout(row1);

        QHBoxLayout *row1b = new QHBoxLayout;
        row1b->setSpacing(16);
        row1b->addWidget(createStatCard("结算工单数", &m_lblIncomeOrderCount, "#8e44ad"));
        row1b->addStretch(1);
        layout->addLayout(row1b);

        // ---- 分隔 ----
        QFrame *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("QFrame { color: #dcdde1; }");
        layout->addWidget(sep);

        // ---- 成本与利润 ----
        QLabel *profitHeader = new QLabel("💰 成本与利润");
        profitHeader->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        layout->addWidget(profitHeader);

        QHBoxLayout *row2 = new QHBoxLayout;
        row2->setSpacing(16);
        row2->addWidget(createStatCard("进货成本", &m_lblIncomeCost, "#c0392b"));
        row2->addWidget(createStatCard("总利润\n(收入 − 成本)", &m_lblIncomeProfit, "#27ae60"));
        row2->addStretch(1);
        layout->addLayout(row2);

        layout->addStretch();
    }
    m_tabWidget->addTab(incomeTab, "📊 收入统计");

    // ======================== Tab 2: 利润分析 ========================
    QWidget *profitTab = new QWidget;
    {
        QVBoxLayout *layout = new QVBoxLayout(profitTab);
        layout->setContentsMargins(0, 8, 0, 0);
        layout->setSpacing(14);

        // 指标行
        QHBoxLayout *row1 = new QHBoxLayout;
        row1->setSpacing(16);
        row1->addWidget(createStatCard("总收入", &m_lblProfitIncome, "#3498db"));
        row1->addWidget(createStatCard("总成本", &m_lblProfitCost, "#e67e22"));
        row1->addWidget(createStatCard("净利润", &m_lblProfitNet, "#27ae60"));
        row1->addWidget(createStatCard("利润率", &m_lblProfitRate, "#e74c3c"));
        layout->addLayout(row1);

        // 图表（放在滚动区域中）
        m_chart = new QChart();
        m_chart->setTitle("月度收入 / 成本 / 利润趋势");
        m_chart->setAnimationOptions(QChart::SeriesAnimations);
        m_chart->legend()->setVisible(true);
        m_chart->legend()->setAlignment(Qt::AlignBottom);
        m_chart->setBackgroundBrush(QBrush(QColor("#ffffff")));

        m_chartView = new QChartView(m_chart);
        m_chartView->setRenderHint(QPainter::Antialiasing);
        m_chartView->setMinimumHeight(280);
        layout->addWidget(m_chartView, 1);
    }
    m_tabWidget->addTab(profitTab, "📈 利润分析");

    mainLayout->addWidget(m_tabWidget, 1);

    // ---- 信号连接 ----
    connect(m_dateRange, &DateRangeWidget::dateRangeChanged,
            this, &FinancePage::onDateRangeChanged);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &FinancePage::onTabChanged);

    // 初始加载
    refreshIncome();
    refreshProfit();
}

// ============================================================
// 时间段变更 → 刷新当前 Tab
// ============================================================
void FinancePage::onDateRangeChanged(const QDate &start, const QDate &end)
{
    Q_UNUSED(start)
    Q_UNUSED(end)
    int idx = m_tabWidget->currentIndex();
    switch (idx) {
    case 0: refreshIncome(); break;
    case 1: refreshProfit(); break;
    }
}

// ============================================================
// 切换 Tab → 刷新对应数据
// ============================================================
void FinancePage::onTabChanged(int index)
{
    switch (index) {
    case 0: refreshIncome(); break;
    case 1: refreshProfit(); break;
    }
}

// ============================================================
// 1. 收入统计（四项数据 + 工单数）
// ============================================================
void FinancePage::refreshIncome()
{
    QDate start = m_dateRange->startDate();
    QDate end   = m_dateRange->endDate();

    // --- 从 t_settlement 统计工时费、材料费、总收入、结算单数 ---
    double totalIncome = 0;
    {
        RemoteQuery q;
        q.prepare(
            "SELECT "
            "  COUNT(*), "
            "  COALESCE(SUM(s.labor_fee), 0), "
            "  COALESCE(SUM(s.material_fee), 0), "
            "  COALESCE(SUM(s.total_amount), 0) "
            "FROM t_settlement s "
            "WHERE DATE(s.settled_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();

        if (q.next()) {
            int    orderCount = q.value(0).toInt();
            double laborFee   = q.value(1).toDouble();
            double materialFee = q.value(2).toDouble();
            totalIncome = q.value(3).toDouble();

            m_lblIncomeOrderCount->setText(QString::number(orderCount));
            m_lblIncomeLabor->setText(QString("¥%1").arg(laborFee, 0, 'f', 2));
            m_lblIncomeMaterial->setText(QString("¥%1").arg(materialFee, 0, 'f', 2));
            m_lblIncomeTotal->setText(QString("¥%1").arg(totalIncome, 0, 'f', 2));
        }
    }

    // --- 从 t_workorder（已结算工单）统计 其它费 + 管理费 ---
    double otherIncome = 0;
    {
        RemoteQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(w.other_fee), 0) + COALESCE(SUM(w.management_fee), 0) "
            "FROM t_workorder w "
            "INNER JOIN t_settlement s ON s.workorder_id = w.id "
            "WHERE DATE(s.settled_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();

        if (q.next()) {
            otherIncome = q.value(0).toDouble();
            m_lblIncomeOther->setText(QString("¥%1").arg(otherIncome, 0, 'f', 2));
        }
    }

    // 总收入 = 结算金额 + 其它费 + 管理费
    totalIncome += otherIncome;
    m_lblIncomeTotal->setText(QString("¥%1").arg(totalIncome, 0, 'f', 2));

    // --- 进货成本：采购入库时记录的实际进价之和 ---
    double purchaseCost = 0;
    {
        RemoteQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(unit_price), 0) "
            "FROM t_inventory_log "
            "WHERE operation_type = '采购入库' "
            "AND DATE(created_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();
        if (q.next()) purchaseCost = q.value(0).toDouble();
    }
    m_lblIncomeCost->setText(QString("¥%1").arg(purchaseCost, 0, 'f', 2));

    // --- 总利润 = 总收入 - 进货成本 ---
    double profit = totalIncome - purchaseCost;
    m_lblIncomeProfit->setText(QString("¥%1").arg(profit, 0, 'f', 2));
}

// ============================================================
// 2. 利润分析
// ============================================================
void FinancePage::refreshProfit()
{
    QDate start = m_dateRange->startDate();
    QDate end   = m_dateRange->endDate();

    // --- 总收入（与收入统计一致：结算金额 + 其它费 + 管理费） ---
    double income = 0;
    {
        RemoteQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(s.total_amount), 0) "
            "FROM t_settlement s "
            "WHERE DATE(s.settled_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();
        if (q.next()) income = q.value(0).toDouble();
    }
    {
        RemoteQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(w.other_fee), 0) + COALESCE(SUM(w.management_fee), 0) "
            "FROM t_workorder w "
            "INNER JOIN t_settlement s ON s.workorder_id = w.id "
            "WHERE DATE(s.settled_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();
        if (q.next()) income += q.value(0).toDouble();
    }

    // --- 总成本（与收入统计一致：采购入库进价之和） ---
    double cost = 0;
    {
        RemoteQuery q;
        q.prepare(
            "SELECT COALESCE(SUM(unit_price), 0) "
            "FROM t_inventory_log "
            "WHERE operation_type = '采购入库' "
            "AND DATE(created_at) BETWEEN :start AND :end");
        q.bindValue(":start", start.toString("yyyy-MM-dd"));
        q.bindValue(":end",   end.toString("yyyy-MM-dd"));
        q.exec();
        if (q.next()) cost = q.value(0).toDouble();
    }

    double profit = income - cost;
    double rate   = income > 0 ? (profit / income) * 100.0 : 0;

    m_lblProfitIncome->setText(QString("¥%1").arg(income, 0, 'f', 2));
    m_lblProfitCost->setText(QString("¥%1").arg(cost, 0, 'f', 2));
    m_lblProfitNet->setText(QString("¥%1").arg(profit, 0, 'f', 2));
    m_lblProfitRate->setText(QString("%1%").arg(rate, 0, 'f', 1));

    // 同步刷新图表
    drawChart();
}

// ============================================================
// 图表：自适应粒度（同月→按天 / 跨月→按月 / 跨年→按年）
// ============================================================
void FinancePage::drawChart()
{
    m_chart->removeAllSeries();
    const auto axes = m_chart->axes();
    for (auto *axis : axes) m_chart->removeAxis(axis);

    QDate start = m_dateRange->startDate();
    QDate end   = m_dateRange->endDate();

    // ---- 判定粒度 ----
    bool sameMonth = (start.year() == end.year() && start.month() == end.month());
    bool sameYear  = (start.year() == end.year());

    enum Granularity { Daily, Monthly, Yearly };
    Granularity gran;
    QString chartTitle;
    if (sameMonth) {
        gran = Daily;
        chartTitle = QString("每日收入 / 成本 / 利润趋势 (%1年%2月)")
                         .arg(start.year()).arg(start.month());
    } else if (sameYear) {
        gran = Monthly;
        chartTitle = QString("月度收入 / 成本 / 利润趋势 (%1年)").arg(start.year());
    } else {
        gran = Yearly;
        chartTitle = QString("年度收入 / 成本 / 利润趋势 (%1 - %2)")
                         .arg(start.year()).arg(end.year());
    }
    m_chart->setTitle(chartTitle);

    // ---- 柱状图 + 折线 ----
    QBarSet *barIncome = new QBarSet("收入");
    QBarSet *barCost   = new QBarSet("成本");
    barIncome->setColor(QColor("#3498db"));
    barCost->setColor(QColor("#e74c3c"));

    QLineSeries *profitLine = new QLineSeries();
    profitLine->setName("利润");
    profitLine->setPen(QPen(QColor("#27ae60"), 2.5));
    profitLine->setPointsVisible(true);
    profitLine->setPointLabelsVisible(true);
    profitLine->setPointLabelsFormat("@yPoint");
    profitLine->setPointLabelsColor(QColor("#27ae60"));
    profitLine->setPointLabelsFont(QFont("Microsoft YaHei", 8));

    QStringList categories;
    if (gran == Daily) {
        // ---- 按天 ----
        QDate d = start;
        while (d <= end) {
            QString dayKey = d.toString("yyyy-MM-dd");
            categories << d.toString("M/d");

            double inc = 0, cst = 0;
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(total_amount),0) FROM t_settlement "
                          "WHERE DATE(settled_at) = :d");
                q.bindValue(":d", dayKey);
                q.exec();
                if (q.next()) inc = q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(w.other_fee),0) + COALESCE(SUM(w.management_fee),0) "
                          "FROM t_workorder w "
                          "INNER JOIN t_settlement s ON s.workorder_id = w.id "
                          "WHERE DATE(s.settled_at) = :d");
                q.bindValue(":d", dayKey);
                q.exec();
                if (q.next()) inc += q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(unit_price), 0) "
                          "FROM t_inventory_log "
                          "WHERE operation_type = '采购入库' "
                          "AND DATE(created_at) = :d");
                q.bindValue(":d", dayKey);
                q.exec();
                if (q.next()) cst = q.value(0).toDouble();
            }

            *barIncome << inc;
            *barCost   << cst;
            profitLine->append(categories.size() - 1, inc - cst);
            d = d.addDays(1);
        }
    } else if (gran == Monthly) {
        // ---- 按月 ----
        QDate d = QDate(start.year(), start.month(), 1);
        QDate endMonth = QDate(end.year(), end.month(), 1);
        while (d <= endMonth) {
            QString monthKey = d.toString("yyyy-MM");
            categories << d.toString("M月");

            double inc = 0, cst = 0;
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(total_amount),0) FROM t_settlement "
                          "WHERE DATE_FORMAT(settled_at,'%Y-%m') = :m");
                q.bindValue(":m", monthKey);
                q.exec();
                if (q.next()) inc = q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(w.other_fee),0) + COALESCE(SUM(w.management_fee),0) "
                          "FROM t_workorder w "
                          "INNER JOIN t_settlement s ON s.workorder_id = w.id "
                          "WHERE DATE_FORMAT(s.settled_at,'%Y-%m') = :m");
                q.bindValue(":m", monthKey);
                q.exec();
                if (q.next()) inc += q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(unit_price), 0) "
                          "FROM t_inventory_log "
                          "WHERE operation_type = '采购入库' "
                          "AND DATE_FORMAT(created_at,'%Y-%m') = :m");
                q.bindValue(":m", monthKey);
                q.exec();
                if (q.next()) cst = q.value(0).toDouble();
            }

            *barIncome << inc;
            *barCost   << cst;
            profitLine->append(categories.size() - 1, inc - cst);
            d = d.addMonths(1);
        }
    } else {
        // ---- 按年 ----
        for (int y = start.year(); y <= end.year(); ++y) {
            categories << QString::number(y);

            double inc = 0, cst = 0;
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(total_amount),0) FROM t_settlement "
                          "WHERE YEAR(settled_at) = :y");
                q.bindValue(":y", y);
                q.exec();
                if (q.next()) inc = q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(w.other_fee),0) + COALESCE(SUM(w.management_fee),0) "
                          "FROM t_workorder w "
                          "INNER JOIN t_settlement s ON s.workorder_id = w.id "
                          "WHERE YEAR(s.settled_at) = :y");
                q.bindValue(":y", y);
                q.exec();
                if (q.next()) inc += q.value(0).toDouble();
            }
            {
                RemoteQuery q;
                q.prepare("SELECT COALESCE(SUM(unit_price), 0) "
                          "FROM t_inventory_log "
                          "WHERE operation_type = '采购入库' "
                          "AND YEAR(created_at) = :y");
                q.bindValue(":y", y);
                q.exec();
                if (q.next()) cst = q.value(0).toDouble();
            }

            *barIncome << inc;
            *barCost   << cst;
            profitLine->append(categories.size() - 1, inc - cst);
        }
    }

    // ---- 系列挂载 ----
    QBarSeries *barSeries = new QBarSeries();
    barSeries->append(barIncome);
    barSeries->append(barCost);
    m_chart->addSeries(barSeries);
    m_chart->addSeries(profitLine);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setGridLineVisible(false);
    // 按天时如果日期太多，缩小标签字号
    if (gran == Daily && categories.size() > 14)
        axisX->setLabelsFont(QFont("Microsoft YaHei", 7));
    m_chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    profitLine->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("金额 (¥)");
    axisY->setLabelFormat("%.0f");
    axisY->setGridLineColor(QColor("#ecf0f1"));

    // 根据实际数据计算 Y 轴范围，避免条形被截断或负值不可见
    double maxVal = 0, minVal = 0;
    for (int i = 0; i < barIncome->count(); ++i) {
        double v = barIncome->at(i);
        if (v > maxVal) maxVal = v;
        if (v < minVal) minVal = v;
    }
    for (int i = 0; i < barCost->count(); ++i) {
        double v = barCost->at(i);
        if (v > maxVal) maxVal = v;
        if (v < minVal) minVal = v;
    }
    for (int i = 0; i < profitLine->count(); ++i) {
        double v = profitLine->at(i).y();
        if (v > maxVal) maxVal = v;
        if (v < minVal) minVal = v;
    }
    double margin = (maxVal - minVal) * 0.15;
    if (margin < 1) margin = 10;
    axisY->setRange(minVal - margin, maxVal > 0 ? maxVal + margin : 100);

    m_chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    profitLine->attachAxis(axisY);

}
