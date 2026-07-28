-- ============================================================
-- 迁移: 将 '已派工' 加入 t_workorder.status ENUM
-- 说明:
--   前台操作台完成"保存并派工"后,工单状态应直接设为"已派工"
--   本脚本为已部署数据库追加该状态值
-- 执行方式: mysql -u root -p garagedb < migration_add_dispatched_status.sql
-- ============================================================

-- 检查当前 ENUM 定义
SELECT COLUMN_TYPE
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = 'garagedb'
  AND TABLE_NAME   = 't_workorder'
  AND COLUMN_NAME  = 'status';

-- 追加 '已派工' 到 ENUM
ALTER TABLE t_workorder
    MODIFY COLUMN status
        ENUM('已派工','待提单','维修中','已提单','已结算')
        DEFAULT '已派工'
        COMMENT '工单状态';

-- 验证修改结果
SHOW COLUMNS FROM t_workorder LIKE 'status';

SELECT '迁移完成: 已添加''已派工''状态' AS message;
