-- ============================================================
-- 汽修4S店综合管理系统 - MySQL 8.0 数据库初始化脚本
-- 日期: 2026-07-31（合并所有迁移，一文件到位）
-- 用法:
--   CREATE DATABASE IF NOT EXISTS garagedb
--     DEFAULT CHARACTER SET utf8mb4
--     DEFAULT COLLATE utf8mb4_unicode_ci;
--   USE garagedb;
--   SOURCE init.sql;
-- 注意: 脚本内第一句 SET NAMES utf8mb4 必须保留，
--   否则在默认 GBK 连接下所有中文会乱码导致建表/插数据失败。
-- ============================================================

SET NAMES utf8mb4;

-- 清空旧表（按依赖顺序反向删除）
DROP TABLE IF EXISTS t_system_log;
DROP TABLE IF EXISTS t_maintenance_history;
DROP TABLE IF EXISTS t_vehicle_transaction;
DROP TABLE IF EXISTS t_part_purchase;
DROP TABLE IF EXISTS t_settlement;
DROP TABLE IF EXISTS t_technician_work_record;
DROP TABLE IF EXISTS t_workorder_repair_item;
DROP TABLE IF EXISTS t_workorder_item;
DROP TABLE IF EXISTS t_quote_item;
DROP TABLE IF EXISTS t_inventory_log;
DROP TABLE IF EXISTS t_part_instance;
DROP TABLE IF EXISTS t_parts;
DROP TABLE IF EXISTS t_workorder;
DROP TABLE IF EXISTS t_vehicle;
DROP TABLE IF EXISTS t_employee;

