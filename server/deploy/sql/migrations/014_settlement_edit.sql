-- 014: 结算修改 overlay 表（幂等）
-- 前台业务-「结算修改」专用：存放已结算工单的修改后快照，
-- 与原始表完全隔离，绝不覆盖 t_workorder / t_workorder_repair_item /
-- t_workorder_item / t_settlement。其他入口（工单查询、财务、报表）仍读原表。
CREATE TABLE IF NOT EXISTS t_settlement_edit (
    workorder_id    INT             PRIMARY KEY                 COMMENT '关联工单ID(一对一)',
    labor_fee       DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '工时费(修改后)',
    material_fee    DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '材料费(修改后)',
    other_fee       DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '其它费(修改后)',
    management_fee  DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '管理费(修改后)',
    discount        DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '优惠金额(修改后)',
    total_amount    DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '应收合计(修改后)',
    operator_id     INT                                        COMMENT '最后修改人(员工ID)',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '最后修改时间',

    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='结算修改-费用快照表(仅结算修改入口显示)';

CREATE TABLE IF NOT EXISTS t_settlement_edit_repair (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    item_type       VARCHAR(10)     NOT NULL                   COMMENT '项目类型(机电/钣金/喷漆)',
    repair_person   VARCHAR(100)                               COMMENT '维修人姓名',
    repair_content  TEXT                                       COMMENT '维修内容',
    fee             DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '费用(修改后)',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '修改时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='结算修改-工时条目表(仅结算修改入口显示)';

CREATE TABLE IF NOT EXISTS t_settlement_edit_item (
    id              INT             PRIMARY KEY AUTO_INCREMENT  COMMENT '明细ID',
    workorder_id    INT             NOT NULL                   COMMENT '关联工单ID',
    part_id         INT             NULL                       COMMENT '关联备件ID(NULL=新增备件无目录)',
    part_name       VARCHAR(100)    NOT NULL                   COMMENT '备件名称',
    quantity        DECIMAL(10,3)   NOT NULL DEFAULT 1         COMMENT '数量(支持小数)',
    unit_price      DECIMAL(10,2)   NOT NULL                   COMMENT '单价(修改后售价)',
    subtotal        DECIMAL(10,2)   NOT NULL DEFAULT 0.00      COMMENT '小计(客户端算好,普通列)',
    item_type       VARCHAR(10)     NOT NULL DEFAULT '材料'    COMMENT '项目类型(仅材料)',
    updated_at      DATETIME        DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '修改时间',

    INDEX idx_workorder (workorder_id),
    FOREIGN KEY (workorder_id) REFERENCES t_workorder(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='结算修改-材料条目表(仅结算修改入口显示)';
