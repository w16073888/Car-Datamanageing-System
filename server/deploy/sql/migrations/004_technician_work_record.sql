-- 004: 创建 t_technician_work_record 表（技工工作记录，幂等）
CREATE TABLE IF NOT EXISTS t_technician_work_record (
    id              INT PRIMARY KEY AUTO_INCREMENT COMMENT '记录ID',
    workorder_id    INT NOT NULL COMMENT '关联工单ID',
    technician_id   INT NOT NULL COMMENT '技师(员工ID)',
    item_type       VARCHAR(10) COMMENT '项目类型(机电/钣金/喷漆/其他)',
    work_content    TEXT COMMENT '工作内容',
    fee             DECIMAL(10,2) DEFAULT 0.00 COMMENT '工时费用',
    created_at      DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    INDEX idx_workorder   (workorder_id),
    INDEX idx_technician  (technician_id),
    FOREIGN KEY (workorder_id)   REFERENCES t_workorder(id) ON DELETE CASCADE,
    FOREIGN KEY (technician_id) REFERENCES t_employee(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='技师工作记录表';
