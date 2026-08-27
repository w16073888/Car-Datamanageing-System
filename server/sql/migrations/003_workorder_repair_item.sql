-- 003: 创建 t_workorder_repair_item 表（工时费明细，幂等）
CREATE TABLE IF NOT EXISTS t_workorder_repair_item (
    id              INT PRIMARY KEY AUTO_INCREMENT COMMENT '明细ID',
    workorder_id    INT NOT NULL COMMENT '关联工单ID',
    item_type       VARCHAR(10) NOT NULL COMMENT '项目类型(机电/钣金/喷漆)',
    repair_person   VARCHAR(100) COMMENT '维修人姓名',
    repair_content  TEXT COMMENT '维修内容',
    fee             DECIMAL(10,2) DEFAULT 0.00 COMMENT '费用',
    created_at      DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='工单维修项目明细表';
