-- ============================================================
-- 迁移：新增车辆维修历史表
-- 说明：每次工单结算完成后，记录完整的维修历史
-- 包含：维修日期、备件使用、维修项目、本次费用、累计消费
-- 执行时间：2026-07-26
-- ============================================================

-- 车辆维修历史表
CREATE TABLE IF NOT EXISTS t_maintenance_history (
    id               INT PRIMARY KEY AUTO_INCREMENT  COMMENT '记录ID',
    vehicle_id       INT NOT NULL                    COMMENT '关联车辆ID',
    workorder_id     INT NOT NULL UNIQUE             COMMENT '关联工单ID(一对一)',
    maintenance_date DATETIME NOT NULL               COMMENT '维修日期',
    total_amount     DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '本次消费总额',
    cumulative_amount DECIMAL(10,2) NOT NULL DEFAULT 0.00 COMMENT '累计消费(含本次)',
    parts_summary    TEXT                            COMMENT '备件使用摘要(名称x数量, ...)',
    repair_summary   TEXT                            COMMENT '维修项目摘要(机电/钣金/喷漆条目及价格)',
    created_at       DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',

    INDEX idx_vehicle (vehicle_id),
    INDEX idx_date (maintenance_date),
    FOREIGN KEY (vehicle_id) REFERENCES t_vehicle(id) ON DELETE CASCADE,
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='车辆维修历史表';

-- 验证
SELECT '迁移完成：t_maintenance_history 表已创建' AS message;
