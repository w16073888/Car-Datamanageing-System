-- 008: t_maintenance_history 扩展字段（v2: 入出厂时间、里程、服务顾问等，幂等）
SET @ex1 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='entry_date');
SET @sql1 = IF(@ex1 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN entry_date DATETIME COMMENT ''入厂时间(派工时间)'' AFTER maintenance_date', 'SELECT 1');
PREPARE stmt1 FROM @sql1; EXECUTE stmt1; DEALLOCATE PREPARE stmt1;

SET @ex2 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='completion_date');
SET @sql2 = IF(@ex2 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN completion_date DATETIME COMMENT ''出厂时间(结算时间)'' AFTER entry_date', 'SELECT 1');
PREPARE stmt2 FROM @sql2; EXECUTE stmt2; DEALLOCATE PREPARE stmt2;

SET @ex3 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='mileage');
SET @sql3 = IF(@ex3 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN mileage INT COMMENT ''里程(km)'' AFTER completion_date', 'SELECT 1');
PREPARE stmt3 FROM @sql3; EXECUTE stmt3; DEALLOCATE PREPARE stmt3;

SET @ex4 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='service_advisor');
SET @sql4 = IF(@ex4 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN service_advisor VARCHAR(50) COMMENT ''服务顾问姓名'' AFTER mileage', 'SELECT 1');
PREPARE stmt4 FROM @sql4; EXECUTE stmt4; DEALLOCATE PREPARE stmt4;

SET @ex5 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='technicians');
SET @sql5 = IF(@ex5 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN technicians VARCHAR(200) COMMENT ''维修人员'' AFTER service_advisor', 'SELECT 1');
PREPARE stmt5 FROM @sql5; EXECUTE stmt5; DEALLOCATE PREPARE stmt5;

SET @ex6 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='labor_fee');
SET @sql6 = IF(@ex6 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN labor_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT ''工时费'' AFTER cumulative_amount', 'SELECT 1');
PREPARE stmt6 FROM @sql6; EXECUTE stmt6; DEALLOCATE PREPARE stmt6;

SET @ex7 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='material_fee');
SET @sql7 = IF(@ex7 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN material_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT ''材料费'' AFTER labor_fee', 'SELECT 1');
PREPARE stmt7 FROM @sql7; EXECUTE stmt7; DEALLOCATE PREPARE stmt7;

SET @ex8 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='other_fee');
SET @sql8 = IF(@ex8 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN other_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT ''其它费'' AFTER material_fee', 'SELECT 1');
PREPARE stmt8 FROM @sql8; EXECUTE stmt8; DEALLOCATE PREPARE stmt8;

SET @ex9 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='management_fee');
SET @sql9 = IF(@ex9 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN management_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT ''管理费'' AFTER other_fee', 'SELECT 1');
PREPARE stmt9 FROM @sql9; EXECUTE stmt9; DEALLOCATE PREPARE stmt9;

SET @ex10 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
             WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='deposit');
SET @sql10 = IF(@ex10 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN deposit DECIMAL(10,2) DEFAULT 0.00 COMMENT ''订金(已收)'' AFTER management_fee', 'SELECT 1');
PREPARE stmt10 FROM @sql10; EXECUTE stmt10; DEALLOCATE PREPARE stmt10;

SET @ex11 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
             WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_maintenance_history' AND COLUMN_NAME='repair_items');
SET @sql11 = IF(@ex11 = 0, 'ALTER TABLE t_maintenance_history ADD COLUMN repair_items TEXT COMMENT ''报修项目明细(JSON)'' AFTER repair_summary', 'SELECT 1');
PREPARE stmt11 FROM @sql11; EXECUTE stmt11; DEALLOCATE PREPARE stmt11;
