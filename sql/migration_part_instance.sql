-- ============================================================
-- 汽修4S店综合管理系统 - 备件实例化追踪数据库迁移脚本
-- 说明:
--   1. t_parts 表: spec/supplier/purchase_price/sale_price 改为可选
--   2. 新建 t_part_instance 表: 每个物理备件一条记录，追踪生命周期
--   3. t_workorder_item / t_inventory_log 增加 part_instance_id
--   4. 迁移现有数据: 根据 stock 生成实例记录
-- 执行方式: mysql -u root -p garagedb < migration_part_instance.sql
-- ============================================================

-- ======================== 1. t_parts 字段改为可选 ========================
ALTER TABLE t_parts
    MODIFY COLUMN spec VARCHAR(100) NULL COMMENT '规格型号(可选)',
    MODIFY COLUMN supplier VARCHAR(100) NULL COMMENT '供应商(可选)',
    MODIFY COLUMN purchase_price DECIMAL(10,2) NULL COMMENT '进货价(可选)',
    MODIFY COLUMN sale_price DECIMAL(10,2) NULL COMMENT '销售价(可选)';

-- ======================== 2. 新建备件实例表 ========================
DROP TABLE IF EXISTS t_part_instance;
CREATE TABLE t_part_instance (
    id                  INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '实例ID',
    part_id             INT             NOT NULL                   COMMENT '关联备件目录ID',
    instance_sn         VARCHAR(50)     NOT NULL UNIQUE             COMMENT '实例唯一编号',
    status              ENUM('在库','已领出','已安装','已退库','已退货')
                                        NOT NULL DEFAULT '在库'     COMMENT '实例状态',
    vehicle_id          INT             NULL                       COMMENT '安装到的车辆ID',
    workorder_id        INT             NULL                       COMMENT '出库关联工单ID',
    purchase_batch_id   INT             NULL                       COMMENT '采购批次ID',
    unit_purchase_price DECIMAL(10,2)   NULL                       COMMENT '进货单价',
    unit_sale_price     DECIMAL(10,2)   NULL                       COMMENT '销售单价',
    recipient           VARCHAR(50)     NULL                       COMMENT '领取人姓名',
    remark              VARCHAR(255)    NULL                       COMMENT '备注',
    created_at          DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at          DATETIME        DEFAULT CURRENT_TIMESTAMP
                                        ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_part      (part_id),
    INDEX idx_status    (status),
    INDEX idx_vehicle   (vehicle_id),
    INDEX idx_workorder (workorder_id),
    INDEX idx_sn        (instance_sn),

    FOREIGN KEY (part_id)       REFERENCES t_parts(id),
    FOREIGN KEY (vehicle_id)    REFERENCES t_vehicle(id) ON DELETE SET NULL,
    FOREIGN KEY (workorder_id)  REFERENCES t_workorder(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件实例表(每个物理备件一条记录)';

-- ======================== 3. t_workorder_item 增加实例关联 ========================
ALTER TABLE t_workorder_item
    ADD COLUMN part_instance_id INT NULL COMMENT '关联备件实例ID' AFTER part_id,
    ADD INDEX idx_instance (part_instance_id),
    ADD FOREIGN KEY (part_instance_id) REFERENCES t_part_instance(id) ON DELETE SET NULL;

-- ======================== 4. t_inventory_log 增加实例关联 ========================
ALTER TABLE t_inventory_log
    ADD COLUMN part_instance_id INT NULL COMMENT '关联备件实例ID' AFTER part_id,
    ADD INDEX idx_instance (part_instance_id);

-- ======================== 5. 数据迁移: 现有库存 → 实例记录 ========================
-- 为每个有库存的备件创建对应数量的实例记录

DELIMITER //
DROP PROCEDURE IF EXISTS migrate_to_instances //
CREATE PROCEDURE migrate_to_instances()
BEGIN
    DECLARE done INT DEFAULT FALSE;
    DECLARE v_part_id INT;
    DECLARE v_part_no VARCHAR(50);
    DECLARE v_stock INT;
    DECLARE v_purchase_price DECIMAL(10,2);
    DECLARE v_sale_price DECIMAL(10,2);
    DECLARE v_supplier VARCHAR(100);
    DECLARE i INT;
    DECLARE v_seq INT DEFAULT 1;
    DECLARE v_sn VARCHAR(50);

    DECLARE cur CURSOR FOR
        SELECT id, part_no, stock, purchase_price, sale_price, supplier
        FROM t_parts
        WHERE stock > 0;
    DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

    OPEN cur;

    read_loop: LOOP
        FETCH cur INTO v_part_id, v_part_no, v_stock, v_purchase_price, v_sale_price, v_supplier;
        IF done THEN
            LEAVE read_loop;
        END IF;

        SET i = 0;
        WHILE i < v_stock DO
            SET v_seq = (SELECT COUNT(*) FROM t_part_instance WHERE part_id = v_part_id) + 1;
            SET v_sn = CONCAT(v_part_no, '-', LPAD(v_seq, 4, '0'));
            INSERT INTO t_part_instance (part_id, instance_sn, status, unit_purchase_price, unit_sale_price, remark)
            VALUES (v_part_id, v_sn, '在库', v_purchase_price, v_sale_price, CONCAT('历史库存迁移-', v_supplier));
            SET i = i + 1;
        END WHILE;
    END LOOP;

    CLOSE cur;
END //
DELIMITER ;

-- 执行迁移存储过程
CALL migrate_to_instances();

-- 清理存储过程
DROP PROCEDURE IF EXISTS migrate_to_instances;

-- ======================== 6. 创建库存视图(方便查询) ========================
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
    COUNT(CASE WHEN i.status = '在库' THEN 1 END) AS stock_in_warehouse,
    COUNT(CASE WHEN i.status = '已领出' THEN 1 END) AS stock_checked_out,
    COUNT(CASE WHEN i.status = '已安装' THEN 1 END) AS stock_installed,
    COUNT(CASE WHEN i.status NOT IN ('已退货') THEN 1 END) AS stock_total
FROM t_parts p
LEFT JOIN t_part_instance i ON i.part_id = p.id
GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier,
         p.purchase_price, p.sale_price, p.warranty_period, p.applicable_model;

-- ======================== 验证 ========================
SELECT '迁移完成! 备件实例表已创建，现有库存已转为实例记录。' AS message;

SELECT
    (SELECT COUNT(*) FROM t_parts) AS 备件种类数,
    (SELECT COUNT(*) FROM t_part_instance) AS 备件实例总数,
    (SELECT COUNT(*) FROM t_part_instance WHERE status = '在库') AS 在库实例数;
