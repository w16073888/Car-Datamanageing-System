-- ============================================================
-- 汽修4S店综合管理系统 - 前台工作台三主修人改造
-- 说明:
--   1. t_workorder 新增 mechanic_tech_id / body_tech_id / paint_tech_id
--   2. 保留原有的 technician_id / main_technician 兼容字段
-- 执行方式: mysql -u root -p garagedb < migration_three_techs.sql
-- ============================================================

-- ======================== 1. t_workorder 新增三个主修人字段 ========================
ALTER TABLE t_workorder
    ADD COLUMN IF NOT EXISTS mechanic_tech_id INT NULL COMMENT '机电主修人(员工ID)' AFTER technician_id,
    ADD COLUMN IF NOT EXISTS body_tech_id     INT NULL COMMENT '钣金主修人(员工ID)' AFTER mechanic_tech_id,
    ADD COLUMN IF NOT EXISTS paint_tech_id    INT NULL COMMENT '喷漆主修人(员工ID)' AFTER body_tech_id;

-- 索引
CREATE INDEX IF NOT EXISTS idx_mech_tech  ON t_workorder(mechanic_tech_id);
CREATE INDEX IF NOT EXISTS idx_body_tech  ON t_workorder(body_tech_id);
CREATE INDEX IF NOT EXISTS idx_paint_tech ON t_workorder(paint_tech_id);

-- ======================== 验证 ========================
SELECT '迁移完成! t_workorder 已新增 mechanic_tech_id / body_tech_id / paint_tech_id 三个字段。' AS message;
