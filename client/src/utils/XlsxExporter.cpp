#include "XlsxExporter.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QDateTime>
#include <QByteArray>
#include <QtGlobal>

// ============================================================
// 内部小工具：XML 转义 / 单元格引用 / CRC32 / ZIP 写入
// ============================================================

namespace {

// ---------- 基础字节写入 ----------
void putLE16(QByteArray &b, int off, quint16 v)
{
    b[off]     = static_cast<char>(v & 0xFF);
    b[off + 1] = static_cast<char>((v >> 8) & 0xFF);
}

void putLE32(QByteArray &b, int off, quint32 v)
{
    b[off]     = static_cast<char>(v & 0xFF);
    b[off + 1] = static_cast<char>((v >> 8) & 0xFF);
    b[off + 2] = static_cast<char>((v >> 16) & 0xFF);
    b[off + 3] = static_cast<char>((v >> 24) & 0xFF);
}

// ---------- XML 文本转义（同时剔除 XML 1.0 非法控制字符） ----------
QByteArray escapeXml(const QString &text)
{
    QString out;
    out.reserve(text.size() * 2);
    for (const QChar &ch : text) {
        const ushort u = ch.unicode();
        // 剔除 XML 1.0 不允许的控制字符（保留 \t \n \r）
        if (u < 0x20 && u != '\t' && u != '\n' && u != '\r')
            continue;
        switch (u) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default:   out += ch; break;
        }
    }
    return out.toUtf8();
}

// ---------- 单元格引用（A1, B2, ... AA10） ----------
QByteArray cellRef(int col, int row)
{
    QByteArray letters;
    int n = col + 1;
    while (n > 0) {
        const int rem = (n - 1) % 26;
        letters.prepend(static_cast<char>('A' + rem));
        n = (n - 1) / 26;
    }
    letters += QByteArray::number(row);
    return letters;
}

// ---------- 数值文本（整数不带小数点；小数最多 12 位有效数字） ----------
QByteArray numericText(double d)
{
    if (d == static_cast<long long>(d) && qAbs(d) < 1e15)
        return QByteArray::number(static_cast<long long>(d));
    return QByteArray::number(d, 'g', 12);
}

// ---------- 单个单元格 XML ----------
QByteArray cellXml(int col, int row, const QVariant &v)
{
    const QByteArray ref = cellRef(col, row);
    if (!v.isValid() || v.isNull())
        return "<c r=\"" + ref + "\" t=\"inlineStr\"><is><t/></is></c>";

    switch (v.type()) {
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return "<c r=\"" + ref + "\"><v>" + numericText(v.toDouble()) + "</v></c>";
    default:
        return "<c r=\"" + ref + "\" t=\"inlineStr\"><is><t xml:space=\"preserve\">"
               + escapeXml(v.toString()) + "</t></is></c>";
    }
}

// ---------- CRC32 ----------
quint32 crcTable[256];
bool crcTableReady = false;

void initCrcTable()
{
    for (int i = 0; i < 256; ++i) {
        quint32 c = static_cast<quint32>(i);
        for (int j = 0; j < 8; ++j)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crcTable[i] = c;
    }
    crcTableReady = true;
}

