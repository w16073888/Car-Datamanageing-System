#include "RemoteModel.h"
#include "RemoteQuery.h"
#include "RemoteClient.h"

#include <QSqlField>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

RemoteModel::RemoteModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

// ---------------- 只读模式 ----------------

void RemoteModel::setQuery(RemoteQuery &q)
{
    beginResetModel();
    m_columns = q.columns();
    m_rows.clear();
    for (int i = 0; i < q.rowCount(); ++i) {
        q.seek(i);
        QVariantList row;
        for (int j = 0; j < m_columns.size(); ++j)
            row.append(q.value(j));
        m_rows.append(row);
    }
    m_table.clear();
    endResetModel();
}

void RemoteModel::setRows(const QStringList &columns, const QList<QVariantList> &rows)
{
    beginResetModel();
    m_columns = columns;
    m_rows = rows;
    m_table.clear();
    endResetModel();
}

void RemoteModel::clear()
{
    beginResetModel();
    m_columns.clear();
    m_rows.clear();
    m_table.clear();
    endResetModel();
}

// ---------------- 可编辑表模式 ----------------

void RemoteModel::setTable(const QString &table)
{
    if (m_table == table)
        return;   // 同一张表无需重置状态（保留列头/排序）
    m_table = table;
    // 切换表：丢弃上一张表残留的列头、排序与只读列设置，避免表头/数据错位
    const int last = qMax(m_columns.size() - 1, 0);
    m_headers.clear();
    m_readOnlyColumns.clear();
    m_sortColumn = -1;
    m_sortOrder = Qt::AscendingOrder;
    emit headerDataChanged(Qt::Horizontal, 0, last);
}

QString RemoteModel::tableName() const
{
    return m_table;
}

void RemoteModel::setFilter(const QString &filter)
{
    m_filter = filter;
}

QString RemoteModel::filter() const
{
    return m_filter;
}

void RemoteModel::setEditStrategy(EditStrategy strategy)
{
    m_strategy = strategy;
}

RemoteModel::EditStrategy RemoteModel::editStrategy() const
{
    return m_strategy;
}

void RemoteModel::setSort(int column, Qt::SortOrder order)
{
    m_sortColumn = column;
    m_sortOrder = order;
}

void RemoteModel::setColumnReadOnly(int column, bool readOnly)
{
    if (readOnly)
        m_readOnlyColumns.insert(column);
    else
        m_readOnlyColumns.remove(column);
}

void RemoteModel::refreshFromServer()
{
    if (m_table.isEmpty())
        return;

    // 先取列名（LIMIT 0），排序需要列名
    RemoteQuery qCols;
    qCols.prepare("SELECT * FROM " + m_table + " WHERE 1=0");
    if (!qCols.exec()) {
        setModelError(qCols.lastError().text());
        clearTableData();   // 失败时清空旧数据，避免显示上一张表的残留行
        return;
    }
    const QStringList cols = qCols.columns();

    QString sql = "SELECT * FROM " + m_table;
    QString where = m_filter.trimmed();
    if (!where.isEmpty()) {
        if (!where.startsWith("WHERE", Qt::CaseInsensitive))
            sql += " WHERE " + where;
        else
            sql += " " + where;
    }
    if (m_sortColumn >= 0 && m_sortColumn < cols.size()) {
        sql += QString(" ORDER BY %1 %2")
                   .arg(cols[m_sortColumn])
                   .arg(m_sortOrder == Qt::AscendingOrder ? "ASC" : "DESC");
    }

    RemoteQuery q;
    q.prepare(sql);
    if (!q.exec()) {
        setModelError(q.lastError().text());
        clearTableData();   // 失败时清空旧数据，避免显示上一张表的残留行
        return;
    }

    beginResetModel();
    m_columns = q.columns();
    m_rows.clear();
    for (int i = 0; i < q.rowCount(); ++i) {
        q.seek(i);
        QVariantList row;
        for (int j = 0; j < m_columns.size(); ++j)
            row.append(q.value(j));
        m_rows.append(row);
    }
    m_pendingDeletes.clear();
    m_pendingEdits.clear();
    m_lastError = QSqlError();
    endResetModel();
}

void RemoteModel::select()
{
    refreshFromServer();
}

void RemoteModel::revertAll()
{
    m_pendingEdits.clear();
    m_pendingDeletes.clear();
    select();
}

bool RemoteModel::submitAll()
{
    if (m_table.isEmpty()) {
        setModelError("未设置数据表");
        return false;
    }

    // 提交待删除行
    for (const QString &pk : m_pendingDeletes) {
        RemoteQuery q;
        q.prepare(QString("DELETE FROM %1 WHERE id = :id").arg(m_table));
        q.bindValue(":id", pk);
        if (!q.exec()) {
            setModelError(q.lastError().text());
            return false;
        }
    }
    m_pendingDeletes.clear();

    // 提交待修改行（OnManualSubmit）
    QList<int> rows = m_pendingEdits.keys();
    std::sort(rows.begin(), rows.end());
    for (int row : rows) {
        const QVariantList &newRow = m_pendingEdits[row];
        for (int col = 0; col < m_columns.size() && col < newRow.size(); ++col) {
            if (newRow[col] != m_rows[row][col]) {
                RemoteQuery q;
                q.prepare(QString("UPDATE %1 SET %2 = :v WHERE id = :id")
                              .arg(m_table, m_columns[col]));
                q.bindValue(":v", newRow[col]);
                q.bindValue(":id", m_rows[row][0]);
                if (!q.exec()) {
                    setModelError(q.lastError().text());
                    return false;
                }
                m_rows[row][col] = newRow[col];
            }
        }
    }
    m_pendingEdits.clear();
    m_lastError = QSqlError();
    return true;
}

