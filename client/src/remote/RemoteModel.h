#ifndef REMOTEMODEL_H
#define REMOTEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QVariantList>
#include <QList>
#include <QHash>
#include <QSet>
#include <QSqlError>
#include <QSqlRecord>

class RemoteQuery;

// ============================================================
// 远程表格模型：一个类同时替代 QSqlQueryModel（只读）与 QSqlTableModel（可编辑）
//
//  只读模式（替代 QSqlQueryModel）：
//     m_model->setQuery(query);            // query 为已 exec() 的 RemoteQuery
//
//  可编辑表模式（替代 QSqlTableModel）：
//     m_model->setTable("t_employee");
//     m_model->setEditStrategy(RemoteModel::OnFieldChange);
//     m_model->setFilter("..."); m_model->setSort(0, Qt::AscendingOrder);
//     m_model->select();
//     m_model->record(row).value("name");   // 读行
//     m_model->removeRow(row); m_model->submitAll();  // 删除
//     setData(...)  // 视图委托在单元格提交时调用 → 即时 UPDATE
// ============================================================
class RemoteModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum EditStrategy { OnFieldChange, OnManualSubmit, OnRowChange };

    explicit RemoteModel(QObject *parent = nullptr);

    // ---- 只读模式（替代 QSqlQueryModel）----
    void setQuery(RemoteQuery &q);     // 消费已执行的 RemoteQuery 结果
    void setRows(const QStringList &columns, const QList<QVariantList> &rows);
    // 直接填充结果（列名即表头，用于客户端合并/加工后的数据）
    void clear();

    // ---- 可编辑表模式（替代 QSqlTableModel）----
    void setTable(const QString &table);
    QString tableName() const;
    void setFilter(const QString &filter);
    QString filter() const;
    void select();
    void setEditStrategy(EditStrategy strategy);
    EditStrategy editStrategy() const;
    void setSort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void setColumnReadOnly(int column, bool readOnly = true);
    bool submitAll();
    void revertAll();
    QSqlError lastError() const;

    // ---- 兼容接口 ----
    QSqlRecord record(int row) const;
    QSqlRecord record() const;              // 空记录，仅字段名
    int fieldIndex(const QString &col) const;
    QVariant value(int row, int col) const;

    // ---- QAbstractItemModel ----
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value,
                       int role = Qt::DisplayRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    bool isEditable() const;
    void refreshFromServer();
    bool commitCell(int row, int col, const QVariant &value);
    QString pkValue(int row) const;
    void setModelError(const QString &err);
    void clearTableData();

    QString m_table;
    QString m_filter;
    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    EditStrategy m_strategy = OnManualSubmit;

    QStringList m_columns;
    QList<QVariantList> m_rows;
    QHash<int, QString> m_headers;      // section → 中文表头
    QSet<int> m_readOnlyColumns;        // 只读列集合（section 索引，单元格编辑表用）
    QHash<int, QVariantList> m_pendingEdits;  // row → 修改后的行（OnManualSubmit）
    QList<QString> m_pendingDeletes;    // 待删除行的主键值
    QSqlError m_lastError;
};

#endif // REMOTEMODEL_H