quint32 crc32(const QByteArray &data)
{
    if (!crcTableReady)
        initCrcTable();
    quint32 c = 0xFFFFFFFFu;
    for (char ch : data)
        c = crcTable[(c ^ static_cast<quint8>(ch)) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ---------- 极简 ZIP 写入器（stored 方式） ----------
struct ZipEntry {
    QByteArray name;
    QByteArray data;
};

QByteArray buildZip(const QList<ZipEntry> &entries)
{
    QByteArray out;
    QList<quint32> offsets;
    offsets.reserve(entries.size());

    // ---- 本地文件头 + 数据 ----
    for (const ZipEntry &e : entries) {
        offsets.append(static_cast<quint32>(out.size()));
        const quint32 crc = crc32(e.data);

        QByteArray hdr(30, Qt::Uninitialized);
        hdr[0] = 0x50; hdr[1] = 0x4B; hdr[2] = 0x03; hdr[3] = 0x04;  // 0x04034b50
        putLE16(hdr, 4, 20);        // 版本
        putLE16(hdr, 6, 0x0800);    // 标志：UTF-8 文件名
        putLE16(hdr, 8, 0);         // 方法：stored
        putLE16(hdr, 10, 0);        // 修改时间
        putLE16(hdr, 12, 0x5021);   // 修改日期 2020-01-01
        putLE32(hdr, 14, crc);
        putLE32(hdr, 18, static_cast<quint32>(e.data.size()));
        putLE32(hdr, 22, static_cast<quint32>(e.data.size()));
        putLE16(hdr, 26, static_cast<quint16>(e.name.size()));
        putLE16(hdr, 28, 0);        // 额外区长度

        out += hdr;
        out += e.name;
        out += e.data;
    }

    // ---- 中央目录 ----
    const quint32 cdStart = static_cast<quint32>(out.size());
    for (int i = 0; i < entries.size(); ++i) {
        const ZipEntry &e = entries[i];
        const quint32 crc = crc32(e.data);

        QByteArray hdr(46, Qt::Uninitialized);
        hdr[0] = 0x50; hdr[1] = 0x4B; hdr[2] = 0x01; hdr[3] = 0x02;  // 0x02014b50
        putLE16(hdr, 4, 20);        // 制作版本
        putLE16(hdr, 6, 20);        // 需要版本
        putLE16(hdr, 8, 0x0800);    // 标志：UTF-8
        putLE16(hdr, 10, 0);        // 方法
        putLE16(hdr, 12, 0);        // 时间
        putLE16(hdr, 14, 0x5021);   // 日期
        putLE32(hdr, 16, crc);
        putLE32(hdr, 20, static_cast<quint32>(e.data.size()));
        putLE32(hdr, 24, static_cast<quint32>(e.data.size()));
        putLE16(hdr, 28, static_cast<quint16>(e.name.size()));
        putLE16(hdr, 30, 0);        // 额外区
        putLE16(hdr, 32, 0);        // 注释
        putLE16(hdr, 34, 0);        // 磁盘号
        putLE16(hdr, 36, 0);        // 内部属性
        putLE32(hdr, 38, 0);        // 外部属性
        putLE32(hdr, 42, offsets[i]);

        out += hdr;
        out += e.name;
    }
    const quint32 cdSize = static_cast<quint32>(out.size()) - cdStart;

    // ---- 结尾记录 ----
    QByteArray eocd(22, Qt::Uninitialized);
    eocd[0] = 0x50; eocd[1] = 0x4B; eocd[2] = 0x05; eocd[3] = 0x06;  // 0x06054b50
    putLE16(eocd, 4, 0);            // 当前磁盘
    putLE16(eocd, 6, 0);            // 中央目录起始磁盘
    putLE16(eocd, 8, static_cast<quint16>(entries.size()));
    putLE16(eocd, 10, static_cast<quint16>(entries.size()));
    putLE32(eocd, 12, cdSize);
    putLE32(eocd, 16, cdStart);
    putLE16(eocd, 20, 0);           // 注释长度
    out += eocd;

    return out;
}

// ---------- XLSX 固定 XML 部件 ----------
QByteArray contentTypesXml()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
           "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
           "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
           "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
           "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
           "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
           "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>"
           "</Types>";
}

QByteArray relsXml()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
           "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
           "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
           "</Relationships>";
}

QByteArray workbookXml(const QString &sheetName)
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
           "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
           "<sheets><sheet name=\"" + escapeXml(sheetName) + "\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
           "</workbook>";
}

QByteArray workbookRelsXml()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
           "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
           "</Relationships>";
}

