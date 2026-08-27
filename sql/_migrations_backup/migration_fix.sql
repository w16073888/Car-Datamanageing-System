-- ============================================================
-- 数据库修复迁移脚本 — 兼容当前表结构
-- ============================================================

-- ======================== 1. t_vehicle 补充缺失列 ========================

-- 品牌（init.sql 中的列，但表里没有）
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS brand VARCHAR(50) DEFAULT '' COMMENT '厂家/品牌';
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS model VARCHAR(50) DEFAULT '' COMMENT '车型/型号';

-- 变速箱
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS transmission VARCHAR(20) DEFAULT '' COMMENT '变速箱(自动/手动/无级变速/双离合)';
-- 油类
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS fuel_type VARCHAR(20) DEFAULT '' COMMENT '油类(汽油/柴油/电动/混动/天然气)';
-- 地区
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS region VARCHAR(50) DEFAULT '' COMMENT '地区';
-- 颜色
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS color VARCHAR(20) DEFAULT '' COMMENT '颜色';
-- 当前里程
ALTER TABLE t_vehicle ADD COLUMN IF NOT EXISTS current_mileage INT DEFAULT 0 COMMENT '当前里程';

-- ======================== 2. t_workorder 补充缺失列 ========================

ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS customer_service_id VARCHAR(20) DEFAULT '' COMMENT '客服工号';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS repair_date DATE COMMENT '报修日期';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS estimated_date DATE COMMENT '预估完工日期';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS shift VARCHAR(20) DEFAULT '' COMMENT '班别(白班/夜班)';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS repair_type VARCHAR(50) DEFAULT '正常维修' COMMENT '报修情况';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS main_technician VARCHAR(50) DEFAULT '' COMMENT '主修';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS material_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '材料费';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS other_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '其它费';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS management_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '管理费';
ALTER TABLE t_workorder ADD COLUMN IF NOT EXISTS deposit DECIMAL(10,2) DEFAULT 0.00 COMMENT '订金';

SELECT '数据库修复完成！' AS message;
