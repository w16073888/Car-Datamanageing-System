#ifndef DATADELETE_H
#define DATADELETE_H

#include <QJsonObject>

// ============================================================
// 数据管理删除命令：data.deleteRow
//   事务型/级联删除在服务端执行，并做职位权限校验。
//   - 回访记录：清除工单回访信息
//   - t_parts  ：事务内级联删除（日志/实例/采购/解绑明细/本体）
//   - t_workorder / 其他表：校验后单条 DELETE（客户端模型路径亦可）
// ============================================================
class DataDelete
{
public:
    static void registerCommands();
    static QJsonObject handleDeleteRow(const QJsonObject &params, const struct SessionInfo *s);
};

#endif // DATADELETE_H
