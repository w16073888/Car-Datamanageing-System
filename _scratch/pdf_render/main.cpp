// ============================================================
// 结算单新版式 方案B 渲染验证（与客户端 buildSettlementHtmlFor 同一模板）
// 用真实工单数据（garagedb, workorder id=11, WO20260800008）填充，
// 走与 PrintUtil 相同的 QPrinter A4 / 8mm 管线，输出 PDF + PNG 供目检。
//
// 方案B：分区块 + QPainter 拼版，替代 font-size 撑高校准。
//   6 个区块各自 setHtml → 测自然高 → 按 y 坐标逐块 draw。
//   多余空间 = usableH − 固定区块 − 自然grow，平分给 工时/材料 两区。
//   新版式这两区只有上边框 → 两区之间留白天然是"内容铺满"，无需撑高技巧。
//
// 新版式（2026-08-24，用户6条精进）：
//  1. 全文字号略调大
//  2. 维修内容/材料清单 表头行不画框
//  3. 工时/材料两区无内部网格，两区之间用实线分割；
//     工时区列宽：序号+主修人+工时费 合计 50%，维修项目 50%
//  4. 汇总区仅 材料费/工时费/管理费 + 总费用/总优惠/应收款；
//     应收款加大加粗；下一排 大写金额(3/4) + 客户签字(1/4)
//  5. 地址沉底，默认 成都市双流区航都大街二段370号（C++ 端读"办公地点"配置）
//  6. 内容纵向铺满整页，多余空间平分到工时/材料两区
// ============================================================
#include <QApplication>
#include <QPrinter>
#include <QPainter>
#include <QImage>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// 直接编译客户端的 PrintUtil.cpp：结算单新版式 CSS / 分区块拼版 / PDF 全走真实实现
#include "../../client/src/utils/PrintUtil.cpp"

