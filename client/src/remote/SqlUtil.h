#ifndef SQLUTIL_H
#define SQLUTIL_H

#include <QString>
#include <QStringList>

// ============================================================
// 模糊搜索 SQL 构造工具（客户端侧）
//   统一处理 LIKE 通配符转义与条件拼接，收敛各处手写 LIKE：
//   1. 全部走参数化绑定，杜绝 SQL 注入（不再手动 replace("'","''")）；
//   2. 关键字里的 % _ ! 按字面量匹配（显式 ESCAPE '!'，不受 MySQL
//      sql_mode / NO_BACKSLASH_ESCAPES 影响，比默认反斜杠转义更稳）。
//   使用示例：
//     q.prepare("SELECT ... WHERE " + SqlUtil::likeConds({"p.part_no", "p.name"}, ":kw"));
//     q.bindValue(":kw", SqlUtil::likePattern(keyword));
// ============================================================
namespace SqlUtil {

// 转义 LIKE 通配符：先转义转义符本身(!→!!)，再 %→!%、_→!_。
// 返回可直接嵌入 LIKE 模式的值（不含 % 包裹）。
QString escapeLike(const QString &raw);

// 生成绑定用全模糊模式：%转义后的关键字%
QString likePattern(const QString &raw);

// 单字段 LIKE 条件： field LIKE :ph ESCAPE '!'
QString likeCond(const QString &field, const QString &placeholder);

// 多字段 OR LIKE 条件（同一关键字）：
//   (f1 LIKE :ph ESCAPE '!' OR f2 LIKE :ph ESCAPE '!')
QString likeConds(const QStringList &fields, const QString &placeholder);

}

#endif // SQLUTIL_H
