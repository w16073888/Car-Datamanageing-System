-- 016: 备件从「一物一记录」改为「一批一记录」（数量支持小数）
--
-- 背景：原模型每入库 1 件就插 1 行 t_part_instance（instance_sn 逐件编号），
--       同一编号进 10 件就是 10 行。现改为「一批一行」：
--         · 基础信息（备件编号 + 规格型号 + 供应商 + 适用车型）相同 → 合并成一行
--         · 数量记在 quantity 列，支持 3 位小数
--         · 出库时从该批次扣量，出库记录共享该批次的供应商信息
--
-- 本迁移做三件事：
--   1) t_part_instance 增加 quantity / spec / supplier / applicable_model 四列
--      （后三列是目录信息的冗余：t_parts.part_no 是唯一键，同编号不同供应商
--        在 t_parts 上放不下两条，必须由批次行自己保存）
--   2) 老行回填规格/供应商/适用车型
--   3) 老数据一次性归并：基础信息 + 状态 + 单价 + 归属工单/车辆/领取人/备注
--      全同的在库行合并成一行，quantity = 行数，并把 t_workorder_item /
--      t_inventory_log 里指向被合并行的 part_instance_id 改指到保留行
--
-- 幂等：重复执行不会重复加列、不会重复归并（归并后每组只剩一行）。
-- ★ 执行前请务必备份数据库（本迁移会删除被合并掉的行）。
--   备份：mysqldump -u test -p garagedb > garagedb_备份.sql

-- ============================================================
-- 1) 加列（幂等：存在则跳过）
-- ============================================================
SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_instance' AND COLUMN_NAME='quantity');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_instance ADD COLUMN quantity DECIMAL(10,3) NOT NULL DEFAULT 1 COMMENT ''本批数量(支持小数)'' AFTER instance_sn',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_instance' AND COLUMN_NAME='spec');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_instance ADD COLUMN spec VARCHAR(100) COMMENT ''规格型号(冗余,参与批次合并)'' AFTER part_id',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_instance' AND COLUMN_NAME='supplier');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_instance ADD COLUMN supplier VARCHAR(100) COMMENT ''供应商/生产厂家(冗余,参与批次合并)'' AFTER spec',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @ex = (SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
           WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='t_part_instance' AND COLUMN_NAME='applicable_model');
SET @sql = IF(@ex = 0,
   'ALTER TABLE t_part_instance ADD COLUMN applicable_model VARCHAR(200) COMMENT ''适用车型(冗余,参与批次合并)'' AFTER supplier',
   'SELECT 1');
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

-- ============================================================
-- 2) 老行回填规格/供应商/适用车型（取自备件目录；已填过的行不动）
-- ============================================================
UPDATE t_part_instance i
  JOIN t_parts p ON p.id = i.part_id
SET i.spec             = COALESCE(NULLIF(p.spec, ''), ''),
    i.supplier         = COALESCE(p.supplier, ''),
    i.applicable_model = COALESCE(p.applicable_model, '')
WHERE i.spec IS NULL;

-- ============================================================
-- 3) 建「老ID → 保留ID」映射
--    分组键：备件 + 规格 + 供应商 + 适用车型 + 状态 + 进货价 + 售价
--            + 归属工单 + 归属车辆 + 领取人 + 备注
--    （已领给不同工单/车辆的行不会被并到一起）
-- ============================================================
DROP TEMPORARY TABLE IF EXISTS tmp_pi_map;
CREATE TEMPORARY TABLE tmp_pi_map AS
SELECT i.id AS old_id,
       (SELECT MIN(i2.id) FROM t_part_instance i2
         WHERE i2.part_id          <=> i.part_id
           AND i2.spec             <=> i.spec
           AND i2.supplier         <=> i.supplier
           AND i2.applicable_model <=> i.applicable_model
           AND i2.status           <=> i.status
           AND i2.unit_purchase_price <=> i.unit_purchase_price
           AND i2.unit_sale_price     <=> i.unit_sale_price
           AND i2.workorder_id     <=> i.workorder_id
           AND i2.vehicle_id       <=> i.vehicle_id
           AND i2.recipient        <=> i.recipient
           AND i2.remark           <=> i.remark) AS keep_id
