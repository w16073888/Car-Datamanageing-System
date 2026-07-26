-- ============================================================
-- 汽修4S店综合管理系统 - MySQL 8.0 数据库初始化脚本
-- 作者: Qt/C++ 开发工程师
-- 日期: 2026-07-24
-- 说明: 执行前请先创建数据库
--   CREATE DATABASE IF NOT EXISTS garagedb
--     DEFAULT CHARACTER SET utf8mb4
--     DEFAULT COLLATE utf8mb4_unicode_ci;
--   USE garagedb;
-- ============================================================

-- 清空旧表（按依赖顺序反向删除）
DROP TABLE IF EXISTS t_system_log;
DROP TABLE IF EXISTS t_return_visit;
DROP TABLE IF EXISTS t_settlement;
DROP TABLE IF EXISTS t_technician_work_record;
DROP TABLE IF EXISTS t_workorder_repair_item;
DROP TABLE IF EXISTS t_inventory_log;
DROP TABLE IF EXISTS t_quote_item;
DROP TABLE IF EXISTS t_workorder_item;
DROP TABLE IF EXISTS t_parts;
DROP TABLE IF EXISTS t_workorder;
DROP TABLE IF EXISTS t_customer;
DROP TABLE IF EXISTS t_vehicle;
DROP TABLE IF EXISTS t_employee;

