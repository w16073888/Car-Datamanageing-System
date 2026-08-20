-- ============================================================
-- 迁移：回访信息从 t_return_visit 移植到 t_workorder
-- 执行日期：2026-07-31
-- ============================================================

-- 1. t_workorder 增加回访字段
ALTER TABLE t_workorder
    ADD COLUMN satisfaction ENUM('满意','一般','不满意') NULL COMMENT '满意度(回访)',
    ADD COLUMN remark        TEXT                        NULL COMMENT '回访备注',
    ADD COLUMN visitor_id    INT                         NULL COMMENT '回访人(员工ID)',
    ADD COLUMN visited_at    DATETIME                    NULL COMMENT '回访时间';

-- 2. 迁移已有回访数据到工单表
UPDATE t_workorder w
INNER JOIN t_return_visit rv ON rv.workorder_id = w.id
SET w.satisfaction = rv.satisfaction,
    w.remark        = rv.remark,
    w.visitor_id    = rv.visitor_id,
    w.visited_at    = rv.visited_at;

-- 3. 删除回访表
DROP TABLE IF EXISTS t_return_visit;

-- 4. 更新初始化脚本中的 DROP 顺序（移除 t_return_visit 行）
--    此操作需手动在 init.sql 中执行
