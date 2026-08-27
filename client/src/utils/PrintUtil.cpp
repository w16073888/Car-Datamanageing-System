#include "PrintUtil.h"

#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPrintPreviewDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QWidget>
#include <QObject>

// ============================================================
// 统一页面规格：A4 + 8mm 页边距
// ============================================================
void PrintUtil::setupPrinter(QPrinter &printer)
{
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(8, 8, 8, 8), QPageLayout::Millimeter);
}

// ============================================================
// 保存 HTML 为 PDF
// ============================================================
bool PrintUtil::renderHtmlToPdf(const QString &html, const QString &filePath,
                                QWidget *parent)
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    setupPrinter(printer);

    QTextDocument doc;
    doc.setHtml(html);
    doc.setPageSize(printer.pageLayout().paintRectPixels(printer.resolution()).size());
    doc.print(&printer);

    if (QFile::exists(filePath) && QFileInfo(filePath).size() > 0)
        return true;

    QMessageBox::warning(parent, "导出失败",
        QString("保存 PDF 失败：\n%1").arg(filePath));
    return false;
}

// ============================================================
// 打印预览（A4 / 8mm 页边距）
// ============================================================
void PrintUtil::printHtmlPreview(const QString &html, QWidget *parent,
                                 const QString &title)
{
    QPrinter printer;
    setupPrinter(printer);
    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(title);
    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [html](QPrinter *p) {
        QTextDocument doc;
        doc.setHtml(html);
        doc.setPageSize(p->pageLayout().paintRectPixels(p->resolution()).size());
        doc.print(p);
    });
    preview.exec();
}

// ============================================================
// 方案B：分区块 + QPainter 拼版
// ============================================================
// 在 QPrinter 上铺开区块并绘制。pt 坐标：scale 后 translate 到可打印区。
// 与 renderHtmlToPdf 的坐标约定一致（A4 + 8mm）。
static void paintSectionsOnPrinter(QPrinter &printer,
                                   const QList<SettlementSection> &sections)
{
    const int res = printer.resolution();
    const double marginPt = 8.0 * 72.0 / 25.4;   // 8mm → pt
    const double contentW = (210.0 - 16.0) * 72.0 / 25.4;   // 194mm
    const double contentH = (297.0 - 16.0) * 72.0 / 25.4;   // 281mm
    QPainter painter(&printer);
    painter.scale(res / 72.0, res / 72.0);
    painter.translate(marginPt, marginPt);
    PrintUtil::drawSections(painter, sections, QSizeF(contentW, contentH));
}

// 测一个区块 HTML 的自然高度（doc 单位 = pt；pageSize 高度给极大值 = 单页自然高）
double PrintUtil::measureSection(const QString &html, const QSizeF &contentSize)
{
    QTextDocument doc;
    doc.setDocumentMargin(0);
    doc.setHtml(html);
    doc.setPageSize(QSizeF(contentSize.width(), 100000.0));
    return doc.size().height();
}

void PrintUtil::drawSections(QPainter &painter,
                             const QList<SettlementSection> &sections,
                             const QSizeF &contentSize)
{
    // 1. 测每块自然高；grow 块平分多余空间
    QVector<double> natural;
    double sumFixed = 0.0, sumGrow = 0.0;
    int growCount = 0;
    for (int i = 0; i < sections.size(); ++i) {
        double h = measureSection(sections.at(i).html, contentSize);
        natural << h;
        if (sections.at(i).grow) { sumGrow += h; growCount++; }
        else sumFixed += h;
    }
    const double extra = contentSize.height() - sumFixed - sumGrow;
    const double growExtra = (growCount > 0 && extra > 0) ? extra / growCount : 0.0;

    // 2. 逐块绘制（内容过多溢出时 growExtra=0，按自然高排，最后超出被裁剪）
    double y = 0.0;
    for (int i = 0; i < sections.size(); ++i) {
        QTextDocument doc;
        doc.setDocumentMargin(0);
        doc.setHtml(sections.at(i).html);
        doc.setPageSize(QSizeF(contentSize.width(), 100000.0));
        const double h = natural.at(i) + (sections.at(i).grow ? growExtra : 0.0);
        painter.save();
        painter.translate(0, y);
        QAbstractTextDocumentLayout::PaintContext ctx;
        ctx.clip = QRectF(0, 0, contentSize.width(), h);
        doc.documentLayout()->draw(&painter, ctx);
        painter.restore();
        y += h;
    }
}

bool PrintUtil::renderSectionsToPdf(const QList<SettlementSection> &sections,
                                    const QString &filePath, QWidget *parent)
{
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    setupPrinter(printer);
    printer.setFullPage(true);   // 原点=整页左上角，由 paintSectionsOnPrinter translate 到可打印区
    paintSectionsOnPrinter(printer, sections);

    if (QFile::exists(filePath) && QFileInfo(filePath).size() > 0)
        return true;

    QMessageBox::warning(parent, "导出失败",
        QString("保存 PDF 失败：\n%1").arg(filePath));
    return false;
}

