-- ============================================================
-- 车辆维修历史表 v2 扩展
-- 新增字段：入出厂时间、里程、服务顾问、维修人员、费用明细、报修项目JSON
-- 执行方式: mysql -u root -p garagedb < migration_maintenance_history_v2.sql
-- ============================================================

ALTER TABLE t_maintenance_history
    ADD COLUMN IF NOT EXISTS entry_date       DATETIME        COMMENT '入厂时间(派工时间)' AFTER maintenance_date,
    ADD COLUMN IF NOT EXISTS completion_date  DATETIME        COMMENT '出厂时间(结算时间)' AFTER entry_date,
    ADD COLUMN IF NOT EXISTS mileage          INT             COMMENT '里程(km)' AFTER completion_date,
    ADD COLUMN IF NOT EXISTS service_advisor  VARCHAR(50)     COMMENT '服务顾问姓名' AFTER mileage,
    ADD COLUMN IF NOT EXISTS technicians      VARCHAR(200)    COMMENT '维修人员(逗号分隔)' AFTER service_advisor,
    ADD COLUMN IF NOT EXISTS labor_fee        DECIMAL(10,2)   DEFAULT 0.00 COMMENT '工时费' AFTER cumulative_amount,
    ADD COLUMN IF NOT EXISTS material_fee     DECIMAL(10,2)   DEFAULT 0.00 COMMENT '材料费' AFTER labor_fee,
    ADD COLUMN IF NOT EXISTS other_fee        DECIMAL(10,2)   DEFAULT 0.00 COMMENT '其它费' AFTER material_fee,
    ADD COLUMN IF NOT EXISTS management_fee   DECIMAL(10,2)   DEFAULT 0.00 COMMENT '管理费' AFTER other_fee,
    ADD COLUMN IF NOT EXISTS deposit          DECIMAL(10,2)   DEFAULT 0.00 COMMENT '订金(已收)' AFTER management_fee,
    ADD COLUMN IF NOT EXISTS repair_items     TEXT            COMMENT '报修项目明细(JSON)' AFTER repair_summary;

SELECT '迁移完成: t_maintenance_history v2 字段扩展' AS message;
