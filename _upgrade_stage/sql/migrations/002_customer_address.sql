-- 002: t_customer 增加 address 列（幂等）
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_customer' AND COLUMN_NAME='address');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_customer ADD COLUMN address VARCHAR(200) COMMENT ''地址'' AFTER phone',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
