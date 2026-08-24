-- 013: 备件实例增加「使用去向记录」字符串列表列（幂等）
--   usage_log 每行一条记录，追加式，记录该实例全部去向：入库/出库/退库/退货/安装
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_instance' AND COLUMN_NAME='usage_log');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_instance ADD COLUMN usage_log TEXT COMMENT ''使用去向记录(每行一条,追加)'' AFTER remark',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
