-- 009: t_employee.position 列收敛为 ENUM('经理','前台','库管','客服')（幂等）
--   原客户端 EmployeePage::ensurePositionColumn() 运行时修复逻辑移入此处
SET @ct = (SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_employee' AND COLUMN_NAME='position');
SET @sql = IF(@ct IS NULL OR @ct NOT LIKE '%经理%前台%库管%客服%',
   'ALTER TABLE t_employee MODIFY COLUMN position ENUM(''经理'',''前台'',''库管'',''客服'') NOT NULL',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