static QString esc(const QString &s) { return s.toHtmlEscaped(); }
static QString money(double v) { return QString("¥%1").arg(v, 0, 'f', 2); }

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    // A4 8mm 边距 → 可打印区（pt）：194mm x 281mm
    const double contentW = 194.0 * 72.0 / 25.4;   // 549.92
    const double usableH  = 281.0 * 72.0 / 25.4;   // 796.54

    // ---- 工单 11 (WO20260800008) 真实数据 ----
    const QString orderNo   = "WO20260800008";
    const QString owner     = "老子";
    const QString phone     = "198";
    const QString plate     = "川B14250";
    const QString model     = "afaf";
    const QString engine    = "afaerw";
    const QString vin       = "zsgga";
    const QString entryDate = "2026-08-09";
    const int     mileage   = 0;
    const QString address   = "成都市双流区航都大街二段370号";

    // 工时明细（真实：5 行）
    QString laborRows;
    {
        struct R { const char *item; const char *person; const char *content; double fee; };
        const R rows[] = {
            {"喷漆","李昊宇","AF",0.00},
            {"喷漆","李昊宇","AFS",0.00},
            {"机电","李昊宇","DSFSWD",0.00},
            {"钣金","李昊宇","FAFF",0.00},
            {"钣金","李昊宇","",1.00},
        };
        int seq = 0;
        for (const R &r : rows) {
            seq++;
            QString item = QString::fromUtf8(r.item);
            QString content = QString::fromUtf8(r.content);
            QString serviceItem = content.isEmpty() ? item : item + ": " + content;
            laborRows += QString("<tr><td class='c'>%1</td><td>%2</td><td class='c'>%3</td><td class='amt'>%4</td></tr>")
                .arg(seq).arg(esc(serviceItem)).arg(esc(QString::fromUtf8(r.person))).arg(money(r.fee));
        }
    }
    // 材料明细（真实：2 行，order by part_name：机(jy) 在前）
    QString partsRows;
    {
        struct R { const char *no; const char *name; const char *unit; int qty; double price; double sub; };
        const R rows[] = {
            {"jy","机","a",3,200.00,600.00},
            {"sb","煞笔","b",2,5.00,10.00},
        };
        int seq = 0;
        for (const R &r : rows) {
            seq++;
            partsRows += QString("<tr><td class='c'>%1</td><td>%2</td><td>%3</td><td class='c'>%4</td><td class='c'>%5</td><td class='amt'>%6</td><td class='amt'>%7</td></tr>")
                .arg(seq).arg(esc(QString::fromUtf8(r.no))).arg(esc(QString::fromUtf8(r.name)))
                .arg(QString::fromUtf8(r.unit)).arg(r.qty).arg(money(r.price)).arg(money(r.sub));
        }
    }

    double displayLabor = 1.00;      // w.labor_fee=1 >0 → 用 1.00
    double partsTotal   = 610.00;
    double mgmtFee      = 4.00;
    double discount     = 0.00;
    double grandTotal   = displayLabor + partsTotal + mgmtFee - discount; // 615.00
    double receivable   = grandTotal;
    const QString amountInWords = "陆佰壹拾伍元整";

    // ---- 6 个区块 body ----
    QString headHtml =
        "<div class='title'>成都科盟汽车服务有限责任公司维修结算单</div>";

    QString infoHtml =
        "<table class='form' width='100%'>"
        "<tr><td>送修单位：%OWNER%</td><td>联系人：%CONTACT%</td><td>联系电话：%PHONE%</td></tr>"
        "<tr><td>车牌号：%PLATE%</td><td>车型：%MODEL%</td><td>发动机号：%ENGINE%</td></tr>"
        "<tr><td>车身号：%VIN%</td><td>送修日期：%ENTRYDATE%</td><td>里程数：%MILEAGE% km</td></tr>"
        "</table>";
    infoHtml.replace("%OWNER%", esc(owner)).replace("%CONTACT%", esc(owner)).replace("%PHONE%", esc(phone));
    infoHtml.replace("%PLATE%", esc(plate)).replace("%MODEL%", esc(model)).replace("%ENGINE%", esc(engine));
    infoHtml.replace("%VIN%", esc(vin)).replace("%ENTRYDATE%", entryDate).replace("%MILEAGE%", QString::number(mileage));

    // 工时区（grow）：列宽 序号+主修人+工时费=50%，维修项目=50%
    QString laborHtml =
        "<div class='area-title'>维修内容</div>"
        "<table class='plain' width='100%'>"
        "<tr class='hd'>"
        "<td width='16.7%'>序号</td>"
        "<td width='50%'>维修项目</td>"
        "<td width='16.7%'>主修人</td>"
        "<td width='16.6%'>工时费</td>"
        "</tr>"
        "%LABORROWS%"
        "<tr class='sub'><td colspan='3' class='r'>工时费小计</td><td class='amt'>%LABORTOTAL%</td></tr>"
        "</table>";
    laborHtml.replace("%LABORROWS%", laborRows).replace("%LABORTOTAL%", money(displayLabor));

    // 材料区（grow）
    QString partsHtml =
        "<div class='area-title'>材料清单</div>"
        "<table class='plain' width='100%'>"
        "<tr class='hd'>"
        "<td width='9%'>序号</td>"
        "<td width='15%'>配件件号</td>"
        "<td width='22%'>配件名称</td>"
        "<td width='9%'>单位</td>"
        "<td width='9%'>数量</td>"
        "<td width='18%'>单价</td>"
        "<td width='18%'>金额</td>"
        "</tr>"
        "%PARTSROWS%"
        "<tr class='sub'><td colspan='6' class='r'>材料费小计</td><td class='amt'>%PARTSTOTAL%</td></tr>"
        "</table>";
    partsHtml.replace("%PARTSROWS%", partsRows).replace("%PARTSTOTAL%", money(partsTotal));

    // 汇总区（固定）：12列 3项/行各4列；大写金额9列=3/4，客户签字3列=1/4
    QString summaryHtml =
        "<table class='form' width='100%'>"
        "<tr>"
        "<td colspan='4' class='lbl' width='33.3%'>材料费 %MFEE%</td>"
        "<td colspan='4' class='lbl' width='33.3%'>工时费 %LFEE%</td>"
        "<td colspan='4' class='lbl' width='33.4%'>管理费 %MGM%</td>"
        "</tr>"
        "<tr>"
        "<td colspan='4' class='lbl' width='33.3%'>总费用 %GRAND%</td>"
        "<td colspan='4' class='lbl' width='33.3%'>总优惠 %DISCOUNT%</td>"
        "<td colspan='4' class='recv' width='33.4%'>应收款 %RECV%</td>"
        "</tr>"
        "<tr>"
        "<td colspan='9' class='words' width='75%'>大写金额：%WORDS%</td>"
        "<td colspan='3' class='sign' width='25%'>客户签字：</td>"
        "</tr>"
        "</table>";
    summaryHtml.replace("%MFEE%", money(partsTotal)).replace("%LFEE%", money(displayLabor)).replace("%MGM%", money(mgmtFee));
    summaryHtml.replace("%GRAND%", money(grandTotal)).replace("%DISCOUNT%", money(discount)).replace("%RECV%", money(receivable));
    summaryHtml.replace("%WORDS%", amountInWords);

    // 底部区块（固定）：工号（从右上角移至此）+ 地址/服务电话同行 +
    // 结算人/结算日期/收款人签字 三个独立表格；原收款人签字行/出厂日期已并入
    QString servicePhone = "028-________";
    // 地址+服务电话、结算人/结算日期/收款人签字 两行：无表格框线，仅对齐
    QString footerHtml =
        "<div class='order-no'>工号：%ORDER%</div>"
        "<table class='main' width='100%'>"
        "<tr><td width='66.7%'>地址：%ADDRESS%</td><td width='33.3%'>服务电话：%PHONE%</td></tr>"
        "</table>"
        "<table class='main' width='100%'>"
        "<tr>"
        "<td width='33.3%'>结算人：%SETTLER%</td>"
        "<td width='33.3%'>结算日期：%SETTLEDATE%</td>"
        "<td width='33.4%'>收款人签字：%SIGN%</td>"
        "</tr>"
        "</table>";
    footerHtml.replace("%ORDER%", esc(orderNo))
        .replace("%ADDRESS%", esc(address))
        .replace("%PHONE%", esc(servicePhone))
        .replace("%SETTLER%", esc(QString::fromUtf8("演示结算员")))
        .replace("%SETTLEDATE%", QString("2026-08-24"))
        .replace("%SIGN%", QString("________________"));

    // 区块列表：固定区块 + 两个 grow 区块（工时/材料）
    QList<SettlementSection> sections;
    sections << SettlementSection{PrintUtil::settlementWrap(headHtml),   false}
             << SettlementSection{PrintUtil::settlementWrap(infoHtml),   false}
             << SettlementSection{PrintUtil::settlementWrap(laborHtml),  true}
             << SettlementSection{PrintUtil::settlementWrap(partsHtml),  true}
             << SettlementSection{PrintUtil::settlementWrap(summaryHtml),false}
             << SettlementSection{PrintUtil::settlementWrap(footerHtml), false};

    // ---- 测量指标（验证纵向铺满）----
    {
        QFile mf("D:/temmpcode/4s/_scratch/pdf_render/measure.txt");
        if (mf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ms(&mf);
            ms << "contentW=" << contentW << " usableH=" << usableH << "\n";
            double sumFixed = 0.0, sumGrow = 0.0;
            for (int i = 0; i < sections.size(); ++i) {
                double h = PrintUtil::measureSection(sections.at(i).html, QSizeF(contentW, usableH));
                if (sections.at(i).grow) sumGrow += h; else sumFixed += h;
                ms << "  block " << i << " grow=" << sections.at(i).grow
                   << " natural=" << h << "\n";
            }
            double extra = usableH - sumFixed - sumGrow;
            ms << "  sumFixed=" << sumFixed << " sumGrow=" << sumGrow
               << " extra=" << extra
               << " growExtra=" << (extra > 0 ? extra / 2.0 : 0.0) << "\n"
               << "  => " << (extra >= 0 ? "FILL_OK" : "OVERFLOW") << "\n";
            mf.close();
        }
    }

    // ---- 输出 PDF（走客户端 PrintUtil::renderSectionsToPdf）----
    PrintUtil::renderSectionsToPdf(sections,
        "D:/temmpcode/4s/_scratch/pdf_render/settlement_new.pdf", nullptr);

    // ---- 输出 PNG（目检用，3x 便于查看）----
    {
        QImage img((int)(contentW * 3), (int)(usableH * 3), QImage::Format_ARGB32);
        img.fill(Qt::white);
        QPainter p(&img);
        p.scale(3, 3);
        PrintUtil::drawSections(p, sections, QSizeF(contentW, usableH));
        p.end();
        img.save("D:/temmpcode/4s/_scratch/pdf_render/settlement_new.png");

        // 诊断：首/末非白像素行（验证原点与边距）。PNG 坐标=pt*3。
        auto firstLast = [](const QImage &im) {
            int first = -1, last = -1;
            for (int y = 0; y < im.height(); ++y) {
                const QRgb *line = (const QRgb *)im.constScanLine(y);
                bool hit = false;
                for (int x = 0; x < im.width(); ++x) {
                    if (qRed(line[x]) < 245 || qGreen(line[x]) < 245 || qBlue(line[x]) < 245) { hit = true; break; }
                }
                if (hit) { if (first < 0) first = y; last = y; }
            }
            return qMakePair(first, last);
        };
        auto fl = firstLast(img);
        QFile mf("D:/temmpcode/4s/_scratch/pdf_render/measure.txt");
        if (mf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream ms(&mf);
            ms << "[png 3x] firstNonWhiteRow=" << fl.first
               << " lastNonWhiteRow=" << fl.second
               << " imgH=" << img.height()
               << "\n  => 顶留白=" << fl.first / 3.0 << "pt (" << fl.first / 3.0 * 25.4 / 72.0 << "mm)"
               << "  底留白=" << (img.height() - 1 - fl.second) / 3.0 << "pt\n";
            mf.close();
        }
    }

    // ============================================================
    // 报价单新版式（平移 FrontDeskPage::buildQuoteSections 的结构）
    // ============================================================
    {
        const QString quoteOrderNo = "WO20260800008";
        const QString sBrand   = QString::fromUtf8("科盟");
        const QString sColor   = QString::fromUtf8("白色");
        const QString sFuel    = QString::fromUtf8("汽油");
        const QString sTrans   = QString::fromUtf8("自动");
        const QString sPurchase= "2026-08-09";
        const QString ownerName = QString::fromUtf8("老子");
        const QString ownerPhone= "198";
        const QString ownerAddr = QString::fromUtf8("成都市武侯区科华北路99号");
        const QString model    = sBrand + QString::fromUtf8(" afaf");
        const QString techs    = QString::fromUtf8("机电:李昊宇 钣金:王五 喷漆:赵六");
        const QString content  = QString::fromUtf8("客户反映车辆启动困难，需检查电瓶及启动系统。");

        QString qHeadHtml =
            "<div class='title'>成都科盟汽车服务有限责任公司维修报价单</div>";

        QString qVehicleHtml =
            "<div class='area-title'>车辆信息</div>"
            "<table class='form' width='100%'>"
            "<tr><td>车牌号：%PLATE%</td><td>品牌/车型：%MODEL%</td><td>颜 色：%COLOR%</td></tr>"
            "<tr><td>车架号(VIN)：%VIN%</td><td>发动机号：%ENGINE%</td><td>燃油类型：%FUEL%</td></tr>"
            "<tr><td>当前里程：%MILEAGE% km</td><td>购车日期：%PURCHASE%</td><td>变速箱：%TRANS%</td></tr>"
            "</table>";
        qVehicleHtml.replace("%PLATE%", esc(QString::fromUtf8("川B14250")))
            .replace("%MODEL%", esc(model)).replace("%COLOR%", esc(sColor))
            .replace("%VIN%", esc(QString::fromUtf8("zsgga")))
            .replace("%ENGINE%", esc(QString::fromUtf8("afaerw")))
            .replace("%FUEL%", esc(sFuel)).replace("%MILEAGE%", "58000")
            .replace("%PURCHASE%", sPurchase).replace("%TRANS%", esc(sTrans));

        QString qOwnerHtml =
            "<div class='area-title'>车主信息</div>"
            "<table class='form' width='100%'>"
            "<tr><td>姓 名：%OWNER%</td><td>联系电话：%OWNERPHONE%</td><td></td></tr>"
            "<tr><td colspan='3'>地 址：%OWNERADDR%</td></tr>"
            "</table>";
        qOwnerHtml.replace("%OWNER%", esc(ownerName)).replace("%OWNERPHONE%", esc(ownerPhone))
            .replace("%OWNERADDR%", esc(ownerAddr));

        QString qOrderHtml =
            "<div class='area-title'>工单信息</div>"
            "<table class='form' width='100%'>"
            "<tr><td>服务顾问：%ADVISOR%</td><td>主修人：%TECHS%</td><td>班 别：%SHIFT%</td></tr>"
            "<tr><td>报修日期：%REPDATE%</td><td>预估完工：%ESTDATE%</td><td>制单时间：%NOW%</td></tr>"
            "</table>";
        qOrderHtml.replace("%ADVISOR%", esc(QString::fromUtf8("张三")))
            .replace("%TECHS%", esc(techs)).replace("%SHIFT%", esc(QString::fromUtf8("白班")))
            .replace("%REPDATE%", "2026-08-24").replace("%ESTDATE%", "2026-08-25")
            .replace("%NOW%", "2026-08-24 15:30");

        QString qContentHtml =
            "<div class='area-title'>报修描述</div>"
            "<table class='plain' width='100%'><tr><td>%CONTENT%</td></tr></table>";
        qContentHtml.replace("%CONTENT%", esc(content));

        // 维修项目（grow）
        double qLaborTotal = 0;
        QString qLaborRows;
        {
            struct R { const char *type; const char *tech; const char *cont; double fee; };
            const R rows[] = {
                {"机电","李昊宇","更换机油机滤",480.00},
                {"机电","李昊宇","检查电瓶及启动系统",200.00},
                {"钣金","王五","右前门钣金修复",350.00},
                {"喷漆","赵六","右前门喷漆",600.00},
            };
            for (const R &r : rows) {
                qLaborTotal += r.fee;
                qLaborRows += QString("<tr><td class='c'>%1</td><td class='c'>%2</td>"
                                      "<td>%3</td><td class='amt'>%4</td></tr>")
                    .arg(esc(QString::fromUtf8(r.type)))
                    .arg(esc(QString::fromUtf8(r.tech)))
                    .arg(esc(QString::fromUtf8(r.cont)))
                    .arg(money(r.fee));
            }
        }
        QString qLaborHtml =
            "<div class='area-title'>维修项目</div>"
            "<table class='plain' width='100%'>"
            "<tr class='hd'>"
            "<td width='14%'>类别</td>"
            "<td width='16%'>维修人</td>"
            "<td width='48%'>维修内容</td>"
            "<td width='22%'>费用</td>"
            "</tr>"
            "%REPAIRROWS%"
            "<tr class='sub'><td colspan='3' class='r'>工时费小计</td><td class='amt'>%LABORTOTAL%</td></tr>"
            "</table>";
        qLaborHtml.replace("%REPAIRROWS%", qLaborRows).replace("%LABORTOTAL%", money(qLaborTotal));

        // 预计部件（grow）
        double qPartsTotal = 0;
        QString qPartsRows;
        {
            struct R { const char *name; const char *spec; double price; };
            const R rows[] = {
                {"机油滤芯","JX0806",35.00},
                {"全合成机油 5W-40","4L",380.00},
                {"前刹车片","EPB-100",290.00},
            };
            for (const R &r : rows) {
                qPartsTotal += r.price;
                qPartsRows += QString("<tr><td>%1</td><td>%2</td><td class='amt'>%3</td></tr>")
                    .arg(esc(QString::fromUtf8(r.name)))
                    .arg(esc(QString::fromUtf8(r.spec)))
                    .arg(money(r.price));
            }
        }
        QString qPartsHtml =
            "<div class='area-title'>预计部件</div>"
            "<table class='plain' width='100%'>"
            "<tr class='hd'>"
            "<td width='40%'>部件名称</td>"
            "<td width='30%'>型号</td>"
            "<td width='30%'>单价</td>"
            "</tr>"
            "%PARTSROWS%"
            "<tr class='sub'><td colspan='2' class='r'>部件费小计</td><td class='amt'>%PARTSTOTAL%</td></tr>"
            "</table>";
        qPartsHtml.replace("%PARTSROWS%", qPartsRows).replace("%PARTSTOTAL%", money(qPartsTotal));

        // 费用汇总（12列）
        double qOth = 0, qMgmt = 40.00;
        double qTotal = qLaborTotal + qPartsTotal + qOth + qMgmt;
        QString qSummaryHtml =
            "<table class='form' width='100%'>"
            "<tr>"
            "<td colspan='4' class='lbl' width='33.3%'>工时费 %LFEE%</td>"
            "<td colspan='4' class='lbl' width='33.3%'>材料费 %MFEE%</td>"
            "<td colspan='4' class='lbl' width='33.4%'>其它费 %OTH%</td>"
            "</tr>"
            "<tr>"
            "<td colspan='4' class='lbl' width='33.3%'>管理费 %MGM%</td>"
            "<td colspan='4' class='recv' width='33.3%'>合 计 %TOTAL%</td>"
            "<td colspan='4' class='lbl' width='33.4%'></td>"
            "</tr>"
            "</table>";
        qSummaryHtml.replace("%LFEE%", money(qLaborTotal)).replace("%MFEE%", money(qPartsTotal))
            .replace("%OTH%", money(qOth)).replace("%MGM%", money(qMgmt))
            .replace("%TOTAL%", money(qTotal));

        // 底部（与结算单同款）
        QString qFooterHtml =
            "<div class='order-no'>工号：%ORDER%</div>"
            "<table class='main' width='100%'>"
            "<tr><td width='66.7%'>地址：%ADDRESS%</td><td width='33.3%'>服务电话：%PHONE%</td></tr>"
            "</table>"
            "<table class='main' width='100%'>"
            "<tr>"
            "<td width='33.3%'>结算人：%SETTLER%</td>"
            "<td width='33.3%'>结算日期：%SETTLEDATE%</td>"
            "<td width='33.4%'>收款人签字：%SIGN%</td>"
            "</tr>"
            "</table>";
        qFooterHtml.replace("%ORDER%", esc(quoteOrderNo))
            .replace("%ADDRESS%", esc(QString::fromUtf8("成都市双流区航都大街二段370号")))
            .replace("%PHONE%", esc(QString::fromUtf8("028-________")))
            .replace("%SETTLER%", esc(QString::fromUtf8("演示结算员")))
            .replace("%SETTLEDATE%", QString("2026-08-24"))
            .replace("%SIGN%", QString("________________"));

        QList<SettlementSection> qSections;
        qSections << SettlementSection{PrintUtil::settlementWrap(qHeadHtml),    false}
                  << SettlementSection{PrintUtil::settlementWrap(qVehicleHtml), false}
                  << SettlementSection{PrintUtil::settlementWrap(qOwnerHtml),   false}
                  << SettlementSection{PrintUtil::settlementWrap(qOrderHtml),   false}
                  << SettlementSection{PrintUtil::settlementWrap(qContentHtml), false}
                  << SettlementSection{PrintUtil::settlementWrap(qLaborHtml),   true}
                  << SettlementSection{PrintUtil::settlementWrap(qPartsHtml),   true}
                  << SettlementSection{PrintUtil::settlementWrap(qSummaryHtml), false}
                  << SettlementSection{PrintUtil::settlementWrap(qFooterHtml),  false};

        {
            QFile mf("D:/temmpcode/4s/_scratch/pdf_render/measure.txt");
            if (mf.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                QTextStream ms(&mf);
                ms << "[quote] contentW=" << contentW << " usableH=" << usableH << "\n";
                double sumFixed = 0.0, sumGrow = 0.0;
                for (int i = 0; i < qSections.size(); ++i) {
                    double h = PrintUtil::measureSection(qSections.at(i).html, QSizeF(contentW, usableH));
                    if (qSections.at(i).grow) sumGrow += h; else sumFixed += h;
                    ms << "  qblock " << i << " grow=" << qSections.at(i).grow
                       << " natural=" << h << "\n";
                }
                double extra = usableH - sumFixed - sumGrow;
                ms << "  sumFixed=" << sumFixed << " sumGrow=" << sumGrow
                   << " extra=" << extra
                   << " growExtra=" << (extra > 0 ? extra / 2.0 : 0.0) << "\n"
                   << "  => " << (extra >= 0 ? "FILL_OK" : "OVERFLOW") << "\n";
                mf.close();
            }
        }

        PrintUtil::renderSectionsToPdf(qSections,
            "D:/temmpcode/4s/_scratch/pdf_render/quote_new.pdf", nullptr);

        {
            QImage img((int)(contentW * 3), (int)(usableH * 3), QImage::Format_ARGB32);
            img.fill(Qt::white);
            QPainter p(&img);
            p.scale(3, 3);
            PrintUtil::drawSections(p, qSections, QSizeF(contentW, usableH));
            p.end();
            img.save("D:/temmpcode/4s/_scratch/pdf_render/quote_new.png");
        }
    }

    qDebug() << "done";
    return 0;
}
