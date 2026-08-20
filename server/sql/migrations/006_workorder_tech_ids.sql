-- 006: t_workorder 增加三主修人列 mechanic_tech_id / body_tech_id / paint_tech_id（幂等）
SET @ex1 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='mechanic_tech_id');
SET @sql1 = IF(@ex1 = 0, 'ALTER TABLE t_workorder ADD COLUMN mechanic_tech_id INT NULL COMMENT ''机电主修人(员工ID)'' AFTER technician_id', 'SELECT 1');
PREPARE stmt1 FROM @sql1; EXECUTE stmt1; DEALLOCATE PREPARE stmt1;

SET @ex2 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='body_tech_id');
SET @sql2 = IF(@ex2 = 0, 'ALTER TABLE t_workorder ADD COLUMN body_tech_id INT NULL COMMENT ''钣金主修人(员工ID)'' AFTER mechanic_tech_id', 'SELECT 1');
PREPARE stmt2 FROM @sql2; EXECUTE stmt2; DEALLOCATE PREPARE stmt2;

SET @ex3 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder' AND COLUMN_NAME='paint_tech_id');
SET @sql3 = IF(@ex3 = 0, 'ALTER TABLE t_workorder ADD COLUMN paint_tech_id INT NULL COMMENT ''喷漆主修人(员工ID)'' AFTER body_tech_id', 'SELECT 1');
PREPARE stmt3 FROM @sql3; EXECUTE stmt3; DEALLOCATE PREPARE stmt3;
