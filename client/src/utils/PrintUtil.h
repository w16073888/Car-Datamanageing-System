#ifndef PRINTUTIL_H
#define PRINTUTIL_H

#include <QString>
#include <QList>
#include <QSizeF>

class QPrinter;
class QPainter;
class QWidget;

// 分区块绘制的一「块」：html=完整 HTML（含 <style>）；grow=true 参与纵向填充
struct SettlementSection {
    QString html;
    bool    grow;
};

// ============================================================
// 打印 / PDF 公共工具（结算单、报价单等单据统一走此管线）
//
// 统一页面规格（Qt 渲染 HTML 时 @page 被忽略，页边距由 QPrinter 控制）：
//   - A4 纸张
//   - 8mm 页边距（QPageLayout::Millimeter）
//   - QTextDocument 渲染，setPageSize 取打印机的 paintRectPixels 实际
//     可绘制区域，避免多页时内容错位/溢出
//
// 用法：
//   if (PrintUtil::renderHtmlToPdf(html, filePath, this))
//       QMessageBox::information(...);
//   PrintUtil::printHtmlPreview(html, this, "打印结算单");
//
// 文本/金额格式化与人民币大写转换也集中在这里，各单据共用，避免重复。
// ============================================================
class PrintUtil
{
public:
    // 将 HTML 渲染为 PDF（A4 / 8mm 页边距 / 打印机分辨率）
    // 成功写出返回 true；失败弹窗提示并返回 false。
    static bool renderHtmlToPdf(const QString &html, const QString &filePath,
                                QWidget *parent = nullptr);

    // 弹出打印预览对话框（A4 / 8mm 页边距），可在对话框内选打印机/改设置
    static void printHtmlPreview(const QString &html, QWidget *parent,
                                 const QString &title);

    // ---------- 方案B：分区块拼版（结算单新版式） ----------
    // 背景：QTextDocument 无 CSS flex/"填满剩余高度"能力，纯 HTML 做纵向铺满
    //       只能靠字号/内边距撑高（非线性、脆弱）。改为分区块 + QPainter 拼版：
    //       每块独立 QTextDocument 测自然高，多余空间平分给 grow 块作为 y 间隔，
    //       逐块 documentLayout()->draw。新版式工时/材料两区仅上边框，区间留白
    //       天然呈现"内容铺满"。

    // 测一个区块 HTML 的自然高度（pt；contentSize 提供宽度）
    static double measureSection(const QString &html, const QSizeF &contentSize);

    // 在已按 pt 缩放并对齐到可打印区的 painter 上逐块绘制
    static void drawSections(QPainter &painter,
                             const QList<SettlementSection> &sections,
                             const QSizeF &contentSize);

    // 区块列表 → PDF / 打印预览（A4 + 8mm）
    static bool renderSectionsToPdf(const QList<SettlementSection> &sections,
                                    const QString &filePath, QWidget *parent = nullptr);
    static void printSectionsPreview(const QList<SettlementSection> &sections,
                                     QWidget *parent, const QString &title);

    // 结算单新版式共享 CSS（分区块拼版用，QuotePage/SettlementEditPage 共用）
    static QString settlementStyle();
    // 将区块 body 包装为完整 HTML 文档（套上共享 CSS）
    static QString settlementWrap(const QString &body);

    // 人民币金额 → 中文大写（小于 0.01 返回空串）
    static QString chineseUpper(double amount);

    // HTML 转义（防止客户/配件名含 < > & 等破坏表格）
    static QString esc(const QString &s);

    // 金额格式化：¥xx.xx
    static QString money(double v);

    // 整型显示（0 显示为空，用于里程等可选字段）
    static QString intText(int v);

private:
    // 统一的 QPrinter 页面规格：A4 + 8mm 页边距
    static void setupPrinter(QPrinter &printer);
};

#endif // PRINTUTIL_H
