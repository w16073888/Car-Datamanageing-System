-- 001: t_workorder.status 收敛为 4 态（已派工/待提单/已提单/已结算）
--   - 历史 '已完工' / '维修中' 记录迁移为 '已派工'
--   - ENUM 定义收敛为 4 态（幂等：仅在列类型不匹配时执行 ALTER）
UPDATE t_workorder SET status='已派工' WHERE status='已完工';
UPDATE t_workorder SET status='已派工' WHERE status='维修中';

SET @ct = (SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='status');
SET @sql = IF(@ct IS NULL
              OR @ct NOT LIKE '%已派工%待提单%已提单%已结算%'
              OR @ct LIKE '%待派工%' OR @ct LIKE '%已完工%' OR @ct LIKE '%维修中%',
   'ALTER TABLE t_workorder MODIFY COLUMN status ENUM(''已派工'',''待提单'',''已提单'',''已结算'') DEFAULT ''已派工'' COMMENT ''工单状态''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
