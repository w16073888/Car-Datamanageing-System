-- 012: 备件数量支持小数（幂等）
--   出库/退库允许小数（实例剩余量追踪）；进货仍按整件建档（单位=1）
--   t_parts.stock            可出库数量(缓存) = SUM(在库实例剩余量)
--   t_workorder_item.quantity 工单材料明细数量（可小数）
--   t_inventory_log.quantity  库存流水数量（可小数）
--   t_part_purchase.quantity  采购数量（进货整件，类型兼容）
--   t_quote_item.quantity     报价明细数量（类型兼容）
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_parts' AND COLUMN_NAME='stock' AND DATA_TYPE='decimal');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_parts MODIFY COLUMN stock DECIMAL(10,3) NOT NULL DEFAULT 0 COMMENT ''可出库数量(缓存)''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_workorder_item' AND COLUMN_NAME='quantity' AND DATA_TYPE='decimal');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_workorder_item MODIFY COLUMN quantity DECIMAL(10,3) NOT NULL DEFAULT 1 COMMENT ''数量(支持小数)''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_inventory_log' AND COLUMN_NAME='quantity' AND DATA_TYPE='decimal');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_inventory_log MODIFY COLUMN quantity DECIMAL(10,3) NOT NULL DEFAULT 0 COMMENT ''数量(正=入库,负=出库)''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_purchase' AND COLUMN_NAME='quantity' AND DATA_TYPE='decimal');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_purchase MODIFY COLUMN quantity DECIMAL(10,3) NOT NULL DEFAULT 0 COMMENT ''采购数量''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_quote_item' AND COLUMN_NAME='quantity' AND DATA_TYPE='decimal');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_quote_item MODIFY COLUMN quantity DECIMAL(10,3) NOT NULL DEFAULT 1 COMMENT ''数量(支持小数)''',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