-- ============================================================
-- 1. 员工表
-- ============================================================
CREATE TABLE t_employee (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '员工ID',
    employee_id     VARCHAR(20)     NOT NULL UNIQUE             COMMENT '工号',
    name            VARCHAR(50)     NOT NULL                    COMMENT '姓名',
    password        VARCHAR(64)     NOT NULL                    COMMENT '密码(明文)',
    position        ENUM('经理','前台','库管','客服')
                                    NOT NULL                    COMMENT '职位',
    phone           VARCHAR(20)                                 COMMENT '联系电话',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    is_active       TINYINT(1)      DEFAULT 1                   COMMENT '是否启用(1=启用,0=禁用)'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='员工表';

INSERT INTO t_employee (employee_id, name, password, position, phone) VALUES
('1', '系统管理员', '123456', '经理', '');

-- ============================================================
-- 2. 车辆档案表
-- ============================================================
CREATE TABLE t_vehicle (
    id                      INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '车辆ID',
    owner_name              VARCHAR(50)                                COMMENT '车主姓名',
    owner_phone             VARCHAR(20)                                COMMENT '车主电话',
    owner_address           VARCHAR(200)                               COMMENT '车主地址',
    plate_number            VARCHAR(20)     NOT NULL UNIQUE             COMMENT '车牌号',
    vin                     VARCHAR(50)                                COMMENT '车架号(VIN)',
    engine_number           VARCHAR(50)                                COMMENT '发动机号',
    brand                   VARCHAR(50)                                COMMENT '厂家/品牌',
    model                   VARCHAR(50)                                COMMENT '车型/型号',
    color                   VARCHAR(20)                                COMMENT '颜色',
    fuel_type               VARCHAR(20)                                COMMENT '燃油类型',
    transmission            VARCHAR(20)                                COMMENT '变速箱',
    region                  VARCHAR(50)                                COMMENT '地区',
    current_mileage         INT             DEFAULT 0                  COMMENT '当前公里数',
    purchase_date           DATE                                       COMMENT '购车日期',
    inspection_date         DATE                                       COMMENT '年审日期',
    insurance_date          DATE                                       COMMENT '保险日期',
    last_maintenance_date   DATE                                       COMMENT '最后保养日期',
    last_maintenance_mileage INT                                       COMMENT '最后保养公里数',
    created_at              DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at              DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_plate      (plate_number),
    INDEX idx_vin        (vin)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆档案表';

-- ============================================================
-- 3. 备件库存表
-- ============================================================
CREATE TABLE t_parts (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '备件ID',
    part_no         VARCHAR(50)     NOT NULL UNIQUE             COMMENT '备件编号',
    name            VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    spec            VARCHAR(100)                                COMMENT '规格型号(可选)',
    stock           INT             NOT NULL DEFAULT 0         COMMENT '当前库存量(缓存)',
    purchase_price  DECIMAL(10,2)                               COMMENT '进货价(可选)',
    sale_price      DECIMAL(10,2)                               COMMENT '销售价(可选)',
    supplier        VARCHAR(100)                                COMMENT '供应商(可选)',
    warranty_period VARCHAR(50)                                COMMENT '质保期(如:12个月/365天)',
    applicable_model VARCHAR(200)                              COMMENT '适用车型(如:丰田凯美瑞/本田雅阁)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_part_no (part_no),
    INDEX idx_name    (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件库存表';

-- ============================================================
-- 5. 工单主表
-- ============================================================
CREATE TABLE t_workorder (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '工单ID',
    order_no        VARCHAR(30)     NOT NULL UNIQUE             COMMENT '工单号(WO+年月日+4位流水)',
    vehicle_id      INT             NOT NULL                   COMMENT '关联车辆ID',
    technician_id   INT                                        COMMENT '主修人(兼容旧字段)',
    mechanic_tech_id INT                                       COMMENT '机电主修人(员工ID)',
    body_tech_id     INT                                       COMMENT '钣金主修人(员工ID)',
    paint_tech_id    INT                                       COMMENT '喷漆主修人(员工ID)',
    customer_service_id INT                                   COMMENT '服务顾问(员工ID)',
    mileage         INT             DEFAULT 0                  COMMENT '当前公里数',
    repair_content  TEXT                                       COMMENT '报修内容',
    labor_fee       DECIMAL(10,2)   DEFAULT 0.00              COMMENT '工时费(机电+钣金+喷漆)',
    material_fee    DECIMAL(10,2)   DEFAULT 0.00              COMMENT '材料费',
    other_fee       DECIMAL(10,2)   DEFAULT 0.00              COMMENT '其它费',
    management_fee  DECIMAL(10,2)   DEFAULT 0.00              COMMENT '管理费',
    total_amount    DECIMAL(10,2)   DEFAULT 0.00              COMMENT '总金额',
    deposit         DECIMAL(10,2)   DEFAULT 0.00              COMMENT '订金',
    shift           VARCHAR(10)                                COMMENT '班别(白班/夜班)',
    main_technician VARCHAR(50)                                COMMENT '主修人姓名(兼容旧字段)',
    repair_date     DATE                                       COMMENT '报修日期',
    estimated_date  DATE                                       COMMENT '预估完工日期',
    status          ENUM('已派工','待提单','已提单','已结算')
                                    DEFAULT '已派工'            COMMENT '工单状态',
    -- 回访字段（原 t_return_visit 并入）
    satisfaction    ENUM('满意','一般','不满意')                COMMENT '满意度(回访)',
    remark          TEXT                                       COMMENT '回访备注',
    visitor_id      INT                                        COMMENT '回访人(员工ID)',
    visited_at      DATETIME                                   COMMENT '回访时间',
    created_by      INT                                        COMMENT '创建人(员工ID)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    is_visited      ENUM('未回访','已回访') NOT NULL DEFAULT '未回访' COMMENT '回访状态',

    INDEX idx_cs_id      (customer_service_id),
    INDEX idx_order_no   (order_no),
    INDEX idx_vehicle    (vehicle_id),
    INDEX idx_technician (technician_id),
    INDEX idx_mech_tech  (mechanic_tech_id),
    INDEX idx_body_tech  (body_tech_id),
    INDEX idx_paint_tech (paint_tech_id),
    INDEX idx_status     (status),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (vehicle_id)    REFERENCES t_vehicle(id),
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单主表';

-- ============================================================
-- 6. 工单维修项目明细表
-- ============================================================
CREATE TABLE t_workorder_repair_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    item_type       VARCHAR(10)     NOT NULL                   COMMENT '项目类型(机电/钣金/喷漆)',
    repair_person   VARCHAR(100)                               COMMENT '维修人姓名',
    repair_content  TEXT                                       COMMENT '维修内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单维修项目明细表';

-- ============================================================
-- 7. 技师工作记录表
-- ============================================================
CREATE TABLE t_technician_work_record (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '记录ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    technician_id   INT             NOT NULL                   COMMENT '技师(员工ID)',
    item_type       VARCHAR(10)                                COMMENT '项目类型(机电/钣金/喷漆/其他)',
    work_content    TEXT                                       COMMENT '工作内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '工时费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder   (workorder_id),
    INDEX idx_technician  (technician_id),
    FOREIGN KEY (workorder_id)   REFERENCES t_workorder(id) ON DELETE CASCADE,
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='技师工作记录表';

-- ============================================================
-- 8. 备件实例表
-- ============================================================
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
    FOREIGN KEY (vehicle_id)    REFERENCES t_vehicle(id)    ON DELETE SET NULL,
    FOREIGN KEY (workorder_id)  REFERENCES t_workorder(id)  ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件实例表(一物一记录)';

-- ============================================================
-- 9. 工单备件使用表
-- ============================================================
CREATE TABLE t_workorder_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    part_id         INT                                        COMMENT '关联备件ID(NULL表示非备件项目)',
    part_instance_id INT                                       COMMENT '关联备件实例ID',
    part_name       VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    quantity        INT             NOT NULL DEFAULT 1         COMMENT '数量',
    unit_price      DECIMAL(10,2)   NOT NULL                   COMMENT '单价',
    subtotal        DECIMAL(10,2)   GENERATED ALWAYS AS (quantity * unit_price) STORED COMMENT '小计(自动计算)',
    item_type       ENUM('材料','工时') DEFAULT '材料'         COMMENT '项目类型',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    INDEX idx_instance  (part_instance_id),
    FOREIGN KEY (workorder_id)      REFERENCES t_workorder(id)     ON DELETE CASCADE,
    FOREIGN KEY (part_id)           REFERENCES t_parts(id),
    FOREIGN KEY (part_instance_id)  REFERENCES t_part_instance(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单备件使用表';

-- ============================================================
-- 10. 报价明细表
-- ============================================================
CREATE TABLE t_quote_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '报价ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    part_name       VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    quantity        INT             NOT NULL DEFAULT 1         COMMENT '数量',
    unit_price      DECIMAL(10,2)   NOT NULL                   COMMENT '单价',
    subtotal        DECIMAL(10,2)   GENERATED ALWAYS AS (quantity * unit_price) STORED COMMENT '小计(自动计算)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='报价明细表';

-- ============================================================
-- 11. 库存流水表
-- ============================================================
CREATE TABLE t_inventory_log (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '流水ID',
    part_id         INT             NOT NULL                   COMMENT '关联备件ID',
    part_instance_id INT                                       COMMENT '关联备件实例ID',
    quantity        INT             NOT NULL                   COMMENT '数量(正=入库,负=出库)',
    unit_price      DECIMAL(10,2)                              COMMENT '单价(出入库时的价格)',
    total_price     DECIMAL(10,2)                              COMMENT '总价',
    operation_type  ENUM('采购入库','维修出库','备件退库','采购退货','盘点调整','材料结算')
                                    NOT NULL                   COMMENT '操作类型',
    ref_order_no    VARCHAR(30)                                COMMENT '关联单号(工单号/采购单号)',
    operator_id     INT                                        COMMENT '操作人(员工ID)',
    recipient       VARCHAR(50)                                COMMENT '领取人',
    remark          VARCHAR(255)                               COMMENT '备注',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '操作时间',

    INDEX idx_part      (part_id),
    INDEX idx_instance  (part_instance_id),
    INDEX idx_operation (operation_type),
    INDEX idx_ref_order (ref_order_no),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (part_id) REFERENCES t_parts(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存流水表';

-- ============================================================
-- 12. 结算记录表
-- ============================================================
CREATE TABLE t_settlement (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '结算ID',
    workorder_id    INT             NOT NULL UNIQUE             COMMENT '关联工单ID(一对一)',
    labor_fee       DECIMAL(10,2)   NOT NULL                   COMMENT '工时费',
    material_fee    DECIMAL(10,2)   NOT NULL                   COMMENT '材料费',
    total_amount    DECIMAL(10,2)   NOT NULL                   COMMENT '总金额',
    settled_by      INT                                        COMMENT '结算人(员工ID)',
    settled_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '结算时间',

    INDEX idx_workorder (workorder_id),
    INDEX idx_settled_at (settled_at),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='结算记录表';

-- ============================================================
-- 13. 备件采购记录表
-- ============================================================
CREATE TABLE t_part_purchase (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '采购ID',
    part_id         INT             NOT NULL                   COMMENT '备件ID',
    supplier        VARCHAR(100)                               COMMENT '供应商/厂家',
    batch_no        VARCHAR(50)                                COMMENT '批次号',
    quantity        INT             NOT NULL                   COMMENT '采购数量',
    unit_cost       DECIMAL(10,2)   NOT NULL                   COMMENT '进货单价',
    total_cost      DECIMAL(10,2)   NOT NULL                   COMMENT '总成本',
    purchase_date   DATE                                       COMMENT '采购日期',
    operator_id     INT                                        COMMENT '操作人',
    remark          VARCHAR(255)                               COMMENT '备注',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_part     (part_id),
    INDEX idx_supplier (supplier),
    FOREIGN KEY (part_id) REFERENCES t_parts(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件采购记录表';

-- ============================================================
-- 14. 车辆维修历史表
-- ============================================================
CREATE TABLE t_maintenance_history (
    id               INT PRIMARY KEY AUTO_INCREMENT  COMMENT '记录ID',
    vehicle_id       INT NOT NULL                    COMMENT '关联车辆ID',
    workorder_id     INT NOT NULL UNIQUE             COMMENT '关联工单ID(一对一)',
    status           VARCHAR(20) NOT NULL DEFAULT '已结算' COMMENT '工单状态',
    maintenance_date DATETIME NOT NULL               COMMENT '维修日期(结算时间)',
    entry_date       DATETIME                        COMMENT '入厂时间(派工时间)',
    completion_date  DATETIME                        COMMENT '出厂时间(结算时间)',
    mileage          INT                             COMMENT '里程(km)',
    service_advisor  VARCHAR(50)                     COMMENT '服务顾问姓名',
    technicians      VARCHAR(200)                    COMMENT '维修人员(逗号分隔)',
    total_amount     DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '本次消费总额',
    cumulative_amount DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '累计消费(含本次)',
    labor_fee        DECIMAL(10,2) DEFAULT 0.00      COMMENT '工时费',
    material_fee     DECIMAL(10,2) DEFAULT 0.00      COMMENT '材料费',
    other_fee        DECIMAL(10,2) DEFAULT 0.00      COMMENT '其它费',
    management_fee   DECIMAL(10,2) DEFAULT 0.00      COMMENT '管理费',
    deposit          DECIMAL(10,2) DEFAULT 0.00      COMMENT '订金(已收)',
    parts_summary    TEXT                            COMMENT '备件使用摘要(名称x数量, ...)',
    repair_summary   TEXT                            COMMENT '维修项目摘要(机电/钣金/喷漆条目及价格)',
    repair_items     TEXT                            COMMENT '报修项目明细(JSON)',
    created_at       DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',

    INDEX idx_vehicle (vehicle_id),
    INDEX idx_date    (maintenance_date),
    FOREIGN KEY (vehicle_id)   REFERENCES t_vehicle(id)   ON DELETE CASCADE,
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆维修历史表';

-- ============================================================
-- 15. 车辆交易历史表
-- ============================================================
CREATE TABLE t_vehicle_transaction (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '交易ID',
    vehicle_id      INT             NOT NULL                   COMMENT '关联车辆ID',
    workorder_id    INT                                        COMMENT '关联工单ID',
    transaction_type ENUM('进厂维修','保养','结算','提单','其他')
                                    DEFAULT '进厂维修'          COMMENT '交易类型',
    description     TEXT                                       COMMENT '交易描述',
    amount          DECIMAL(10,2)                              COMMENT '交易金额',
    operator_id     INT                                        COMMENT '操作人(员工ID)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '交易时间',

    INDEX idx_vehicle    (vehicle_id),
    INDEX idx_workorder  (workorder_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (vehicle_id)   REFERENCES t_vehicle(id)   ON DELETE CASCADE,
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆交易历史表';

-- ============================================================
-- 16. 系统日志表
-- ============================================================
CREATE TABLE t_system_log (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '日志ID',
    operator_id     INT                                        COMMENT '操作人(员工ID)',
    action_type     VARCHAR(50)     NOT NULL                   COMMENT '操作类型',
    table_name      VARCHAR(50)                                COMMENT '操作表名',
    record_id       INT                                        COMMENT '操作记录ID',
    detail          TEXT                                       COMMENT '操作详情',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '操作时间',

    INDEX idx_operator   (operator_id),
    INDEX idx_action     (action_type),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='系统日志表';

-- ============================================================
-- 17. 库存视图
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
    COUNT(CASE WHEN i.status = '在库' THEN 1 END)   AS stock_in_warehouse,
    COUNT(CASE WHEN i.status = '已领出' THEN 1 END) AS stock_checked_out,
    COUNT(CASE WHEN i.status = '已安装' THEN 1 END) AS stock_installed,
    COUNT(CASE WHEN i.status NOT IN ('已退货') THEN 1 END) AS stock_total
FROM t_parts p
LEFT JOIN t_part_instance i ON i.part_id = p.id
GROUP BY p.id, p.part_no, p.name, p.spec, p.supplier,
         p.purchase_price, p.sale_price, p.warranty_period, p.applicable_model;

-- ============================================================
-- 验证
-- ============================================================
SELECT '数据库初始化完成！' AS message,
       COUNT(*) AS 表总数
FROM information_schema.tables
WHERE table_schema = DATABASE() AND table_type = 'BASE TABLE';