-- ======================== 1. 员工表 ========================
CREATE TABLE t_employee (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '员工ID',
    employee_id     VARCHAR(20)     NOT NULL UNIQUE             COMMENT '工号',
    name            VARCHAR(50)     NOT NULL                    COMMENT '姓名',
    password        VARCHAR(64)     NOT NULL                    COMMENT '密码(SHA256)',
    position        ENUM('总经理','服务顾问','维修技师','仓库管理员')
                                    NOT NULL                    COMMENT '职位',
    phone           VARCHAR(20)                                 COMMENT '联系电话',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    is_active       TINYINT(1)      DEFAULT 1                   COMMENT '是否启用(1=启用,0=禁用)'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='员工表';

-- 插入默认管理员（明文密码: admin123）
INSERT INTO t_employee (employee_id, name, password, position, phone) VALUES
('ADMIN001', '系统管理员', 'admin123', '总经理', '13800000000'),
('SV0001',   '服务顾问张三', '123456', '服务顾问', '13800000001'),
('TECH001',  '技师李四',   '123456', '维修技师', '13800000002'),
('WH001',    '库管王五',   '123456', '仓库管理员', '13800000003');

-- ======================== 2. 车辆档案表 ========================
CREATE TABLE t_vehicle (
    id                      INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '车辆ID',
    plate_number            VARCHAR(20)     NOT NULL UNIQUE             COMMENT '车牌号',
    vin                     VARCHAR(50)                                COMMENT '车架号(VIN)',
    engine_number           VARCHAR(50)                                COMMENT '发动机号',
    brand                   VARCHAR(50)                                COMMENT '厂家/品牌',
    model                   VARCHAR(50)                                COMMENT '车型/型号',
    color                   VARCHAR(20)                                COMMENT '颜色',
    fuel_type               VARCHAR(20)                                COMMENT '燃油类型',
    transmission            VARCHAR(20)                                COMMENT '变速箱',
    region                  VARCHAR(20)                                COMMENT '地区',
    current_mileage         INT             DEFAULT 0                  COMMENT '当前公里数',
    purchase_date           DATE                                       COMMENT '购车日期',
    inspection_date         DATE                                       COMMENT '年审日期',
    insurance_date          DATE                                       COMMENT '保险日期',
    last_maintenance_date   DATE                                       COMMENT '最后保养日期',
    last_maintenance_mileage INT                                       COMMENT '最后保养公里数',
    last_visit_date        DATE                                       COMMENT '最后光顾日期',
    created_at              DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at              DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_plate     (plate_number),
    INDEX idx_vin       (vin),
    INDEX idx_last_visit (last_visit_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆档案表';

-- ======================== 3. 客户联系人表 ========================
CREATE TABLE t_customer (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '客户ID',
    vehicle_id      INT             NOT NULL                   COMMENT '关联车辆ID',
    name            VARCHAR(50)     NOT NULL                   COMMENT '姓名',
    phone           VARCHAR(20)                                COMMENT '电话',
    address         VARCHAR(200)                               COMMENT '地址',
    type            ENUM('车主','驾驶员') DEFAULT '车主'        COMMENT '类型',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_vehicle (vehicle_id),
    INDEX idx_phone   (phone),
    FOREIGN KEY (vehicle_id) REFERENCES t_vehicle(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='客户联系人表';

-- ======================== 4. 备件库存表 ========================
CREATE TABLE t_parts (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '备件ID',
    part_no         VARCHAR(50)     NOT NULL UNIQUE             COMMENT '备件编号',
    name            VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    spec            VARCHAR(100)                                COMMENT '规格型号',
    stock           INT             NOT NULL DEFAULT 0         COMMENT '当前库存量',
    purchase_price  DECIMAL(10,2)   NOT NULL                   COMMENT '进货价',
    sale_price      DECIMAL(10,2)   NOT NULL                   COMMENT '销售价',
    supplier        VARCHAR(100)                                COMMENT '供应商',
    warranty_period VARCHAR(50)                                COMMENT '质保期(如:12个月/365天)',
    applicable_model VARCHAR(200)                              COMMENT '适用车型(如:丰田凯美瑞/本田雅阁)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_part_no (part_no),
    INDEX idx_name    (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件库存表';

-- ======================== 5. 工单主表 ========================
CREATE TABLE t_workorder (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '工单ID',
    order_no        VARCHAR(30)     NOT NULL UNIQUE             COMMENT '工单号(WO+年月日+4位流水)',
    vehicle_id      INT             NOT NULL                   COMMENT '关联车辆ID',
    technician_id   INT                                        COMMENT '主修人(员工ID)',
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
    main_technician VARCHAR(50)                                COMMENT '主修人姓名(冗余)',
    repair_date     DATE                                       COMMENT '报修日期',
    estimated_date  DATE                                       COMMENT '预估完工日期',
    status          ENUM('待派工','维修中','已完工','已提单','已结算')
                                    DEFAULT '待派工'            COMMENT '工单状态',
    created_by      INT                                        COMMENT '创建人(员工ID)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_order_no  (order_no),
    INDEX idx_vehicle   (vehicle_id),
    INDEX idx_technician (technician_id),
    INDEX idx_status    (status),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (vehicle_id)    REFERENCES t_vehicle(id),
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单主表';

-- ======================== 5b. 工单维修项目明细表 ========================
CREATE TABLE IF NOT EXISTS t_workorder_repair_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    item_type       VARCHAR(10)     NOT NULL                   COMMENT '项目类型(机电/钣金/喷漆)',
    repair_person   VARCHAR(50)                                COMMENT '维修人姓名',
    repair_content  VARCHAR(200)                               COMMENT '维修内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单维修项目明细表(机电/钣金/喷漆)';

-- ======================== 5c. 技师工作记录表 ========================
CREATE TABLE IF NOT EXISTS t_technician_work_record (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '记录ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    technician_id   INT                                        COMMENT '技师ID',
    item_type       VARCHAR(10)                                COMMENT '项目类型(机电/钣金/喷漆)',
    work_content    VARCHAR(200)                               COMMENT '工作内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    INDEX idx_technician (technician_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE,
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='技师工作记录表(用于统计工作量)';

-- ======================== 6. 工单备件使用表 ========================
CREATE TABLE t_workorder_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    part_id         INT                                        COMMENT '关联备件ID(NULL表示非备件项目)',
    part_name       VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    quantity        INT             NOT NULL DEFAULT 1         COMMENT '数量',
    unit_price      DECIMAL(10,2)   NOT NULL                   COMMENT '单价',
    subtotal        DECIMAL(10,2)   GENERATED ALWAYS AS (quantity * unit_price) STORED COMMENT '小计(自动计算)',
    item_type       ENUM('材料','工时') DEFAULT '材料'         COMMENT '项目类型',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE,
    FOREIGN KEY (part_id)      REFERENCES t_parts(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单备件使用表';

-- ======================== 7. 报价明细表 ========================
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

-- ======================== 8. 库存流水表 ========================
CREATE TABLE t_inventory_log (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '流水ID',
    part_id         INT             NOT NULL                   COMMENT '关联备件ID',
    quantity        INT             NOT NULL                   COMMENT '数量(正=入库, 负=出库)',
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
    INDEX idx_operation (operation_type),
    INDEX idx_ref_order (ref_order_no),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (part_id) REFERENCES t_parts(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存流水表';

-- ======================== 9. 结算记录表 ========================
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

-- ======================== 10. 回访记录表 ========================
CREATE TABLE t_return_visit (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '回访ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    satisfaction    ENUM('满意','一般','不满意') NOT NULL       COMMENT '满意度',
    remark          TEXT                                       COMMENT '备注',
    visitor_id      INT                                        COMMENT '回访人(员工ID)',
    visited_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '回访时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='回访记录表';

-- ======================== 11. 系统日志表 ========================
CREATE TABLE t_system_log (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '日志ID',
    operator_id     INT                                        COMMENT '操作人(员工ID)',
    action_type     VARCHAR(50)     NOT NULL                   COMMENT '操作类型',
    table_name      VARCHAR(50)                                COMMENT '操作表名',
    record_id       INT                                        COMMENT '操作记录ID',
    detail          TEXT                                       COMMENT '操作详情',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '操作时间',

    INDEX idx_operator  (operator_id),
    INDEX idx_action    (action_type),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='系统日志表';

-- ======================== 12. 车辆交易历史表 ========================
CREATE TABLE t_vehicle_transaction (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '交易ID',
    vehicle_id      INT             NOT NULL                   COMMENT '关联车辆ID',
    workorder_id    INT                                        COMMENT '关联工单ID',
    transaction_type ENUM('进厂维修','保养','结算','其他') DEFAULT '进厂维修' COMMENT '交易类型',
    description     TEXT                                       COMMENT '交易描述',
    amount          DECIMAL(10,2)                              COMMENT '交易金额',
    operator_id     INT                                        COMMENT '操作人(员工ID)',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '交易时间',

    INDEX idx_vehicle (vehicle_id),
    INDEX idx_workorder (workorder_id),
    INDEX idx_created_at (created_at),
    FOREIGN KEY (vehicle_id) REFERENCES t_vehicle(id) ON DELETE CASCADE,
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆交易历史表';

-- ============================================================
-- 备件采购记录表（追踪备件从哪来）
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

    INDEX idx_part (part_id),
    INDEX idx_supplier (supplier),
    FOREIGN KEY (part_id) REFERENCES t_parts(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='备件采购记录表';

-- ======================== 数据库初始化验证 ========================
SELECT '数据库初始化完成！' AS message,
       COUNT(*) AS 表总数
FROM information_schema.tables
WHERE table_schema = DATABASE() AND table_type = 'BASE TABLE';