void PrintUtil::printSectionsPreview(const QList<SettlementSection> &sections,
                                     QWidget *parent, const QString &title)
{
    QPrinter printer;
    setupPrinter(printer);
    printer.setFullPage(true);
    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle(title);
    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [sections](QPrinter *p) {
        paintSectionsOnPrinter(*p, sections);
    });
    preview.exec();
}

// 结算单新版式共享 CSS（与 _scratch/pdf_render/main.cpp 同一套）
QString PrintUtil::settlementStyle()
{
    return QString(
        "body{font-family:SimSun,'宋体',serif;font-size:12px;margin:0;padding:0;color:#000;}"
        "table{border-collapse:collapse;width:100%;}"
        ".main td{padding:0;border:0;}"
        ".form{border:1px solid #000;}"
        ".form td{border:1px solid #000;padding:5px 8px;vertical-align:middle;}"
        // 页脚表：collapse 会被下方 colspan 行吞掉竖线 → 改用 separate，
        // 每格自己画四边，共享边坐标重合=单线，外框靠单元格自身边框
        ".fbox{border-collapse:separate;border-spacing:0;border:0;}"
        ".fbox td{border:1px solid #000;padding:5px 8px;vertical-align:middle;}"
        ".order-no{text-align:right;font-size:11px;padding:2px 4px;}"
        ".title{text-align:center;font-size:21px;font-weight:bold;letter-spacing:3px;padding:8px 0 14px 0;}"
        ".c{text-align:center;}"
        ".r{text-align:right;}"
        ".amt{text-align:right;font-family:Consolas,'Courier New',monospace;}"
        ".lbl{text-align:left;font-weight:normal;}"
        ".hd{text-align:center;font-weight:bold;}"
        ".sub{font-weight:bold;}"
        ".recv{font-weight:bold;font-size:14px;}"
        ".words{font-weight:bold;letter-spacing:1px;}"
        ".sign{vertical-align:top;}"
        "tr{page-break-inside:avoid;}"
        ".no-split{page-break-inside:avoid;}"
        // 明细区（维修内容/材料清单）：无内部网格，仅顶部实线分隔；
        // 表头与内容之间一条横线（.plain tr.hd td 下边框）
        ".plain{border-top:1px solid #000;}"
        ".plain td{border:0;padding:4px 8px;vertical-align:middle;}"
        ".plain tr.hd td{border-bottom:1px solid #000;}"
        // 明细区上方大标题（维修内容 / 材料清单）
        ".area-title{font-size:16px;font-weight:bold;padding:12px 0 6px 0;}");
}

QString PrintUtil::settlementWrap(const QString &body)
{
    return QString("<!DOCTYPE html><html><head><meta charset='utf-8'><style>%1</style></head><body>%2</body></html>")
        .arg(settlementStyle(), body);
}

// ============================================================
// 人民币大写转换（与各页原有实现保持一致）
// ============================================================
QString PrintUtil::chineseUpper(double amount)
{
    if (amount < 0.005) return QString();
    const QStringList digits  = {"零","壹","贰","叁","肆","伍","陆","柒","捌","玖"};
    const QStringList radices = {"","拾","佰","仟"};
    const int divs[] = {1000, 100, 10, 1};

    qint64 yuan = static_cast<qint64>(amount);
    int jiao = static_cast<int>((amount - yuan) * 100 + 0.5) / 10;
    int fen  = static_cast<int>((amount - yuan) * 100 + 0.5) % 10;

    if (yuan == 0 && jiao == 0 && fen == 0) return "零元整";

    QString result;
    if (yuan > 0) {
        QList<int> segs;
        qint64 t = yuan;
        while (t > 0) { segs.prepend(static_cast<int>(t % 10000)); t /= 10000; }
        for (int s = 0; s < segs.size(); s++) {
            int seg = segs[s];
            if (seg == 0) continue;
            QString part;
            bool needZero = false;
            for (int i = 0; i < 4; i++) {
                int d = (seg / divs[i]) % 10;
                if (d != 0) {
                    if (needZero) { part += "零"; needZero = false; }
                    part += digits[d] + radices[i];
                } else {
                    if (!part.isEmpty()) needZero = true;
                }
            }
            if (part.endsWith("零")) part.chop(1);
            int bigIdx = segs.size() - 1 - s;
            if (bigIdx == 1) part += "万";
            else if (bigIdx == 2) part += "亿";
            result += part;
        }
        result += "元";
    }
    if (jiao > 0) result += digits[jiao] + "角";
    else if (yuan > 0 && fen > 0) result += "零";
    if (fen > 0) result += digits[fen] + "分";
    else if (jiao == 0 && fen == 0) result += "整";

    return result;
}

QString PrintUtil::esc(const QString &s) { return s.toHtmlEscaped(); }

QString PrintUtil::money(double v) { return QString("¥%1").arg(v, 0, 'f', 2); }

QString PrintUtil::intText(int v) { return v > 0 ? QString::number(v) : QString(); }
