#ifndef XLSXEXPORTER_H
#define XLSXEXPORTER_H

#include <QString>
#include <QStringList>
#include <QVariantList>

class QAbstractItemModel;

// ============================================================
// 极简 XLSX 导出工具（不依赖第三方库 / Office COM / ActiveQt）
//
// 直接手工生成符合 Office Open XML 规范的最小 .xlsx 文件：
//   - xlsx 本质是一个 zip 容器，内部是若干 XML 部件；
//   - 本工具内置一个极简 ZIP 写入器（stored 方式 + CRC32），
//     无需 qCompress / QuaZip 等压缩库；
//   - 单元格使用 inlineStr 内联字符串，数字写为数值单元格，
//     文本（如备件编号、车牌号）写为字符串，可保留前导 0。
//
// 用法：
//   QString err = XlsxExporter::writeModel(path, model, sheetName);
//   返回空串表示成功，否则为错误信息。
// ============================================================
class XlsxExporter
{
public:
    // 从任意 QAbstractItemModel 导出（表头取 headerData，内容取 DisplayRole）
    static QString writeModel(const QString &filePath,
                              const QAbstractItemModel *model,
                              const QString &sheetName = QStringLiteral("Sheet1"));

    // 直接按「表头 + 行数据」导出
    static QString writeFile(const QString &filePath,
                             const QStringList &columns,
                             const QList<QVariantList> &rows,
                             const QString &sheetName = QStringLiteral("Sheet1"));
};

#endif // XLSXEXPORTER_H