QByteArray corePropsXml()
{
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
           "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
           "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
           "xmlns:dcmitype=\"http://purl.org/dc/dcmitype/\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
           "<dc:creator>4s-client</dc:creator>"
           "<dcterms:created xsi:type=\"dcterms:W3CDTF\">" + escapeXml(now) + "</dcterms:created>"
           "</cp:coreProperties>";
}

QByteArray appPropsXml()
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
           "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\" "
           "xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\">"
           "<Application>4s-client</Application>"
           "</Properties>";
}

// ---------- 工作表 XML ----------
QByteArray sheetXml(const QStringList &columns, const QList<QVariantList> &rows)
{
    QByteArray sheet;
    sheet += "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    sheet += "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>\n";

    // 表头行（强制文本）
    sheet += "<row r=\"1\">";
    for (int c = 0; c < columns.size(); ++c) {
        sheet += "<c r=\"" + cellRef(c, 1) + "\" t=\"inlineStr\"><is><t xml:space=\"preserve\">"
                 + escapeXml(columns[c]) + "</t></is></c>";
    }
    sheet += "</row>";

    // 数据行
    for (int r = 0; r < rows.size(); ++r) {
        sheet += "<row r=\"" + QByteArray::number(r + 2) + "\">";
        const QVariantList &row = rows[r];
        for (int c = 0; c < columns.size(); ++c)
            sheet += cellXml(c, r + 2, c < row.size() ? row[c] : QVariant());
        sheet += "</row>";
    }

    sheet += "</sheetData></worksheet>";
    return sheet;
}

QString sanitizeSheetName(const QString &name)
{
    QString n = name.trimmed();
    if (n.isEmpty())
        n = QStringLiteral("Sheet1");
    QString out;
    out.reserve(n.size());
    for (const QChar &ch : n) {
        const ushort u = ch.unicode();
        if (u == '[' || u == ']' || u == ':' || u == '*' || u == '?' || u == '/' || u == '\\')
            out += '_';
        else
            out += ch;
    }
    if (out.size() > 31)
        out = out.left(31);
    return out;
}

} // namespace

// ============================================================
// 公共接口
// ============================================================

QString XlsxExporter::writeModel(const QString &filePath,
                                 const QAbstractItemModel *model,
                                 const QString &sheetName)
{
    if (!model)
        return QStringLiteral("数据模型为空");
    const int cols = model->columnCount();
    const int rows = model->rowCount();
    if (cols <= 0)
        return QStringLiteral("表格没有列，无法导出");

    QStringList columns;
    columns.reserve(cols);
    for (int c = 0; c < cols; ++c)
        columns << model->headerData(c, Qt::Horizontal).toString();

    QList<QVariantList> data;
    data.reserve(rows);
    for (int r = 0; r < rows; ++r) {
        QVariantList row;
        row.reserve(cols);
        for (int c = 0; c < cols; ++c)
            row << model->index(r, c).data(Qt::DisplayRole);
        data.append(row);
    }

    return writeFile(filePath, columns, data, sheetName);
}

QString XlsxExporter::writeFile(const QString &filePath,
                                const QStringList &columns,
                                const QList<QVariantList> &rows,
                                const QString &sheetName)
{
    if (columns.isEmpty())
        return QStringLiteral("没有表头，无法导出");

    QList<ZipEntry> entries;
    entries.append({QByteArray("[Content_Types].xml"), contentTypesXml()});
    entries.append({QByteArray("_rels/.rels"), relsXml()});
    entries.append({QByteArray("xl/workbook.xml"), workbookXml(sanitizeSheetName(sheetName))});
    entries.append({QByteArray("xl/_rels/workbook.xml.rels"), workbookRelsXml()});
    entries.append({QByteArray("xl/worksheets/sheet1.xml"), sheetXml(columns, rows)});
    entries.append({QByteArray("docProps/core.xml"), corePropsXml()});
    entries.append({QByteArray("docProps/app.xml"), appPropsXml()});

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QStringLiteral("无法写入文件：%1").arg(filePath);
    f.write(buildZip(entries));
    f.close();
    return QString();
}
