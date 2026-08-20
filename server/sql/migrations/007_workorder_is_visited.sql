-- 007: t_workorder 增加 is_visited 回访状态列（幂等）
--   历史已回访（satisfaction 非空）的记录标记为已回访
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='is_visited');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_workorder ADD COLUMN is_visited ENUM(''未回访'',''已回访'') NOT NULL DEFAULT ''未回访'' COMMENT ''回访状态''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

UPDATE t_workorder SET is_visited='已回访' WHERE satisfaction IS NOT NULL;
