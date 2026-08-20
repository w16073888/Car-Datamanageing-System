-- 010: 客户表合并进车辆表 + 删除「最后光顾日期」列（幂等）
--   1) t_vehicle 增加 owner_name / owner_phone / owner_address（车主信息，置于车牌号前）
--   2) 删除 last_visit_date 列及其索引（用户确认不保留）
--   3) 把 t_customer 中 type='车主' 的姓名/电话/地址合并进 t_vehicle
--   4) 删除 t_customer 表

-- 1a. owner_name（车主姓名）
SET @ex1 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_vehicle' AND COLUMN_NAME='owner_name');
SET @sql1 = IF(@ex1 = 0, 'ALTER TABLE t_vehicle ADD COLUMN owner_name VARCHAR(50) COMMENT ''车主姓名'' AFTER id', 'SELECT 1');
PREPARE stmt1 FROM @sql1; EXECUTE stmt1; DEALLOCATE PREPARE stmt1;

-- 1b. owner_phone（车主电话）
SET @ex2 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_vehicle' AND COLUMN_NAME='owner_phone');
SET @sql2 = IF(@ex2 = 0, 'ALTER TABLE t_vehicle ADD COLUMN owner_phone VARCHAR(20) COMMENT ''车主电话'' AFTER owner_name', 'SELECT 1');
PREPARE stmt2 FROM @sql2; EXECUTE stmt2; DEALLOCATE PREPARE stmt2;

-- 1c. owner_address（车主地址）
SET @ex3 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_vehicle' AND COLUMN_NAME='owner_address');
SET @sql3 = IF(@ex3 = 0, 'ALTER TABLE t_vehicle ADD COLUMN owner_address VARCHAR(200) COMMENT ''车主地址'' AFTER owner_phone', 'SELECT 1');
PREPARE stmt3 FROM @sql3; EXECUTE stmt3; DEALLOCATE PREPARE stmt3;

-- 2a. 删除 last_visit_date 列（存在才删）
SET @ex4 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_vehicle' AND COLUMN_NAME='last_visit_date');
SET @sql4 = IF(@ex4 > 0, 'ALTER TABLE t_vehicle DROP COLUMN last_visit_date', 'SELECT 1');
PREPARE stmt4 FROM @sql4; EXECUTE stmt4; DEALLOCATE PREPARE stmt4;

-- 2b. 删除 idx_last_visit 索引（存在才删；若随列自动删除则为空操作）
SET @ex5 = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS
            WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_vehicle' AND INDEX_NAME='idx_last_visit');
SET @sql5 = IF(@ex5 > 0, 'ALTER TABLE t_vehicle DROP INDEX idx_last_visit', 'SELECT 1');
PREPARE stmt5 FROM @sql5; EXECUTE stmt5; DEALLOCATE PREPARE stmt5;

-- 3. 车主数据合并（仅 type='车主'；车辆无车主记录时保持新列为 NULL）
UPDATE t_vehicle v
LEFT JOIN t_customer c ON c.vehicle_id = v.id AND c.type = '车主'
SET v.owner_name = c.name,
    v.owner_phone = c.phone,
    v.owner_address = c.address;

-- 4. 删除客户表
DROP TABLE IF EXISTS t_customer;