FROM t_part_instance i;

CREATE INDEX idx_tmp_pi_map_old ON tmp_pi_map (old_id);
CREATE INDEX idx_tmp_pi_map_keep ON tmp_pi_map (keep_id);

-- ============================================================
-- 4) 把指向「将被删除行」的引用改指到保留行
-- ============================================================
UPDATE t_workorder_item wi
  JOIN tmp_pi_map m ON m.old_id = wi.part_instance_id
SET wi.part_instance_id = m.keep_id
WHERE m.keep_id <> m.old_id;

UPDATE t_inventory_log il
  JOIN tmp_pi_map m ON m.old_id = il.part_instance_id
SET il.part_instance_id = m.keep_id
WHERE m.keep_id <> m.old_id;

-- ============================================================
-- 5) 保留行的数量 = 组内行数（原模型每行 1 件）
--    只在「确实发生了合并」(cnt > 1) 时才赋值：
--    否则重复执行本迁移会把已合并批次的真实数量抹成 1（幂等性靠这条保证）。
-- ============================================================
UPDATE t_part_instance i
  JOIN (SELECT keep_id, COUNT(*) AS cnt FROM tmp_pi_map GROUP BY keep_id) g
    ON g.keep_id = i.id
SET i.quantity = g.cnt
WHERE g.cnt > 1;

-- ============================================================
-- 6) 归并使用去向记录（把被合并行的 usage_log 追加到保留行）
-- ============================================================
SET SESSION group_concat_max_len = 1000000;
UPDATE t_part_instance i
  JOIN (SELECT m.keep_id,
               GROUP_CONCAT(i2.usage_log ORDER BY i2.id SEPARATOR '\n') AS logs
          FROM tmp_pi_map m
          JOIN t_part_instance i2 ON i2.id = m.old_id
         WHERE m.keep_id <> m.old_id
           AND i2.usage_log IS NOT NULL AND i2.usage_log <> ''
         GROUP BY m.keep_id) g
    ON g.keep_id = i.id
SET i.usage_log = CONCAT_WS('\n', i.usage_log, g.logs);

-- ============================================================
-- 7) 删除被合并掉的行，收尾
-- ============================================================
DELETE i FROM t_part_instance i
  JOIN tmp_pi_map m ON m.old_id = i.id
WHERE m.keep_id <> m.old_id;

DROP TEMPORARY TABLE IF EXISTS tmp_pi_map;

-- ============================================================
-- 8) 库存视图改为按批次数量计算
-- ============================================================
CREATE OR REPLACE VIEW v_parts_stock AS
SELECT
    p.id,
    p.part_no,
    p.name,
    p.spec,
    p.supplier,
    p.purchase_price,
    p.sale_price,
    p.warranty_period,
    p.applicable_model,
    ROUND(SUM(CASE WHEN i.status = '在库' THEN
          (i.quantity - COALESCE((SELECT SUM(wi.quantity) FROM t_workorder_item wi
                         WHERE wi.part_instance_id = i.id AND wi.item_type = '材料'), 0))
          ELSE 0 END), 3)                           AS stock_in_warehouse,
    ROUND(SUM(CASE WHEN i.status = '已领出' THEN i.quantity ELSE 0 END), 3) AS stock_checked_out,
    ROUND(SUM(CASE WHEN i.status = '已安装' THEN i.quantity ELSE 0 END), 3) AS stock_installed,
    ROUND(SUM(CASE WHEN i.status <> '已退货' THEN i.quantity ELSE 0 END), 3) AS stock_total
FROM t_parts p
LEFT JOIN t_part_instance i ON i.part_id = p.id
GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier,
         p.purchase_price, p.sale_price, p.warranty_period, p.applicable_model;

-- 完成后重建一下可出库库存缓存（t_parts.stock）
UPDATE t_parts p
SET p.stock = ROUND(COALESCE((SELECT SUM(i.quantity - COALESCE(
        (SELECT SUM(wi.quantity) FROM t_workorder_item wi
          WHERE wi.part_instance_id = i.id AND wi.item_type = '材料'), 0))
      FROM t_part_instance i
      WHERE i.part_id = p.id AND i.status = '在库'), 0), 3);
