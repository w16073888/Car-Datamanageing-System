-- ============================================================
-- 迁移：t_maintenance_history 新增 status 列
-- 说明：记录工单状态，支持全状态维修历史查询
-- 执行方式: mysql -u root -p garagedb < migration_maintenance_history_status.sql
-- ============================================================

-- 添加 status 列（已有数据的行默认为'已结算'）
SET @exist := (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
    WHERE TABLE_SCHEMA = 'garagedb' AND TABLE_NAME = 't_maintenance_history' AND COLUMN_NAME = 'status');
SET @sql := IF(@exist = 0,
    'ALTER TABLE t_maintenance_history ADD COLUMN status VARCHAR(20) NOT NULL DEFAULT ''已结算'' COMMENT ''工单状态'' AFTER workorder_id',
    'SELECT ''列 status 已存在，跳过'' AS message');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SELECT '迁移完成: t_maintenance_history.status 列已添加' AS message;