QSqlError RemoteModel::lastError() const
{
    return m_lastError;
}

// ---------------- 兼容接口 ----------------

QSqlRecord RemoteModel::record(int row) const
{
    QSqlRecord rec;
    if (row < 0 || row >= m_rows.size())
        return rec;
    for (int j = 0; j < m_columns.size(); ++j) {
        rec.append(QSqlField(m_columns[j], m_rows[row][j].type()));
        rec.setValue(j, m_rows[row][j]);
    }
    return rec;
}

QSqlRecord RemoteModel::record() const
{
    QSqlRecord rec;
    for (const QString &c : m_columns)
        rec.append(QSqlField(c, QVariant::String));
    return rec;
}

int RemoteModel::fieldIndex(const QString &col) const
{
    return m_columns.indexOf(col);
}

QVariant RemoteModel::value(int row, int col) const
{
    if (row < 0 || row >= m_rows.size() || col < 0 || col >= m_columns.size())
        return QVariant();
    return m_rows[row][col];
}

// ---------------- QAbstractItemModel ----------------

int RemoteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int RemoteModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_columns.size();
}

QVariant RemoteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()
        || index.column() < 0 || index.column() >= m_columns.size())
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return m_rows[index.row()][index.column()];
    return QVariant();
}

bool RemoteModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || !isEditable())
        return false;
    if (m_readOnlyColumns.contains(index.column()))
        return false;
    if (index.row() < 0 || index.row() >= m_rows.size()
        || index.column() < 0 || index.column() >= m_columns.size())
        return false;

    const int row = index.row();
    const int col = index.column();
    if (m_rows[row][col] == value)
        return true;

    if (m_strategy == OnFieldChange) {
        // 即时提交；失败则回滚缓存值并返回 false
        const QVariant old = m_rows[row][col];
        if (!commitCell(row, col, value)) {
            m_rows[row][col] = old;
            return false;
        }
        m_rows[row][col] = value;
        emit dataChanged(index, index, { role });
        return true;
    }

    // OnManualSubmit / OnRowChange：先缓存，submitAll() 时统一提交
    if (!m_pendingEdits.contains(row))
        m_pendingEdits[row] = m_rows[row];
    m_pendingEdits[row][col] = value;
    m_rows[row][col] = value;
    emit dataChanged(index, index, { role });
    return true;
}

QVariant RemoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal) {
        if (m_headers.contains(section))
            return m_headers[section];
        if (section >= 0 && section < m_columns.size())
            return m_columns[section];
        return QVariant();
    }
    return section + 1;
}

bool RemoteModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return false;
    m_headers[section] = value.toString();
    emit headerDataChanged(orientation, section, section);
    return true;
}

Qt::ItemFlags RemoteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (isEditable() && !m_readOnlyColumns.contains(index.column()))
        f |= Qt::ItemIsEditable;
    return f;
}

bool RemoteModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || row < 0 || row + count > m_rows.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = row + count - 1; i >= row; --i) {
        if (!m_rows[i].isEmpty())
            m_pendingDeletes.append(m_rows[i][0].toString());   // 主键在列 0
        m_rows.removeAt(i);
    }
    endRemoveRows();
    return true;
}

// ---------------- 私有辅助 ----------------

bool RemoteModel::isEditable() const
{
    return m_strategy == OnFieldChange || m_strategy == OnManualSubmit || m_strategy == OnRowChange;
}

QString RemoteModel::pkValue(int row) const
{
    if (row < 0 || row >= m_rows.size() || m_rows[row].isEmpty())
        return QString();
    return m_rows[row][0].toString();
}

bool RemoteModel::commitCell(int row, int col, const QVariant &value)
{
    if (m_table.isEmpty())
        return false;

    RemoteQuery q;
    q.prepare(QString("UPDATE %1 SET %2 = :v WHERE id = :id").arg(m_table, m_columns[col]));
    q.bindValue(":v", value);
    q.bindValue(":id", pkValue(row));
    if (!q.exec()) {
        setModelError(q.lastError().text());
        return false;
    }
    return true;
}

void RemoteModel::setModelError(const QString &err)
{
    m_lastError = QSqlError(err, QString(), QSqlError::StatementError);
    qWarning() << "[RemoteModel]" << err;
}

// 查询失败时清空展示数据：防止上一张表的行数据/列头残留，导致表头与内容错位
void RemoteModel::clearTableData()
{
    beginResetModel();
    m_columns.clear();
    m_rows.clear();
    m_pendingDeletes.clear();
    m_pendingEdits.clear();
    m_lastError = QSqlError();
    endResetModel();
}
