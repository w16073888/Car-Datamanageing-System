-- ============================================================
-- 数据库升级脚本 — 2026-07-23
-- 为车辆报修一体化界面新增字段
-- 用法: USE garagedb; SOURCE migration_20260723.sql;
-- ============================================================

-- ======================== 1. t_vehicle 新增字段 ========================

-- 负责人
ALTER TABLE t_vehicle
  ADD COLUMN responsible_person VARCHAR(50) DEFAULT '' COMMENT '负责人'
  AFTER plate_number;

-- 地址
ALTER TABLE t_vehicle
  ADD COLUMN address VARCHAR(200) DEFAULT '' COMMENT '地址'
  AFTER vin;

-- 变速箱
ALTER TABLE t_vehicle
  ADD COLUMN transmission VARCHAR(20) DEFAULT '' COMMENT '变速箱(自动/手动/无级变速/双离合)'
  AFTER model;

-- 油类
ALTER TABLE t_vehicle
  ADD COLUMN fuel_type VARCHAR(20) DEFAULT '' COMMENT '油类(汽油/柴油/电动/混动/天然气)'
  AFTER transmission;

-- 地区
ALTER TABLE t_vehicle
  ADD COLUMN region VARCHAR(50) DEFAULT '' COMMENT '地区'
  AFTER fuel_type;

-- 颜色
ALTER TABLE t_vehicle
  ADD COLUMN color VARCHAR(20) DEFAULT '' COMMENT '颜色'
  AFTER region;

-- 当前里程
ALTER TABLE t_vehicle
  ADD COLUMN current_mileage INT DEFAULT 0 COMMENT '当前里程'
  AFTER color;


-- ======================== 2. t_workorder 新增字段 ========================

-- 客服工号
ALTER TABLE t_workorder
  ADD COLUMN customer_service_id VARCHAR(20) DEFAULT '' COMMENT '客服工号'
  AFTER technician_id;

-- 报修日期
ALTER TABLE t_workorder
  ADD COLUMN repair_date DATE COMMENT '报修日期'
  AFTER customer_service_id;

-- 预估日期
ALTER TABLE t_workorder
  ADD COLUMN estimated_date DATE COMMENT '预估完工日期'
  AFTER repair_date;

-- 班别
ALTER TABLE t_workorder
  ADD COLUMN shift VARCHAR(20) DEFAULT '' COMMENT '班别(白班/夜班)'
  AFTER status;

-- 报修情况
ALTER TABLE t_workorder
  ADD COLUMN repair_type VARCHAR(50) DEFAULT '正常维修' COMMENT '报修情况'
  AFTER shift;

-- 主修人
ALTER TABLE t_workorder
  ADD COLUMN main_technician VARCHAR(50) DEFAULT '' COMMENT '主修'
  AFTER repair_type;

-- 材料费
ALTER TABLE t_workorder
  ADD COLUMN material_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '材料费'
  AFTER total_amount;

-- 其它费
ALTER TABLE t_workorder
  ADD COLUMN other_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '其它费'
  AFTER material_fee;

-- 管理费
ALTER TABLE t_workorder
  ADD COLUMN management_fee DECIMAL(10,2) DEFAULT 0.00 COMMENT '管理费'
  AFTER other_fee;

-- 订金
ALTER TABLE t_workorder
  ADD COLUMN deposit DECIMAL(10,2) DEFAULT 0.00 COMMENT '订金'
  AFTER management_fee;

SELECT '数据库升级完成！新字段已添加。' AS message;
