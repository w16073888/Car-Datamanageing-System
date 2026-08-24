-- 011: 工单查询增加「优惠」字段（幂等）
--   t_workorder.discount / t_maintenance_history.discount
--   应收合计 = 各费用之和 − 优惠
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='discount');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_workorder ADD COLUMN discount DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT ''优惠金额'' AFTER management_fee',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='discount');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_maintenance_history ADD COLUMN discount DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT ''优惠金额'' AFTER management_fee',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
