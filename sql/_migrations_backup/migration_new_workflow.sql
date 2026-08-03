-- ============================================================
-- 汽修4S店综合管理系统 - 新工作流数据库迁移脚本
-- 修改说明：
--   1. t_employee.position 更新为包含「维修技师」
--   2. t_workorder 新增状态「待提单」「已提单」
--   3. t_workorder 新增字段：客服、日期、班别、主修、费用明细等
--   4. 新增 t_workorder_repair_item — 机电/钣金/喷漆条目
--   5. 新增 t_technician_work_record — 技师工作量追踪
--   6. t_parts 新增厂家/型号等追踪字段
--   7. 更新默认员工数据适配新职位
-- ============================================================

-- ======================== 1. 更新员工表职位枚举 ========================
ALTER TABLE t_employee MODIFY COLUMN position
    ENUM('总经理','服务顾问','维修技师','仓库管理员') NOT NULL
    COMMENT '职位';

-- 将旧职位映射到新职位
UPDATE t_employee SET position = '总经理' WHERE position = '经理';
UPDATE t_employee SET position = '服务顾问' WHERE position = '前台';
UPDATE t_employee SET position = '仓库管理员' WHERE position = '库管';

-- ======================== 2. 更新工单状态枚举 ========================
ALTER TABLE t_workorder MODIFY COLUMN status
    ENUM('已派工','待提单','已提单','已结算')
    DEFAULT '已派工' COMMENT '工单状态';

-- ======================== 3. t_workorder 新增字段 ========================
ALTER TABLE t_workorder
    ADD COLUMN IF NOT EXISTS customer_service_id INT
        COMMENT '客服(服务顾问)ID' AFTER technician_id,
    ADD COLUMN IF NOT EXISTS repair_date DATE
        AFTER customer_service_id,
    ADD COLUMN IF NOT EXISTS estimated_date DATE
        AFTER repair_date,
    ADD COLUMN IF NOT EXISTS shift VARCHAR(10)
        AFTER estimated_date,
    ADD COLUMN IF NOT EXISTS main_technician VARCHAR(50)
        AFTER shift,
    ADD COLUMN IF NOT EXISTS material_fee DECIMAL(10,2) DEFAULT 0.00
        AFTER labor_fee,
    ADD COLUMN IF NOT EXISTS other_fee DECIMAL(10,2) DEFAULT 0.00
        AFTER material_fee,
    ADD COLUMN IF NOT EXISTS management_fee DECIMAL(10,2) DEFAULT 0.00
        AFTER other_fee,
    ADD COLUMN IF NOT EXISTS deposit DECIMAL(10,2) DEFAULT 0.00
        AFTER management_fee;

-- 索引
CREATE INDEX IF NOT EXISTS idx_cs_id ON t_workorder(customer_service_id);

-- ======================== 4. 工单维修项目明细表（机电/钣金/喷漆） ========================
DROP TABLE IF EXISTS t_workorder_repair_item;
CREATE TABLE t_workorder_repair_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    item_type       ENUM('机电','钣金','喷漆') NOT NULL        COMMENT '维修类别',
    repair_person   VARCHAR(100)                               COMMENT '维修人',
    repair_content  TEXT                                       COMMENT '维修内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单维修项目明细表（机电/钣金/喷漆）';

-- ======================== 5. 技师工作量记录表 ========================
DROP TABLE IF EXISTS t_technician_work_record;
CREATE TABLE t_technician_work_record (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '记录ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    technician_id   INT             NOT NULL                   COMMENT '技师(员工ID)',
    item_type       ENUM('机电','钣金','喷漆','其他')          COMMENT '维修类别',
    work_content    TEXT                                       COMMENT '工作内容',
    fee             DECIMAL(10,2)   DEFAULT 0.00              COMMENT '工时费用',
    created_at      DATETIME        DEFAULT CURRENT_TIMESTAMP   COMMENT '创建时间',

    INDEX idx_workorder (workorder_id),
    INDEX idx_technician (technician_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE,
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='技师工作量记录表';

-- ======================== 6. t_parts 新增追溯字段 ========================
ALTER TABLE t_parts
    ADD COLUMN IF NOT EXISTS manufacturer VARCHAR(100)
        COMMENT '生产厂家' AFTER spec,
    ADD COLUMN IF NOT EXISTS model_no VARCHAR(100)
        COMMENT '型号' AFTER manufacturer;

-- ======================== 7. t_vehicle 补充字段（旧版已有，新版确保存在） ========================
ALTER TABLE t_vehicle
    ADD COLUMN IF NOT EXISTS transmission VARCHAR(20)      COMMENT '变速箱' AFTER model,
    ADD COLUMN IF NOT EXISTS fuel_type VARCHAR(20)          COMMENT '油类' AFTER transmission,
    ADD COLUMN IF NOT EXISTS region VARCHAR(20)            COMMENT '地区' AFTER fuel_type,
    ADD COLUMN IF NOT EXISTS color VARCHAR(10)             COMMENT '颜色' AFTER region,
    ADD COLUMN IF NOT EXISTS current_mileage INT           COMMENT '当前里程' AFTER color,
    ADD COLUMN IF NOT EXISTS address VARCHAR(200)          COMMENT '地址' AFTER current_mileage,
    ADD COLUMN IF NOT EXISTS responsible_person VARCHAR(50) COMMENT '负责人' AFTER address;

-- ======================== 8. t_customer 补充地址字段 ========================
ALTER TABLE t_customer
    ADD COLUMN IF NOT EXISTS address VARCHAR(200) COMMENT '地址' AFTER phone;

-- ======================== 9. t_vehicle_transaction 更新交易类型枚举 ========================
ALTER TABLE t_vehicle_transaction MODIFY COLUMN transaction_type
    ENUM('进厂维修','保养','结算','提单','其他')
    DEFAULT '进厂维修' COMMENT '交易类型';

-- ======================== 更新默认管理员密码适配新职位 ========================
-- 管理员已经是「总经理」，无需变更
-- 如有需要，可插入测试数据
INSERT IGNORE INTO t_employee (employee_id, name, password, position, phone) VALUES
('TECH001', '张技师', '123456', '维修技师', '13800000001'),
('TECH002', '李技师', '123456', '维修技师', '13800000002'),
('FRONT001', '王顾问', '123456', '服务顾问', '13800000003'),
('STORE001', '赵仓管', '123456', '仓库管理员', '13800000004');

-- ======================== 验证 ========================
SELECT '迁移完成!' AS message;
